#include "pch.h"
#include "math_utils.h"
#include "profiler.h"
#include "resource.h"
#include "GameContext.h"
#include "camera.h"
#include "globals.h"
#include "location.h"
#include "entity_grid.h"
#include "team.h"
#include "GameSimEventQueue.h"
#include "virii.h"
#include "egg.h"

ViriiUnit::ViriiUnit(int teamId, int unitId, int numEntities, const LegacyVector3& _pos)
  : Unit(Entity::TypeVirii, teamId, unitId, numEntities, _pos),
    m_enemiesFound(false),
    m_cameraClose(false) {}

bool ViriiUnit::Advance(int _slice)
{
  float searchRadius = m_radius + VIRII_MAXSEARCHRANGE;

  m_enemiesFound = g_context->m_location->m_entityGrid->AreEnemiesPresent(m_centerPos.x, m_centerPos.z, searchRadius, m_teamId);

  return Unit::Advance(_slice);
}

Virii::Virii()
  : Entity(),
    m_state(StateIdle),
    m_hoverHeight(0.1f),
    m_retargetTimer(-1),
    m_spiritId(-1),
    m_historyTimer(0.0f),
    m_prevPosTimer(0.0f) { m_retargetTimer = syncfrand(2.0f); }

Virii::~Virii() { m_positionHistory.EmptyAndDelete(); }

bool Virii::Advance(Unit* _unit)
{
  m_prevPosTimer -= SERVER_ADVANCE_PERIOD;

  if (m_prevPosTimer <= 0.0f)
  {
    m_prevPos = m_pos;
    m_prevPosTimer = 5.0f;
  }

  //
  // Take damage if we're in the water
  // Actually, don't
  // if( m_pos.y < 0.0f ) ChangeHealth( -1 );

  bool amIDead = Entity::Advance(_unit);

  if (m_dead)
  {
    m_vel.Zero();

    if (m_spiritId != -1)
    {
      if (g_context->m_location->m_spirits.ValidIndex(m_spiritId))
      {
        Spirit* spirit = g_context->m_location->m_spirits.GetPointer(m_spiritId);
        if (spirit && spirit->m_state == Spirit::StateAttached)
          spirit->CollectorDrops();
      }
      m_spiritId = -1;
    }
  }

  if (!m_onGround)
  {
    m_vel.y += -10.0f * SERVER_ADVANCE_PERIOD;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    float groundLevel = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 2.0f;
    if (m_pos.y <= groundLevel)
    {
      m_onGround = true;
      m_vel = g_zeroVector;
    }
  }

  if (!m_dead)
  {
    switch (m_state)
    {
    case StateIdle:
      amIDead = AdvanceIdle();
      break;
    case StateAttacking:
      amIDead = AdvanceAttacking();
      break;
    case StateToSpirit:
      amIDead = AdvanceToSpirit();
      break;
    case StateToEgg:
      amIDead = AdvanceToEgg();
      break;
    }

    if (g_context->m_location->m_spirits.ValidIndex(m_spiritId))
    {
      Spirit* spirit = g_context->m_location->m_spirits.GetPointer(m_spiritId);
      if (spirit && spirit->m_state == Spirit::StateAttached)
      {
        spirit->m_pos = m_pos;
        spirit->m_pos.y += 2.0f;
        spirit->m_vel = m_vel;
      }
    }

    // Next 5 lines by Andrew. Purpose - to prevent virii grinding themselves against
    // the side of obstructions.
    //        LegacyVector3 newPos(PushFromObstructions( m_pos ));
    //		if (newPos.x != m_pos.x || newPos.z != m_pos.z)
    //		{
    //			m_pos = newPos;
    //			if (m_retargetTimer > 0.05f) m_retargetTimer = 0.1f;
    //		}
  }

  if (m_positionHistory.Size() > 0)
  {
    bool recorded = false;

    if (m_state != StateIdle)
    {
      m_historyTimer -= SERVER_ADVANCE_PERIOD;
      if (m_historyTimer <= 0.0f)
      {
        m_historyTimer = 0.5f;
        RecordHistoryPosition(true);
        recorded = true;
      }
    }

    if (!recorded)
    {
      float lastRecordedHeight = m_positionHistory[0]->m_pos.y;
      if (fabs(m_pos.y - lastRecordedHeight) > 3.0f)
        RecordHistoryPosition(false);
    }
  }
  else
    RecordHistoryPosition(true);

  float worldSizeX = g_context->m_location->m_landscape.GetWorldSizeX();
  float worldSizeZ = g_context->m_location->m_landscape.GetWorldSizeZ();
  if (m_pos.x < 0.0f)
    m_pos.x = 0.0f;
  if (m_pos.z < 0.0f)
    m_pos.z = 0.0f;
  if (m_pos.x >= worldSizeX)
    m_pos.x = worldSizeX;
  if (m_pos.z >= worldSizeZ)
    m_pos.z = worldSizeZ;

  return amIDead;
}

void Virii::RecordHistoryPosition(bool _required)
{
  START_PROFILE(g_context->m_profiler, "RecordHistory");

  LegacyVector3 landNormal = g_context->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
  LegacyVector3 prevPos;
  if (m_positionHistory.Size() > 0)
    prevPos = m_positionHistory[0]->m_pos;

  auto history = new ViriiHistory();
  history->m_pos = m_pos;
  history->m_right = (m_pos - prevPos) ^ landNormal;
  history->m_right.Normalise();
  history->m_distance = 0.0f;
  history->m_required = _required;

  if (m_positionHistory.Size() > 0)
  {
    history->m_distance = (m_pos - m_positionHistory[0]->m_pos).Mag();
    history->m_glowDiff = (m_pos - m_positionHistory[0]->m_pos);
    history->m_glowDiff.SetLength(10.0f);
  }

  m_positionHistory.PutDataAtStart(history);

  float totalDistance = 0.0f;
  int removeFrom = -1;

  float tailLength = 175.0f; 
  for (int i = 0; i < m_positionHistory.Size(); ++i)
  {
    ViriiHistory* viriiHistory = m_positionHistory[i];
    totalDistance += viriiHistory->m_distance;

    if (totalDistance > tailLength)
    {
      removeFrom = i;
      break;
    }
  }

  if (removeFrom != -1)
  {
    while (m_positionHistory.ValidIndex(removeFrom))
    {
      ViriiHistory* viriiHistory = m_positionHistory[removeFrom];
      m_positionHistory.RemoveData(removeFrom);
      delete viriiHistory;
    }
  }

  END_PROFILE(g_context->m_profiler, "RecordHistory");
}

bool Virii::AdvanceToTargetPos(const LegacyVector3& _pos)
{
  START_PROFILE(g_context->m_profiler, "AdvanceToTargetPos");

  LegacyVector3 oldPos = m_pos;

  LegacyVector3 distance = _pos - m_pos;
  m_vel = distance;
  m_vel.Normalise();
  m_front = m_vel;
  m_vel *= m_stats[StatSpeed];
  distance.y = 0.0f;

  LegacyVector3 nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
  nextPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z) + m_hoverHeight;
  float currentHeight = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  float nextHeight = g_context->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);

  //
  // Slow us down if we're going up hill
  // Speed up if going down hill

  float factor = 1.0f - (currentHeight - nextHeight) / -5.0f;
  if (factor < 0.1f)
    factor = 0.1f;
  if (factor > 2.0f)
    factor = 2.0f;
  m_vel *= factor;
  nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
  nextPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z) + m_hoverHeight;

  //
  // Are we there?

  bool arrived = false;
  if (distance.MagSquared() < m_vel.MagSquared() * SERVER_ADVANCE_PERIOD * SERVER_ADVANCE_PERIOD)
  {
    nextPos = _pos;
    arrived = true;
  }

  m_pos = nextPos;
  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

  END_PROFILE(g_context->m_profiler, "AdvanceToTargetPos");

  return arrived;
}

bool Virii::AdvanceIdle()
{
  START_PROFILE(g_context->m_profiler, "AdvanceIdle");

  m_retargetTimer -= SERVER_ADVANCE_PERIOD;
  bool foundTarget = false;

  if (m_retargetTimer <= 0.0f)
  {
    m_retargetTimer = 1.0f;

    if (g_context->m_location->m_spirits.ValidIndex(m_spiritId))
      foundTarget = SearchForEggs();

    if (!foundTarget)
      foundTarget = SearchForEnemies();
    if (!foundTarget)
      foundTarget = SearchForSpirits();
    if (!foundTarget)
      foundTarget = SearchForIdleDirection();
  }

  if (m_state == StateIdle)
    AdvanceToTargetPos(m_wayPoint);

  END_PROFILE(g_context->m_profiler, "AdvanceIdle");
  return false;
}

bool Virii::AdvanceAttacking()
{
  START_PROFILE(g_context->m_profiler, "AdvanceAttacking");

  WorldObject* enemy = g_context->m_location->GetEntity(m_enemyId);
  auto entity = static_cast<Entity*>(enemy);

  if (!entity || entity->m_dead)
  {
    m_state = StateIdle;
    END_PROFILE(g_context->m_profiler, "AdvanceAttacking");
    return false;
  }

  //    int enemySpiritId = g_context->m_location->GetSpirit( m_enemyId );
  //    if( enemySpiritId != -1 && entity->m_dead )
  //    {
  //        // Our enemy has died and dropped a spirit
  //        // If we can see an egg, then go for it
  //        if( FindNearbyEgg( enemySpiritId, VIRII_MAXSEARCHRANGE ).IsValid() )
  //        {
  //            m_enemyId.SetInvalid();
  //            m_spiritId = enemySpiritId;
  //            m_state = StateToSpirit;
  //        }
  //        return false;
  //    }

  bool arrived = AdvanceToTargetPos(entity->m_pos);
  if (arrived)
  {
    entity->ChangeHealth(-20);
    for (int i = 0; i < 3; ++i)
    {
      g_simEventQueue.Push(SimEvent::MakeParticle(m_pos, LegacyVector3(syncsfrand(15.0f), syncsfrand(15.0f) + 15.0f, syncsfrand(15.0f)), SimParticle::TypeMuzzleFlash));
    }
    g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "Attack"));
    SearchForEnemies();
  }

  END_PROFILE(g_context->m_profiler, "AdvanceAttacking");
  return false;
}

bool Virii::AdvanceToSpirit()
{
  START_PROFILE(g_context->m_profiler, "AdvanceToSpirit");

  Spirit* s = nullptr;
  if (g_context->m_location->m_spirits.ValidIndex(m_spiritId))
    s = g_context->m_location->m_spirits.GetPointer(m_spiritId);

  if (!s || s->m_state == Spirit::StateDeath || s->m_state == Spirit::StateAttached || s->m_state == Spirit::StateInEgg)
  {
    m_spiritId = -1;
    m_state = StateIdle;
    END_PROFILE(g_context->m_profiler, "AdvanceToSpirit");
    return false;
  }

  Spirit* spirit = g_context->m_location->m_spirits.GetPointer(m_spiritId);
  LegacyVector3 targetPos = spirit->m_pos;
  targetPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);
  bool arrived = AdvanceToTargetPos(targetPos);
  if (arrived)
  {
    spirit->CollectorArrives();
    m_state = StateToEgg;
  }

  END_PROFILE(g_context->m_profiler, "AdvanceToSpirit");
  return false;
}

bool Virii::AdvanceToEgg()
{
  START_PROFILE(g_context->m_profiler, "AdvanceToEgg");
  auto theEgg = static_cast<Egg*>(g_context->m_location->GetEntitySafe(m_eggId, Entity::TypeEgg));

  if (!theEgg || theEgg->m_state == Egg::StateFertilised)
  {
    bool found = SearchForEggs();
    if (!found)
    {
      // We can't find any eggs, so go into holding pattern
      if (g_context->m_location->m_spirits.ValidIndex(m_spiritId))
      {
        Spirit* spirit = g_context->m_location->m_spirits.GetPointer(m_spiritId);
        if (spirit->m_state == Spirit::StateAttached)
          spirit->CollectorDrops();
        m_spiritId = -1;
      }
      m_state = StateIdle;
      END_PROFILE(g_context->m_profiler, "AdvanceToEgg");
      return false;
    }
  }

  if (!g_context->m_location->m_spirits.ValidIndex(m_spiritId))
  {
    m_spiritId = -1;
    m_state = StateIdle;
    END_PROFILE(g_context->m_profiler, "AdvanceToEgg");
    return false;
  }

  //
  // At this point we MUST have found an egg, otherwise we'd have returned by now

  theEgg = static_cast<Egg*>(g_context->m_location->GetEntitySafe(m_eggId, Entity::TypeEgg));
  DEBUG_ASSERT(theEgg);

  bool arrived = AdvanceToTargetPos(theEgg->m_pos);

  if (arrived)
  {
    theEgg->Fertilise(m_spiritId);
    m_spiritId = -1;
    m_eggId.SetInvalid();
  }

  END_PROFILE(g_context->m_profiler, "AdvanceToEgg");
  return false;
}

bool Virii::SearchForEnemies()
{
  START_PROFILE(g_context->m_profiler, "SearchForEnemies");

  auto unit = static_cast<ViriiUnit*>(g_context->m_location->GetUnit(m_id));
  if (unit && !unit->m_enemiesFound)
  {
    END_PROFILE(g_context->m_profiler, "SearchForEnemies");
    return false;
  }

  WorldObjectId bestEnemyId = g_context->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, VIRII_MINSEARCHRANGE, VIRII_MAXSEARCHRANGE,
                                                                            m_id.GetTeamId());
  Entity* enemy = g_context->m_location->GetEntity(bestEnemyId);

  if (enemy && !enemy->m_dead)
  {
    m_enemyId = bestEnemyId;
    m_state = StateAttacking;
    END_PROFILE(g_context->m_profiler, "SearchForEnemies");
    return true;
  }

  END_PROFILE(g_context->m_profiler, "SearchForEnemies");
  return false;
}

bool Virii::SearchForSpirits()
{
  START_PROFILE(g_context->m_profiler, "SearchForSpirits");

  Spirit* found = nullptr;
  int spiritId = -1;
  float closest = 999999.9f;

  for (int i = 0; i < g_context->m_location->m_spirits.Size(); ++i)
  {
    if (g_context->m_location->m_spirits.ValidIndex(i))
    {
      Spirit* s = g_context->m_location->m_spirits.GetPointer(i);
      float theDist = (s->m_pos - m_pos).Mag();

      if (theDist <= VIRII_MAXSEARCHRANGE && theDist < closest && s->NumNearbyEggs() > 0 && (s->m_state == Spirit::StateBirth || s->m_state
        == Spirit::StateFloating))
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
    m_state = StateToSpirit;
    END_PROFILE(g_context->m_profiler, "SearchForSpirits");
    return true;
  }

  END_PROFILE(g_context->m_profiler, "SearchForSpirits");
  return false;
}

WorldObjectId Virii::FindNearbyEgg(int _spiritId, float _autoAccept)
{
  if (!g_context->m_location->m_spirits.ValidIndex(_spiritId))
    return WorldObjectId();

  Spirit* spirit = g_context->m_location->m_spirits.GetPointer(_spiritId);
  if (!spirit)
    return WorldObjectId();

  WorldObjectId* m_nearbyEggs = spirit->GetNearbyEggs();
  int numNearbyEggs = spirit->NumNearbyEggs();

  WorldObjectId eggId;
  float closest = 999999.9f;

  for (int i = 0; i < numNearbyEggs; ++i)
  {
    WorldObjectId thisEggId = m_nearbyEggs[i];
    auto egg = static_cast<Egg*>(g_context->m_location->GetEntitySafe(thisEggId, Entity::TypeEgg));

    if (egg && egg->m_state == Egg::StateDormant)
    {
      float theDist = (egg->m_pos - spirit->m_pos).Mag();
      if (theDist <= _autoAccept)
        return thisEggId;
      if (theDist < closest)
      {
        eggId = thisEggId;
        closest = theDist;
      }
    }
  }

  return eggId;
}

WorldObjectId Virii::FindNearbyEgg(const LegacyVector3& _pos)
{
  int numFound;
  WorldObjectId* ids = g_context->m_location->m_entityGrid->GetFriends(_pos.x, _pos.z, VIRII_MAXSEARCHRANGE, &numFound, m_id.GetTeamId());

  //
  // Build a list of candidates within range

  LList<WorldObjectId> eggIds;

  for (int i = 0; i < numFound; ++i)
  {
    WorldObjectId id = ids[i];
    auto egg = static_cast<Egg*>(g_context->m_location->GetEntitySafe(id, Entity::TypeEgg));

    if (egg && egg->m_state == Egg::StateDormant && egg->m_onGround)
      eggIds.PutData(id);
  }

  //
  // Chose one randomly

  if (eggIds.Size() > 0)
  {
    int chosenIndex = syncfrand(eggIds.Size());
    WorldObjectId* eggId = eggIds.GetPointer(chosenIndex);
    return *eggId;
  }
  return WorldObjectId();
}

bool Virii::SearchForEggs()
{
  START_PROFILE(g_context->m_profiler, "SearchForEggs");

  WorldObjectId eggId = FindNearbyEgg(m_pos);

  if (eggId.IsValid())
  {
    m_eggId = eggId;
    m_state = StateToEgg;
    END_PROFILE(g_context->m_profiler, "SearchForEggs");
    return true;
  }

  END_PROFILE(g_context->m_profiler, "SearchFprEggs");
  return false;
}

bool Virii::SearchForIdleDirection()
{
  START_PROFILE(g_context->m_profiler, "SearchForIdleDir");

  float distToSpawnPoint = (m_pos - m_spawnPoint).Mag();
  float chanceOfReturn = (distToSpawnPoint / m_roamRange);
  if (chanceOfReturn < 0.75f)
    chanceOfReturn = 0.0f;
  if (chanceOfReturn >= 1.0f || syncfrand(1.0f) <= chanceOfReturn)
  {
    // We have strayed too far from our spawn point
    // So head back there now
    LegacyVector3 newDirection = (m_spawnPoint - m_pos);
    newDirection.y = 0;
    if (newDirection.x < -0.5f)
      newDirection.x = -1.0f;
    else if (newDirection.x > 0.5f)
      newDirection.x = 1.0f;
    else
      newDirection.x = 0.0f;
    if (newDirection.z < -0.5f)
      newDirection.z = -1.0f;
    else if (newDirection.z > 0.5f)
      newDirection.z = 1.0f;
    else
      newDirection.z = 0.0f;
    newDirection.SetLength(m_stats[StatSpeed]);
    LegacyVector3 nextPos = m_pos + newDirection * m_retargetTimer;
    nextPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);
    m_wayPoint = nextPos;
    m_state = StateIdle;
    RecordHistoryPosition(true);
    g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "ChangeDirection"));
    END_PROFILE(g_context->m_profiler, "SearchForIdleDir");
    return true;
  }
  int attempts = 0;
  while (attempts < 3)
  {
    ++attempts;

    int x = (static_cast<int>(syncfrand(3.0f)) - 1);
    int z = (static_cast<int>(syncfrand(3.0f)) - 1);
    LegacyVector3 newVel(x, 0.0f, z);
    newVel.Normalise();
    newVel *= m_stats[StatSpeed];

    LegacyVector3 nextPos = m_pos + newVel * m_retargetTimer;
    nextPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);
    if (nextPos.y > 0.0f)
    {
      m_wayPoint = nextPos;
      m_state = StateIdle;
      RecordHistoryPosition(true);
      g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "ChangeDirection"));
      END_PROFILE(g_context->m_profiler, "SearchForIdleDir");
      return true;
    }
  }

  END_PROFILE(g_context->m_profiler, "SearchForIdleDir");
  return false;
}

LegacyVector3 Virii::AdvanceDeadPositionVector(int _index, const LegacyVector3& _pos, float _time)
{
  float distance = 10.0f * _time;
  LegacyVector3 result = _pos;

  switch (_index % 8)
  {
  case 0:
    result += LegacyVector3(-distance, 0, 0);
    break;
  case 1:
    result += LegacyVector3(-distance, 0, distance);
    break;
  case 2:
    result += LegacyVector3(0, 0, distance);
    break;
  case 3:
    result += LegacyVector3(distance, 0, distance);
    break;
  case 4:
    result += LegacyVector3(distance, 0, 0);
    break;
  case 5:
    result += LegacyVector3(distance, 0, -distance);
    break;
  case 6:
    result += LegacyVector3(0, 0, -distance);
    break;
  case 7:
    result += LegacyVector3(-distance, 0, -distance);
    break;
  }

  return result;
}

bool Virii::AdvanceDead()
{
  for (int i = 0; i < m_positionHistory.Size(); ++i)
  {
    LegacyVector3* thisPos = &m_positionHistory[i]->m_pos;
    *thisPos = AdvanceDeadPositionVector(i, *thisPos, SERVER_ADVANCE_PERIOD);
  }
  return true;
}

bool Virii::IsInView()
{
  LegacyVector3 centerPos = m_pos;
  float radiusSqd = 0.0f;

  for (int i = 0; i < m_positionHistory.Size(); ++i)
  {
    LegacyVector3 pos = m_positionHistory[i]->m_pos;
    float distance = (pos - centerPos).MagSquared();
    if (distance > radiusSqd)
      radiusSqd = distance;
  }

  float radius = sqrtf(radiusSqd);
  return g_context->m_camera->SphereInViewFrustum(centerPos, radius);
}
