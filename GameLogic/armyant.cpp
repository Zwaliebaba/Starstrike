#include "pch.h"
#include "resource.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "math_utils.h"
#include "GameSimEventQueue.h"
#include "GameAppSim.h"
#include "location.h"
#include "unit.h"
#include "entity_grid.h"
#include "armyant.h"
#include "anthill.h"
#include "darwinian.h"

ArmyAnt::ArmyAnt()
  : Entity(),
    m_orders(NoOrders),
    m_targetFound(false),
    m_spiritId(-1)
{
  m_type = TypeArmyAnt;

  m_shapes[0] = g_app->m_resource->GetShapeStatic("armyant.shp");
  m_shapes[1] = g_app->m_resource->GetShapeStatic("armyant2.shp");
  m_shapes[2] = g_app->m_resource->GetShapeStatic("armyant3.shp");

  m_shape = m_shapes[0];
  m_carryMarker = m_shape->GetMarkerData("MarkerCarry");
}

void ArmyAnt::Begin()
{
  Entity::Begin();

  m_onGround = true;

  m_scale = 1.0f + syncsfrand(0.9f);

  float speed = m_stats[StatSpeed];
  speed *= (1.0f + syncsfrand(0.4f));
  if (speed < 0)
    speed = 0;
  if (speed > 255)
    speed = 255;
  m_stats[StatSpeed] = static_cast<unsigned char>(speed);
}

void ArmyAnt::ChangeHealth(int _amount)
{
  bool dead = m_dead;

  Entity::ChangeHealth(_amount);

  if (m_dead && !dead)
  {
    //
    // We just died

    Matrix34 transform(m_front, g_upVector, m_pos);
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;
    g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, transform, 1.0f));

    //
    // Drop any spirits we are carrying

    if (m_spiritId != -1)
    {
      if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
      {
        Spirit* spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
        if (spirit && spirit->m_state == Spirit::StateAttached)
        {
          spirit->CollectorDrops();
          spirit->m_vel = m_vel;
        }
      }
      m_spiritId = -1;
    }
  }
}

bool ArmyAnt::Advance(Unit* _unit)
{
  bool amIDead = Entity::Advance(_unit);

  if (!m_onGround)
    AdvanceInAir(_unit);

  if (!amIDead && !m_dead && m_onGround)
  {
    switch (m_orders)
    {
    case NoOrders:
      OrderReturnToBase();
      break;
    case ScoutArea:
      amIDead = AdvanceScoutArea();
      break;
    case CollectSpirit:
      amIDead = AdvanceCollectSpirit();
      break;
    case CollectEntity:
      amIDead = AdvanceCollectEntity();
      break;
    case AttackEnemy:
      amIDead = AdvanceAttackEnemy();
      break;
    case ReturnToBase:
      amIDead = AdvanceReturnToBase();
      break;
    case BaseDestroyed:
      amIDead = AdvanceBaseDestroyed();
      break;
    }

    if (!m_targetFound)
      m_targetFound = SearchForTargets();

    //
    // Keep attached spirits attached to us

    if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
    {
      Spirit* spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
      if (spirit && spirit->m_state == Spirit::StateAttached)
      {
        LegacyVector3 carryPos, carryVel;
        GetCarryMarker(carryPos, carryVel);
        spirit->m_pos = carryPos;
        spirit->m_vel = carryVel;
      }
    }
  }

  //
  // Move the legs on our model

  int currentIndex = -1;
  for (int i = 0; i < 3; ++i)
  {
    if (m_shape == m_shapes[i])
    {
      currentIndex = i;
      break;
    }
  }

  int newIndex = -1;
  while (true)
  {
    newIndex = darwiniaRandom() % 3;
    if (newIndex != currentIndex)
      break;
  }

  m_shape = m_shapes[newIndex];

  return amIDead;
}

bool ArmyAnt::AdvanceScoutArea()
{
  bool arrived = AdvanceToTargetPosition();
  if (arrived)
  {
    Entity* targetEntity = g_app->m_location->GetEntity(m_targetId);
    if (targetEntity)
    {
      if (targetEntity->m_type == TypeDarwinian)
        m_orders = CollectEntity;
      else
        m_orders = AttackEnemy;
    }
    else
      OrderReturnToBase();
  }
  return false;
}

bool ArmyAnt::AdvanceCollectSpirit()
{
  Spirit* s = nullptr;
  if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
    s = g_app->m_location->m_spirits.GetPointer(m_spiritId);

  if (!s || s->m_state == Spirit::StateDeath || s->m_state == Spirit::StateAttached || s->m_state == Spirit::StateInEgg)
  {
    m_spiritId = -1;
    m_targetFound = false;
    OrderReturnToBase();
    return false;
  }

  m_wayPoint = s->m_pos;
  m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_wayPoint.x, m_wayPoint.z);
  bool arrived = AdvanceToTargetPosition();
  if (arrived)
  {
    s->CollectorArrives();
    OrderReturnToBase();
  }

  return false;
}

bool ArmyAnt::AdvanceCollectEntity()
{
  //
  // Is our entity still here

  Entity* targetEntity = g_app->m_location->GetEntity(m_targetId);
  if (!targetEntity || targetEntity->m_dead)
  {
    m_targetId.SetInvalid();
    m_targetFound = false;
    OrderReturnToBase();
    return false;
  }

  //
  // Make sure he is still capturable

  auto targetDarwinian = static_cast<Darwinian*>(targetEntity);
  if (targetDarwinian->m_state == Darwinian::StateCapturedByAnt)
  {
    m_targetId.SetInvalid();
    m_targetFound = false;
    OrderReturnToBase();
    return false;
  }

  //
  // Go after him

  m_wayPoint = targetEntity->m_pos;
  bool arrived = AdvanceToTargetPosition();
  if (arrived)
  {
    targetDarwinian->AntCapture(m_id);
    OrderReturnToBase();
  }

  return false;
}

bool ArmyAnt::AdvanceAttackEnemy()
{
  //
  // Is our entity still here

  Entity* targetEntity = g_app->m_location->GetEntity(m_targetId);
  if (!targetEntity || targetEntity->m_dead)
  {
    m_targetId.SetInvalid();
    m_targetFound = false;
    OrderReturnToBase();
    return false;
  }

  //
  // Go after him

  m_wayPoint = targetEntity->m_pos;

  if (targetEntity->m_type == TypeEngineer)
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_wayPoint.x, m_wayPoint.z);

  bool arrived = AdvanceToTargetPosition();
  if (arrived)
  {
    targetEntity->ChangeHealth(-1);
    for (int i = 0; i < 3; ++i)
    {
      g_simEventQueue.Push(SimEvent::MakeParticle(m_pos, LegacyVector3(syncsfrand(15.0f), syncsfrand(15.0f) + 15.0f, syncsfrand(15.0f)), SimParticle::TypeMuzzleFlash));
    }
    g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "Attack"));
  }

  return false;
}

bool ArmyAnt::AdvanceReturnToBase()
{
  bool arrived = AdvanceToTargetPosition();
  if (arrived)
  {
    Building* building = g_app->m_location->GetBuilding(m_buildingId);

    if (!building)
    {
      // Our anthill has been destroyed
      bool newBaseFound = SearchForAntHill();
      if (!newBaseFound)
        m_orders = BaseDestroyed;
      return false;
    }
    if (building && building->m_type == Building::TypeAntHill)
    {
      auto antHill = static_cast<AntHill*>(building);
      antHill->m_numAntsInside++;

      // Drop off any spirits we are carrying
      if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
      {
        Spirit* spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
        if (spirit && spirit->m_state == Spirit::StateAttached)
        {
          antHill->m_numSpiritsInside++;
          g_app->m_location->m_spirits.MarkNotUsed(m_spiritId);
        }
      }

      // Any Darwinians being carried are now killed
      Entity* entity = g_app->m_location->GetEntity(m_targetId);
      if (entity && entity->m_type == TypeDarwinian)
      {
        auto darwinian = static_cast<Darwinian*>(entity);
        if (darwinian->m_state == Darwinian::StateCapturedByAnt)
          darwinian->ChangeHealth(darwinian->m_stats[StatHealth] * -2);
      }
    }

    return true;
  }

  return false;
}

bool ArmyAnt::AdvanceBaseDestroyed()
{
  bool arrived = AdvanceToTargetPosition();
  if (arrived)
    SearchForRandomPosition();

  return false;
}

void ArmyAnt::OrderReturnToBase()
{
  Building* building = g_app->m_location->GetBuilding(m_buildingId);
  if (building)
  {
    m_wayPoint = building->m_pos;
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_wayPoint.x, m_wayPoint.z);
    m_orders = ReturnToBase;
  }
  else
  {
    bool newBaseFound = SearchForAntHill();
    if (!newBaseFound)
    {
      m_buildingId = -1;
      m_orders = BaseDestroyed;
    }
  }
}

bool ArmyAnt::SearchForTargets()
{
  bool targetFound = false;

  if (!targetFound)
    targetFound = SearchForSpirits();
  if (!targetFound)
    targetFound = SearchForEnemies();

  return targetFound;
}

bool ArmyAnt::SearchForSpirits()
{
  Spirit* found = nullptr;
  int spiritId = -1;
  float closest = 999999.9f;

  for (int i = 0; i < g_app->m_location->m_spirits.Size(); ++i)
  {
    if (g_app->m_location->m_spirits.ValidIndex(i))
    {
      Spirit* s = g_app->m_location->m_spirits.GetPointer(i);
      float theDist = (s->m_pos - m_pos).Mag();

      if (theDist <= ARMYANT_SEARCHRANGE && theDist < closest && (s->m_state == Spirit::StateBirth || s->m_state == Spirit::StateFloating))
      {
        found = s;
        spiritId = i;
        closest = theDist;
      }
    }
  }

  if (found)
  {
    m_spiritId = spiritId;
    m_orders = CollectSpirit;
    return true;
  }

  return false;
}

bool ArmyAnt::SearchForEnemies()
{
  WorldObjectId enemyId = g_app->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, 0.0f, ARMYANT_SEARCHRANGE, m_id.GetTeamId());
  Entity* enemy = g_app->m_location->GetEntity(enemyId);

  if (enemy && !enemy->m_dead && enemy->m_type != TypeDarwinian)
  {
    m_targetId = enemyId;
    m_orders = AttackEnemy;
    return true;
  }

  return false;
}

bool ArmyAnt::SearchForAntHill()
{
  int buildingId = -1;
  float nearest = 500.0f;

  for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
  {
    if (g_app->m_location->m_buildings.ValidIndex(i))
    {
      Building* building = g_app->m_location->m_buildings[i];

      if (building->m_type == Building::TypeAntHill && g_app->m_location->IsFriend(building->m_id.GetTeamId(), m_id.GetTeamId()))
      {
        float distance = (building->m_pos - m_pos).Mag();
        if (distance < nearest)
        {
          buildingId = building->m_id.GetUniqueId();
          nearest = distance;
        }
      }
    }
  }

  if (buildingId != -1)
  {
    m_buildingId = buildingId;
    OrderReturnToBase();
    return true;
  }

  return false;
}

bool ArmyAnt::SearchForRandomPosition()
{
  float distToSpawnPoint = (m_pos - m_spawnPoint).Mag();
  float chanceOfReturn = (distToSpawnPoint / 400.0f);
  if (chanceOfReturn >= 1.0f || syncfrand(1.0f) <= chanceOfReturn)
  {
    // We have strayed too far from our spawn point
    // So head back there now
    LegacyVector3 returnVector = m_spawnPoint - m_pos;
    returnVector.SetLength(100.0f);
    m_wayPoint = m_pos + returnVector;
  }
  else
  {
    float distance = 100.0f;
    float angle = syncsfrand(2.0f * M_PI);

    m_wayPoint = m_pos + LegacyVector3(sinf(angle) * distance, 0.0f, cosf(angle) * distance);
    m_wayPoint = PushFromObstructions(m_wayPoint);
  }

  m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_wayPoint.x, m_wayPoint.z);
  return true;
}

bool ArmyAnt::AdvanceToTargetPosition()
{
  //
  // Work out where we want to be next

  float speed = m_stats[StatSpeed];
  LegacyVector3 oldPos = m_pos;

  if (m_orders == CollectEntity || m_orders == AttackEnemy)
    speed *= 2.0f;

  LegacyVector3 actualDir = (m_wayPoint - m_pos).Normalise();
  LegacyVector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
  //newPos = PushFromObstructions( newPos );
  newPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(newPos.x, newPos.z);
  LegacyVector3 moved = newPos - oldPos;
  if (moved.Mag() > speed * SERVER_ADVANCE_PERIOD)
    moved.SetLength(speed * SERVER_ADVANCE_PERIOD);
  newPos = m_pos + moved;

  m_pos = newPos;
  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
  m_front = (newPos - oldPos).Normalise();
  m_front.RotateAroundY(syncsfrand(0.2f));

  float distance = (m_pos - m_wayPoint).Mag();
  if (distance < m_vel.Mag() * SERVER_ADVANCE_PERIOD)
  {
    m_vel.Zero();
    return true;
  }

  return false;
}

void ArmyAnt::GetCarryMarker(LegacyVector3& _pos, LegacyVector3& _vel)
{
  LegacyVector3 groundUp = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
  Matrix34 mat(m_front, groundUp, m_pos);
  _pos = m_shape->GetMarkerWorldMatrix(m_carryMarker, mat).pos;
  _vel = m_vel;
}
