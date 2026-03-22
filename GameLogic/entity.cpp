#include "pch.h"
#include "entity.h"
#include "GameApp.h"
#include "ai.h"
#include "airstrike.h"
#include "armour.h"
#include "armyant.h"
#include "bridge.h"
#include "camera.h"
#include "centipede.h"
#include "darwinian.h"
#include "egg.h"
#include "engineer.h"
#include "entity_grid.h"
#include "insertion_squad.h"
#include "lander.h"
#include "language_table.h"
#include "laserfence.h"
#include "lasertrooper.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "obstruction_grid.h"
#include "officer.h"
#include "radardish.h"
#include "renderer.h"
#include "resource.h"
#include "routing_system.h"
#include "ShapeStatic.h"
#include "souldestroyer.h"
#include "GameSimEventQueue.h"
#include "spider.h"
#include "sporegenerator.h"
#include "text_stream_readers.h"
#include "triffid.h"
#include "tripod.h"
#include "unit.h"
#include "virii.h"

// ****************************************************************************
//  Class EntityBlueprint
// ****************************************************************************

char* EntityBlueprint::m_names[Entity::NumEntityTypes];
float EntityBlueprint::m_stats[Entity::NumEntityTypes][Entity::NumStats];

void EntityBlueprint::Initialise()
{
  TextReader* theFile = g_app->m_resource->GetTextReader("stats.txt");
  ASSERT_TEXT(theFile && theFile->IsOpen(), "Couldn't open stats.txt");

  int entityIndex = 0;

  while (theFile->ReadLine())
  {
    if (!theFile->TokenAvailable())
      continue;
    ASSERT_TEXT(entityIndex < Entity::NumEntityTypes, "Too many entity blueprints defined");

    m_names[entityIndex] = _strdup(theFile->GetNextToken()); // Process-lifetime allocation — intentional [CI §2.3]
    m_stats[entityIndex][0] = atof(theFile->GetNextToken());
    m_stats[entityIndex][1] = atof(theFile->GetNextToken());
    m_stats[entityIndex][2] = atof(theFile->GetNextToken());

    ++entityIndex;
  }

  delete theFile;
}

char* EntityBlueprint::GetName(unsigned char _type)
{
  DEBUG_ASSERT(_type < Entity::NumEntityTypes);
  return m_names[_type];
}

float EntityBlueprint::GetStat(unsigned char _type, int _stat)
{
  DEBUG_ASSERT(_type < Entity::NumEntityTypes);
  DEBUG_ASSERT(_stat < Entity::NumStats);

  if (_stat == Entity::StatSpeed)
  {
    if (_type != Entity::TypeSpaceInvader)
      return m_stats[_type][_stat] * (1.0f + static_cast<float>(g_app->m_difficultyLevel) / 10.0f);
  }

  return m_stats[_type][_stat];
}

// ****************************************************************************
//  Class Entity
// ****************************************************************************

Entity::Entity()
  : WorldObject(),
    m_formationIndex(-1),
    m_buildingId(-1),
    m_roamRange(0.0f),
    m_dead(false),
    m_justFired(false),
    m_reloading(0.0f),
    m_inWater(-1.0f),
    m_shape(nullptr),
    m_radius(0.0f),
    m_renderDamaged(false),
    m_routeId(-1),
    m_routeWayPointId(-1),
    m_routeTriggerDistance(10.0f) { memset(m_stats, 0, NumStats * sizeof(m_stats[0])); }

Entity::~Entity() {}

void Entity::SetType(unsigned char _type)
{
  m_type = _type;

  for (int i = 0; i < NumStats; ++i)
    m_stats[i] = EntityBlueprint::GetStat(m_type, i);

  m_reloading = syncfrand(m_stats[StatRate]);
}

void Entity::ChangeHealth(int amount)
{
  if (!m_dead)
  {
    if (amount < 0)
      g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "LoseHealth"));

    if (m_stats[StatHealth] + amount <= 0)
    {
      m_stats[StatHealth] = 100;
      m_dead = true;
      g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "Die"));
      g_app->m_location->SpawnSpirit(m_pos, m_vel * 0.5f, m_id.GetTeamId(), m_id);
    }
    else if (m_stats[StatHealth] + amount > 255)
      m_stats[StatHealth] = 255;
    else
      m_stats[StatHealth] += amount;
  }
}

void Entity::Attack(const LegacyVector3& pos)
{
  if (m_reloading == 0.0f)
  {
    LegacyVector3 toPos(pos.x + syncsfrand(15.0f), pos.y, pos.z + syncsfrand(15.0f));
    LegacyVector3 fromPos = m_pos;
    fromPos.y += 2.0f;
    LegacyVector3 velocity = (toPos - fromPos).SetLength(200.0f);
    g_app->m_location->FireLaser(fromPos, velocity, m_id.GetTeamId());

    m_reloading = m_stats[StatRate];
    m_justFired = true;

    g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "Attack"));
  }
}

bool Entity::AdvanceDead(Unit* _unit)
{
  int newHealth = m_stats[StatHealth];
  if (m_onGround)
    newHealth -= 40 * SERVER_ADVANCE_PERIOD;
  else
    newHealth -= 20 * SERVER_ADVANCE_PERIOD;

  if (newHealth <= 0)
  {
    m_stats[StatHealth] = 0;
    return true;
  }
  m_stats[StatHealth] = newHealth;

  return false;
}

int Entity::EnterTeleports(int _requiredId)
{
  LList<int>* buildings = g_app->m_location->m_obstructionGrid->GetBuildings(m_pos.x, m_pos.z);

  for (int i = 0; i < buildings->Size(); ++i)
  {
    int buildingId = buildings->GetData(i);
    if (_requiredId != -1 && _requiredId != buildingId)
    {
      // We are only permitted to enter building with id _requiredId
      continue;
    }

    Building* building = g_app->m_location->GetBuilding(buildingId);
    DEBUG_ASSERT(building);

    if (building->m_type == Building::TypeRadarDish)
    {
      auto radarDish = static_cast<RadarDish*>(building);
      LegacyVector3 entrancePos, entranceFront;
      radarDish->GetEntrance(entrancePos, entranceFront);
      float range = (m_pos - entrancePos).Mag();
      if (radarDish->ReadyToSend() && range < 15.0f && m_front * entranceFront < 0.0f)
      {
        WorldObjectId id(m_id);
        radarDish->EnterTeleport(id);
        g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "EnterTeleport"));
        return buildingId;
      }
    }
    else if (building->m_type == Building::TypeBridge)
    {
      auto bridge = static_cast<Bridge*>(building);
      LegacyVector3 entrancePos, entranceFront;
      if (bridge->GetEntrance(entrancePos, entranceFront))
      {
        float range = (m_pos - bridge->m_pos).Mag();
        if (bridge->ReadyToSend() && range < 25.0f && m_front * entranceFront < 0.0f)
        {
          WorldObjectId id(m_id);
          bridge->EnterTeleport(id);
          g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "EnterTeleport"));
          return buildingId;
        }
      }
    }
  }

  return -1;
}

void Entity::AdvanceInAir(Unit* _unit)
{
  m_vel += LegacyVector3(0, -15.0, 0) * SERVER_ADVANCE_PERIOD;
  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  if (!m_dead)
  {
    float groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 0.1f;
    if (m_pos.y <= groundLevel)
    {
      m_onGround = true;
      m_vel = g_zeroVector;
      m_pos.y = groundLevel + 0.1f;
    }
    if (m_pos.y <= 0.0f /*seaLevel*/)
    {
      m_inWater = 0;
      m_vel = g_zeroVector;
    }
  }
}

void Entity::AdvanceInWater(Unit* _unit)
{
  m_inWater += SERVER_ADVANCE_PERIOD;
  if (m_inWater > 10.0f /* && m_type != TypeInsertionSquadie */)
    ChangeHealth(-500);

  LegacyVector3 targetPos;
  if (_unit)
  {
    targetPos = _unit->GetWayPoint();
    targetPos.y = 0.0f;
    LegacyVector3 offset = _unit->GetFormationOffset(Unit::FormationRectangle, m_id.GetIndex());
    targetPos += offset;
  }

  if (targetPos != g_zeroVector)
  {
    LegacyVector3 distance = targetPos - m_pos;
    distance.y = 0.0f;
    m_vel = distance;
    m_vel.Normalise();
    m_front = m_vel;
    m_vel *= m_stats[StatSpeed] * 0.3f;

    if (distance.Mag() < m_vel.Mag() * SERVER_ADVANCE_PERIOD)
      m_vel = distance / SERVER_ADVANCE_PERIOD;
  }

  m_vel.SetLength(5.0f);
  m_pos += m_vel * SERVER_ADVANCE_PERIOD;
  m_pos.y = 0.0f - sinf(m_inWater * 3.0f) - 4.0f;

  float groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  if (groundLevel > 0.0f)
    m_inWater = -1;
}

void Entity::Begin()
{
  g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "Create"));

  if (m_shape)
  {
    m_centerPos = m_shape->CalculateCenter(g_identityMatrix34);
    m_radius = m_shape->CalculateRadius(g_identityMatrix34, m_centerPos);
  }
}

bool Entity::Advance(Unit* _unit)
{
  if (!m_enabled)
    return false;

  if (m_dead)
  {
    bool amIDead = AdvanceDead(_unit);
    if (amIDead)
      return true;
  }

  //m_pos = PushFromObstructions( m_pos );
  //m_pos = PushFromEachOther( m_pos );

  if (m_reloading > 0.0f)
  {
    m_reloading -= SERVER_ADVANCE_PERIOD;
    if (m_reloading < 0.0f)
      m_reloading = 0.0f;
  }

  float healthFraction = static_cast<float>(m_stats[StatHealth]) / EntityBlueprint::GetStat(m_type, StatHealth);
  float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
  m_renderDamaged = (frand(0.75f) * (1.0f - fabs(sinf(timeIndex)) * 0.8f) > healthFraction);

  if (m_inWater != -1)
    AdvanceInWater(_unit);

  float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
  float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
  if (m_pos.x < 0.0f)
    m_pos.x = 0.0f;
  if (m_pos.z < 0.0f)
    m_pos.z = 0.0f;
  if (m_pos.x >= worldSizeX)
    m_pos.x = worldSizeX;
  if (m_pos.z >= worldSizeZ)
    m_pos.z = worldSizeZ;

  if (_unit && !m_dead)
    _unit->UpdateEntityPosition(m_pos, m_radius);

  if (m_routeId != -1)
    FollowRoute();

  return false;
}

bool Entity::IsInView() { return true; }

LegacyVector3 Entity::PushFromEachOther(const LegacyVector3& _pos)
{
  LegacyVector3 result = _pos;

  int numFound;
  WorldObjectId* neighbours = g_app->m_location->m_entityGrid->GetNeighbours(_pos.x, _pos.z, 2.0f, &numFound);

  for (int i = 0; i < numFound; ++i)
  {
    WorldObjectId id = neighbours[i];
    if (!(id == m_id))
    {
      WorldObject* obj = g_app->m_location->GetEntity(id);
      //            float distance = (obj->m_pos - thisPos).Mag();
      //            while( distance < 1.0f )
      //            {
      //                LegacyVector3 pushForce = (obj->m_pos - thisPos).Normalise() * 0.1f;
      //                if( obj->m_pos == thisPos ) pushForce = LegacyVector3(0.0f,0.0f,1.0f);
      //                thisPos -= pushForce;
      //                distance = (obj->m_pos - thisPos).Mag();
      //            }

      LegacyVector3 diff = (_pos - obj->m_pos);
      float force = 0.1f / diff.Mag();
      diff.Normalise();

      LegacyVector3 thisDiff = (diff * force);
      result += thisDiff;
    }
  }

  result.y = g_app->m_location->m_landscape.m_heightMap->GetValue(result.x, result.z);
  return result;
}

LegacyVector3 Entity::PushFromCliffs(const LegacyVector3& pos, const LegacyVector3& oldPos)
{
  LegacyVector3 horiz = (pos - oldPos);
  horiz.y = 0.0f;
  float horizDistance = horiz.Mag();
  float gradient = (pos.y - oldPos.y) / horizDistance;

  LegacyVector3 result = pos;

  if (gradient > 1.0f)
  {
    float distance = 0.0f;
    while (distance < 50.0f)
    {
      float angle = distance * 2.0f * M_PI;
      LegacyVector3 offset(cosf(angle) * distance, 0.0f, sinf(angle) * distance);
      LegacyVector3 newPos = result + offset;
      newPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(newPos.x, newPos.z);
      horiz = (newPos - oldPos);
      horiz.y = 0.0f;
      horizDistance = horiz.Mag();
      float newGradient = (newPos.y - oldPos.y) / horizDistance;
      if (newGradient < 1.0f)
      {
        result = newPos;
        break;
      }
      distance += 0.2f;
    }
  }

  return result;
}

LegacyVector3 Entity::PushFromObstructions(const LegacyVector3& pos, bool killem)
{
  LegacyVector3 result = pos;
  if (m_onGround)
    result.y = g_app->m_location->m_landscape.m_heightMap->GetValue(result.x, result.z);

  Matrix34 transform(m_front, g_upVector, result);

  //
  // Push from Water

  if (result.y <= 1.0f)
  {
    float pushAngle = syncsfrand(1.0f);
    float distance = 0.0f;
    while (distance < 50.0f)
    {
      float angle = distance * pushAngle * M_PI;
      LegacyVector3 offset(cosf(angle) * distance, 0.0f, sinf(angle) * distance);
      LegacyVector3 newPos = result + offset;
      float height = g_app->m_location->m_landscape.m_heightMap->GetValue(newPos.x, newPos.z);
      if (height > 1.0f)
      {
        result = newPos;
        result.y = height;
        break;
      }
      distance += 1.0f;
    }
  }

  //
  // Push from buildings

  LList<int>* buildings = g_app->m_location->m_obstructionGrid->GetBuildings(result.x, result.z);

  for (int b = 0; b < buildings->Size(); ++b)
  {
    int buildingId = buildings->GetData(b);
    Building* building = g_app->m_location->GetBuilding(buildingId);
    if (building)
    {
      bool hit = false;
      if (m_shape && building->DoesShapeHit(m_shape, transform))
        hit = true;
      if (!m_shape && building->DoesSphereHit(result, 1.0f))
        hit = true;

      if (hit)
      {
        if (building->m_type == Building::TypeLaserFence && killem && static_cast<LaserFence*>(building)->IsEnabled())
        {
          ChangeHealth(-9999);
          static_cast<LaserFence*>(building)->Electrocute(m_pos);
        }
        else
        {
          LegacyVector3 pushForce = (building->m_pos - result).SetLength(2.0f);
          while (building->DoesSphereHit(result, 1.0f))
          {
            result -= pushForce;
            //result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
          }
        }
      }
    }
  }

  return result;
}

static float s_nearPlaneStart;

void Entity::BeginRenderShadow()
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/glow.bmp"));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glDisable(GL_CULL_FACE);
  glDepthMask(false);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  glColor4f(0.6f, 0.6f, 0.6f, 0.0f);

  s_nearPlaneStart = g_app->m_renderer->GetNearPlane();
  g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart * 1.05f, g_app->m_renderer->GetFarPlane());
}

void Entity::EndRenderShadow()
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);

  glDepthMask(true);
  glEnable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);

  g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart, g_app->m_renderer->GetFarPlane());
}

void Entity::RenderShadow(const LegacyVector3& _pos, float _size)
{
  LegacyVector3 shadowPos = _pos;
  auto shadowR = LegacyVector3(_size, 0, 0);
  auto shadowU = LegacyVector3(0, 0, _size);

  LegacyVector3 posA = shadowPos - shadowR - shadowU;
  LegacyVector3 posB = shadowPos + shadowR - shadowU;
  LegacyVector3 posC = shadowPos + shadowR + shadowU;
  LegacyVector3 posD = shadowPos - shadowR + shadowU;

  posA.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posA.x, posA.z) + 0.9f;
  posB.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posB.x, posB.z) + 0.9f;
  posC.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posC.x, posC.z) + 0.9f;
  posD.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posD.x, posD.z) + 0.9f;

  posA.y = max(posA.y, 1.0f);
  posB.y = max(posB.y, 1.0f);
  posC.y = max(posC.y, 1.0f);
  posD.y = max(posD.y, 1.0f);

  if (posA.y > _pos.y && posB.y > _pos.y && posC.y > _pos.y && posD.y > _pos.y)
  {
    // The object casting the shadow is beneath the ground
    return;
  }

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3fv(posA.GetData());
  glTexCoord2f(1.0f, 0.0f);
  glVertex3fv(posB.GetData());
  glTexCoord2f(1.0f, 1.0f);
  glVertex3fv(posC.GetData());
  glTexCoord2f(0.0f, 1.0f);
  glVertex3fv(posD.GetData());
  glEnd();
}

Entity* Entity::NewEntity(int _troopType)
{
  Entity* entity = nullptr;

  switch (_troopType)
  {
  case TypeLaserTroop:
    entity = new LaserTrooper();
    break;
  case TypeInsertionSquadie:
    entity = new Squadie();
    break;
  case TypeEngineer:
    entity = new Engineer();
    break;
  case TypeVirii:
    entity = new Virii();
    break;
  case TypeEgg:
    entity = new Egg();
    break;
  case TypeSporeGenerator:
    entity = new SporeGenerator();
    break;
  case TypeLander:
    entity = new Lander();
    break;
  case TypeTripod:
    entity = new Tripod();
    break;
  case TypeCentipede:
    entity = new Centipede();
    break;
  case TypeSpaceInvader:
    entity = new SpaceInvader();
    break;
  case TypeSpider:
    entity = new Spider();
    break;
  case TypeDarwinian:
    entity = new Darwinian();
    break;
  case TypeOfficer:
    entity = new Officer();
    break;
  case TypeArmyAnt:
    entity = new ArmyAnt();
    break;
  case TypeArmour:
    entity = new Armour();
    break;
  case TypeSoulDestroyer:
    entity = new SoulDestroyer();
    break;
  case TypeTriffidEgg:
    entity = new TriffidEgg();
    break;
  case TypeAI:
    entity = new AI();
    break;

  default: DEBUG_ASSERT(false);
  }

  entity->m_id.GenerateUniqueId();

  return entity;
}

int Entity::GetTypeId(const char* _typeName)
{
  for (int i = 0; i < NumEntityTypes; ++i)
  {
    if (_stricmp(_typeName, GetTypeName(i)) == 0)
      return i;
  }
  return -1;
}

const char* Entity::GetTypeName(int _troopType)
{
  static const char* typeNames[NumEntityTypes] = {
    "InvalidType", "LaserTroop", "Engineer", "Virii", "Squadie", "Egg", "SporeGenerator", "Lander", "Tripod", "Centipede", "SpaceInvader",
    "Spider", "Darwinian", "Officer", "ArmyAnt", "Armour", "SoulDestroyer", "TriffidEgg", "AI"
  };

  DEBUG_ASSERT(_troopType >= 0 && _troopType < NumEntityTypes);
  return typeNames[_troopType];
}

const char* Entity::GetTypeNameTranslated(int _troopType)
{
  const char* typeName = GetTypeName(_troopType);

  char stringId[256];
  snprintf(stringId, sizeof(stringId), "entityname_%s", typeName);

  if (ISLANGUAGEPHRASE(stringId))
    return LANGUAGEPHRASE(stringId);
  return typeName;
}

bool Entity::RayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir)
{
  if (m_shape)
  {
    RayPackage package(_rayStart, _rayDir);
    Matrix34 mat(m_front, g_upVector, m_pos);
    return m_shape->RayHit(&package, mat);
  }
  return RaySphereIntersection(_rayStart, _rayDir, m_pos, 5.0f);
}

void Entity::DirectControl(const TeamControls& _teamControls) {}

LegacyVector3 Entity::GetCameraFocusPoint() { return m_pos + m_vel; }

void Entity::SetWaypoint(const LegacyVector3 _waypoint) {}

void Entity::FollowRoute()
{
  DEBUG_ASSERT(m_routeId != -1);
  Route* route = g_app->m_location->m_levelFile->GetRoute(m_routeId);
  DEBUG_ASSERT(route);

  if (m_routeWayPointId == -1)
    m_routeWayPointId = 0;

  WayPoint* waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
  SetWaypoint(waypoint->GetPos());
  LegacyVector3 targetVect = waypoint->GetPos() - m_pos;

  m_spawnPoint = waypoint->GetPos();

  if (waypoint->m_type != WayPoint::TypeBuilding && targetVect.Mag() < m_routeTriggerDistance)
  {
    m_routeWayPointId++;
    if (m_routeWayPointId >= route->m_wayPoints.Size())
    {
      m_routeWayPointId = -1;
      m_routeId = -1;
    }
  }

  //
  // If its a building instead of a 3D pos, this unit will never
  // get to the next waypoint.  A new unit is created when the unit
  // enters the teleport, and taht new unit will automatically
  // continue towards the next waypoint instead.
  //
}
