#include "pch.h"
#include "building.h"
#include "GameContext.h"
#include "ai.h"
#include "anthill.h"
#include "blueprintstore.h"
#include "bridge.h"
#include "camera.h"
#include "cave.h"
#include "clienttoserver.h"
#include "constructionyard.h"
#include "controltower.h"
#include "darwinian.h"
#include "entity_grid.h"
#include "GameSimEventQueue.h"
#include "factory.h"
#include "feedingtube.h"
#include "file_writer.h"
#include "generator.h"
#include "generichub.h"
#include "globals.h"
#include "goddish.h"
#include "gunturret.h"
#include "incubator.h"
#include "language_table.h"
#include "laserfence.h"
#include "library.h"
#include "location.h"
#include "math_utils.h"
#include "matrix34.h"
#include "mine.h"
#include "obstruction_grid.h"
#include "powerstation.h"
#include "preferences.h"
#include "profiler.h"
#include "radardish.h"
#include "researchitem.h"
#include "resource.h"
#include "rocket.h"
#include "safearea.h"
#include "scripttrigger.h"
#include "ShapeStatic.h"
#include "spam.h"
#include "spawnpoint.h"
#include "spiritreceiver.h"
#include "staticshape.h"
#include "switch.h"
#include "team.h"
#include "text_stream_readers.h"
#include "tree.h"
#include "triffid.h"
#include "trunkport.h"
#include "upgrade_port.h"
#include "wall.h"

ShapeStatic* Building::s_controlPad = nullptr;
ShapeMarkerData* Building::s_controlPadStatus = nullptr;
static std::once_flag s_controlPadOnce;

Building::Building()
  : m_front(1, 0, 0),
    m_timeOfDeath(-1.0f),
    m_dynamic(false),
    m_isGlobal(false),
    m_radius(13.0f),
    m_destroyed(false),
    m_shape(nullptr)
{
  std::call_once(s_controlPadOnce, []()
  {
    s_controlPad = Resource::GetShapeStatic("controlpad.shp");
    DEBUG_ASSERT(s_controlPad);

    s_controlPadStatus = s_controlPad->GetMarkerData("MarkerStatus");
    DEBUG_ASSERT(s_controlPadStatus);
  });

  m_id.SetTeamId(1);

  m_up = g_upVector;
}

void Building::Initialise(Building* _template)
{
  m_id = _template->m_id;
  m_pos = _template->m_pos;
  m_pos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  m_front = _template->m_front;
  m_up = _template->m_up;
  m_dynamic = _template->m_dynamic;
  m_isGlobal = _template->m_isGlobal;
  m_destroyed = _template->m_destroyed;

  if (m_shape)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    m_centerPos = m_shape->CalculateCenter(mat);
    m_radius = m_shape->CalculateRadius(mat, m_centerPos);

    SetShapeLights(m_shape->m_rootFragment);
    SetShapePorts(m_shape->m_rootFragment);
  }
  else
  {
    m_centerPos = m_pos;
    m_radius = 13.0f;
  }

  GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_requestedLocationId);
  if (gb)
    m_id.SetTeamId(gb->m_teamId);

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Create"));
}

void Building::SetDetail([[maybe_unused]] int _detail)
{
  m_pos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

  if (m_shape)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    m_centerPos = m_shape->CalculateCenter(mat);
    m_radius = m_shape->CalculateRadius(mat, m_centerPos);

    m_ports.EmptyAndDelete();
    SetShapePorts(m_shape->m_rootFragment);
  }
  else
  {
    m_centerPos = m_pos;
    m_radius = 13.0f;
  }
}

bool Building::Advance()
{
  if (m_destroyed)
    return true;

  EvaluatePorts();
  return false;
}

void Building::SetShape(ShapeStatic* _shape) { m_shape = _shape; }

void Building::SetShapeLights(const ShapeFragmentData* _fragment)
{
  //
  // Enter all lights from this fragment

  int i;

  for (i = 0; i < _fragment->m_childMarkers.Size(); ++i)
  {
    ShapeMarkerData* marker = _fragment->m_childMarkers.GetData(i);
    if (strstr(marker->m_name, "MarkerLight"))
      m_lights.PutData(marker);
  }

  //
  // Recurse to all child fragments

  for (i = 0; i < _fragment->m_childFragments.Size(); ++i)
  {
    ShapeFragmentData* fragment = _fragment->m_childFragments.GetData(i);
    SetShapeLights(fragment);
  }
}

void Building::SetShapePorts(const ShapeFragmentData* _fragment)
{
  //
  // Enter all ports from this fragment

  int i;

  Matrix34 buildingMat(m_front, m_up, m_pos);

  for (i = 0; i < _fragment->m_childMarkers.Size(); ++i)
  {
    ShapeMarkerData* marker = _fragment->m_childMarkers.GetData(i);
    if (strstr(marker->m_name, "MarkerPort"))
    {
      auto port = new BuildingPort();
      port->m_marker = marker;
      port->m_mat = m_shape->GetMarkerWorldMatrix(marker, buildingMat);
      port->m_mat.pos = PushFromBuilding(port->m_mat.pos, 5.0f);
      port->m_mat.pos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(port->m_mat.pos.x, port->m_mat.pos.z);

      for (int t = 0; t < NUM_TEAMS; ++t)
        port->m_counter[t] = 0;

      m_ports.PutData(port);
    }
  }

  //
  // Recurse to all child fragments

  for (i = 0; i < _fragment->m_childFragments.Size(); ++i)
  {
    ShapeFragmentData* fragment = _fragment->m_childFragments.GetData(i);
    SetShapePorts(fragment);
  }
}

void Building::Reprogram([[maybe_unused]] float _complete) {}

void Building::ReprogramComplete()
{
  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "ReprogramComplete"));

  GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
  if (gb)
    gb->m_online = !gb->m_online;

  g_context->m_globalWorld->EvaluateEvents();
}

void Building::SetTeamId(int _teamId)
{
  m_id.SetTeamId(_teamId);

  GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
  if (gb)
    gb->m_teamId = _teamId;

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "ChangeTeam"));
}

LegacyVector3 Building::PushFromBuilding(const LegacyVector3& pos, float _radius)
{
  START_PROFILE(g_context->m_profiler, "PushFromBuilding");

  LegacyVector3 result = pos;

  bool hit = false;
  if (DoesSphereHit(result, _radius))
    hit = true;

  if (hit)
  {
    LegacyVector3 pushForce = (m_pos - result).SetLength(2.0f);
    while (DoesSphereHit(result, _radius)) { result -= pushForce; }
  }

  END_PROFILE(g_context->m_profiler, "PushFromBuilding");

  return result;
}

bool Building::IsInView() { return (g_context->m_camera->SphereInViewFrustum(m_centerPos, m_radius)); }

bool Building::PerformDepthSort([[maybe_unused]] LegacyVector3& _centerPos) { return false; }

void Building::EvaluatePorts()
{
  for (int i = 0; i < GetNumPorts(); ++i)
  {
    BuildingPort* port = m_ports[i];
    //port->m_mat = port->m_marker->GetWorldMatrix(buildingMat);
    port->m_occupant.SetInvalid();

    //
    // Look for a valid Darwinian near the port

    int numFound;
    if (g_context->m_location->m_entityGrid)
    {
      WorldObjectId* ids = g_context->m_location->m_entityGrid->GetNeighbours(port->m_mat.pos.x, port->m_mat.pos.z, 5.0f, &numFound);
      for (int j = 0; j < numFound; ++j)
      {
        WorldObjectId id = ids[j];
        Entity* entity = g_context->m_location->GetEntity(id);
        if (entity && entity->m_type == Entity::TypeDarwinian)
        {
          auto darwinian = static_cast<Darwinian*>(entity);
          if (darwinian->m_state == Darwinian::StateOperatingPort)
          {
            port->m_occupant = id;
            break;
          }
        }
      }
    }

    //
    // Update the operation counter

    for (int t = 0; t < NUM_TEAMS; ++t)
    {
      if (port->m_occupant.IsValid())
        port->m_counter[t] = 0;
      else
      {
        port->m_counter[t] -= 4;
        port->m_counter[t] = std::max(port->m_counter[t], 0);
      }
    }
  }
}

void Building::Damage([[maybe_unused]] float _damage) { g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Damage")); }

void Building::Destroy(float _intensity)
{
  m_destroyed = true;

  Matrix34 mat(m_front, g_upVector, m_pos);
  for (int i = 0; i < 3; ++i)
    g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, mat, 1.0f));
  g_context->m_location->Bang(m_pos, _intensity, _intensity / 4.0f);

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Explode"));

  for (int i = 0; i < static_cast<int>(_intensity / 4.0f); ++i)
  {
    LegacyVector3 vel(0.0f, 0.0f, 0.0f);
    vel.x += syncsfrand(100.0f);
    vel.y += syncsfrand(100.0f);
    vel.z += syncsfrand(100.0f);
    g_simEventQueue.Push(SimEvent::MakeParticle(m_pos, vel, SimParticle::TypeExplosionCore, 100.0f));
  }
}

bool Building::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, [[maybe_unused]] LegacyVector3* _pos,
                          [[maybe_unused]] LegacyVector3* norm)
{
  if (m_shape)
  {
    RayPackage ray(_rayStart, _rayDir, _rayLen);
    Matrix34 transform(m_front, m_up, m_pos);
    return m_shape->RayHit(&ray, transform, true);
  }
  return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
}

bool Building::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  if (m_shape)
  {
    SpherePackage sphere(_pos, _radius);
    Matrix34 transform(m_front, m_up, m_pos);
    return m_shape->SphereHit(&sphere, transform);
  }
  float distance = (_pos - m_pos).Mag();
  return (distance <= _radius + m_radius);
}

bool Building::DoesShapeHit(ShapeStatic* _shape, Matrix34 _theTransform)
{
  if (m_shape)
  {
    Matrix34 ourTransform(m_front, m_up, m_pos);
    //return m_shape->ShapeHit( _shape, _theTransform, ourTransform, true );
    return _shape->ShapeHit(m_shape, ourTransform, _theTransform, true);
  }
  SpherePackage package(m_pos, m_radius);
  return _shape->SphereHit(&package, _theTransform, true);
}

int Building::GetBuildingLink() { return -1; }

void Building::SetBuildingLink([[maybe_unused]] int _buildingId) {}

int Building::GetNumPorts() { return m_ports.Size(); }

int Building::GetNumPortsOccupied()
{
  int result = 0;
  for (int i = 0; i < GetNumPorts(); ++i)
    if (GetPortOccupant(i).IsValid())
      result++;
  return result;
}

WorldObjectId Building::GetPortOccupant(int _portId)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    return port->m_occupant;
  }

  return WorldObjectId();
}

bool Building::GetPortPosition(int _portId, LegacyVector3& _pos, LegacyVector3& _front)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    _pos = port->m_mat.pos;
    _front = port->m_mat.f;
    return true;
  }

  return false;
}

void Building::OperatePort(int _portId, int _teamId)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    port->m_counter[_teamId]++;
    port->m_counter[_teamId] = std::min(port->m_counter[_teamId], 50);
  }
}

int Building::GetPortOperatorCount(int _portId, int _teamId)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    return port->m_counter[_teamId];
  }

  return -1;
}

const char* Building::GetObjectiveCounter() { return ""; }

void Building::Read(TextReader* _in, bool _dynamic)
{
  char* word;

  int buildingId;
  int teamId;

  word = _in->GetNextToken();
  buildingId = atoi(word);
  word = _in->GetNextToken();
  m_pos.x = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  m_pos.z = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  teamId = atoi(word);
  word = _in->GetNextToken();
  m_front.x = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  m_front.z = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  m_isGlobal = static_cast<bool>(atoi(word));

  m_front.Normalise();
  m_id.Set(teamId, UNIT_BUILDINGS, -1, buildingId);
  m_dynamic = _dynamic;
}

void Building::Write(FileWriter* _out)
{
  _out->printf("\t%-20s", GetTypeName(m_type));

  _out->printf("%-8d", m_id.GetUniqueId());
  _out->printf("%-8.2f", m_pos.x);
  _out->printf("%-8.2f", m_pos.z);
  _out->printf("%-8d", m_id.GetTeamId());
  _out->printf("%-8.2f", m_front.x);
  _out->printf("%-8.2f", m_front.z);
  _out->printf("%-8d", m_isGlobal);
}

Building* Building::CreateBuilding(const char* _name)
{
  for (int i = 0; i < NumBuildingTypes; ++i)
  {
    if (_stricmp(_name, GetTypeName(i)) == 0)
      return CreateBuilding(i);
  }

  //DEBUG_ASSERT(false);
  return nullptr;
}

Building* Building::CreateBuilding(int _type)
{
  Building* building = nullptr;

  switch (_type)
  {
  case TypeFactory:
    building = new Factory();
    break;
  case TypeCave:
    building = new Cave();
    break;
  case TypeRadarDish:
    building = new RadarDish();
    break;
  case TypeLaserFence:
    building = new LaserFence();
    break;
  case TypeControlTower:
    building = new ControlTower();
    break;
  case TypeGunTurret:
    building = new GunTurret();
    break;
  case TypeBridge:
    building = new Bridge();
    break;
  case TypePowerstation:
    building = new Powerstation();
    break;
  case TypeTree:
    building = new Tree();
    break;
  case TypeWall:
    building = new Wall();
    break;
  case TypeTrunkPort:
    building = new TrunkPort();
    break;
  case TypeResearchItem:
    building = new ResearchItem();
    break;
  case TypeLibrary:
    building = new Library();
    break;
  case TypeGenerator:
    building = new Generator();
    break;
  case TypePylon:
    building = new Pylon();
    break;
  case TypePylonStart:
    building = new PylonStart();
    break;
  case TypePylonEnd:
    building = new PylonEnd();
    break;
  case TypeSolarPanel:
    building = new SolarPanel();
    break;
  case TypeTrackLink:
    building = new TrackLink();
    break;
  case TypeTrackJunction:
    building = new TrackJunction();
    break;
  case TypeTrackStart:
    building = new TrackStart();
    break;
  case TypeTrackEnd:
    building = new TrackEnd();
    break;
  case TypeRefinery:
    building = new Refinery();
    break;
  case TypeMine:
    building = new Mine();
    break;
  case TypeYard:
    building = new ConstructionYard();
    break;
  case TypeDisplayScreen:
    building = new DisplayScreen();
    break;
  case TypeUpgradePort:
    building = new UpgradePort;
    break;
  case TypePrimaryUpgradePort:
    building = new PrimaryUpgradePort();
    break;
  case TypeIncubator:
    building = new Incubator();
    break;
  case TypeAntHill:
    building = new AntHill();
    break;
  case TypeSafeArea:
    building = new SafeArea();
    break;
  case TypeTriffid:
    building = new Triffid();
    break;
  case TypeSpiritReceiver:
    building = new SpiritReceiver();
    break;
  case TypeReceiverLink:
    building = new ReceiverLink();
    break;
  case TypeReceiverSpiritSpawner:
    building = new ReceiverSpiritSpawner();
    break;
  case TypeSpiritProcessor:
    building = new SpiritProcessor();
    break;
  case TypeSpawnPoint:
    building = new SpawnPoint();
    break;
  case TypeSpawnPopulationLock:
    building = new SpawnPopulationLock();
    break;
  case TypeSpawnPointMaster:
    building = new MasterSpawnPoint();
    break;
  case TypeSpawnLink:
    building = new SpawnLink();
    break;
  case TypeBlueprintStore:
    building = new BlueprintStore();
    break;
  case TypeBlueprintConsole:
    building = new BlueprintConsole();
    break;
  case TypeBlueprintRelay:
    building = new BlueprintRelay();
    break;
  case TypeAITarget:
    building = new AITarget();
    break;
  case TypeAISpawnPoint:
    building = new AISpawnPoint();
    break;
  case TypeScriptTrigger:
    building = new ScriptTrigger();
    break;
  case TypeSpam:
    building = new Spam();
    break;
  case TypeGodDish:
    building = new GodDish();
    break;
  case TypeStaticShape:
    building = new StaticShape();
    break;
  case TypeFuelGenerator:
    building = new FuelGenerator();
    break;
  case TypeFuelPipe:
    building = new FuelPipe();
    break;
  case TypeFuelStation:
    building = new FuelStation();
    break;
  case TypeEscapeRocket:
    building = new EscapeRocket();
    break;
  case TypeFenceSwitch:
    building = new FenceSwitch();
    break;
  case TypeDynamicHub:
    building = new DynamicHub();
    break;
  case TypeDynamicNode:
    building = new DynamicNode();
    break;
  case TypeFeedingTube:
    building = new FeedingTube();
    break;
  }

  if (_type == TypeRadarDish || _type == TypeControlTower || _type == TypeTrunkPort || _type == TypeIncubator || _type == TypeDynamicHub ||
    _type == TypeFenceSwitch)
    building->m_isGlobal = true;

  return building;
}

int Building::GetTypeId(const char* _name)
{
  for (int i = 0; i < NumBuildingTypes; ++i)
  {
    if (_stricmp(_name, GetTypeName(i)) == 0)
      return i;
  }
  return -1;
}

const char* Building::GetTypeName(int _type)
{
  static const char* buildingNames[] = {
    "Invalid", "Factory", // These must be in the
    "Cave", // Same order as defined
    "RadarDish", // in building.h
    "LaserFence", "ControlTower", "GunTurret", "Bridge", "Powerstation", "Tree", "Wall", "TrunkPort", "ResearchItem", "Library",
    "Generator", "Pylon", "PylonStart", "PylonEnd", "SolarPanel", "TrackLink", "TrackJunction", "TrackStart", "TrackEnd", "Refinery",
    "Mine", "Yard", "DisplayScreen", "UpgradePort", "PrimaryUpgrade", "Incubator", "AntHill", "SafeArea", "Triffid", "SpiritReceiver",
    "ReceiverLink", "SpiritSpawner", "SpiritProcessor", "SpawnPoint", "SpawnPopulationLock", "SpawnPointMaster", "SpawnLink", "AITarget",
    "AISpawnPoint", "BlueprintStore", "BlueprintConsole", "BlueprintRelay", "ScriptTrigger", "Spam", "GodDish", "StaticShape",
    "FuelGenerator", "FuelPipe", "FuelStation", "EscapeRocket", "FenceSwitch", "DynamicHub", "DynamicNode", "FeedingTube"
  };

  if (_type >= 0 && _type < NumBuildingTypes)
    return buildingNames[_type];
  DEBUG_ASSERT(false);
  return nullptr;
}

const char* Building::GetTypeNameTranslated(int _type)
{
  const char* typeName = GetTypeName(_type);

  char stringId[256];
  snprintf(stringId, sizeof(stringId), "buildingname_%s", typeName);

  if (ISLANGUAGEPHRASE(stringId))
    return LANGUAGEPHRASE(stringId);
  return typeName;
}
