#include "pch.h"
#include "resource.h"
#include "file_writer.h"
#include "text_stream_readers.h"
#include "ShapeStatic.h"
#include "preferences.h"
#include "language_table.h"
#include "rocket.h"
#include "darwinian.h"
#include "GameApp.h"
#include "location.h"
#include "camera.h"
#include "team.h"
#include "main.h"
#include "GameSimEventQueue.h"
#include "global_world.h"
#include "script.h"
#include "entity_grid.h"
#include "taskmanager_interface.h"

ShapeStatic* FuelBuilding::s_fuelPipe = nullptr;

FuelBuilding::FuelBuilding()
  : Building(),
    m_fuelMarker(nullptr),
    m_fuelLink(-1),
    m_currentLevel(0.0f)
{
  if (!s_fuelPipe)
  {
    s_fuelPipe = g_app->m_resource->GetShapeStatic("fuelpipe.shp");
    DEBUG_ASSERT(s_fuelPipe);
  }
}

void FuelBuilding::Initialise(Building* _template)
{
  Building::Initialise(_template);

  m_fuelLink = static_cast<FuelBuilding*>(_template)->m_fuelLink;
}

LegacyVector3 FuelBuilding::GetFuelPosition()
{
  if (!m_fuelMarker)
  {
    m_fuelMarker = m_shape->GetMarkerData("MarkerFuel");
    DEBUG_ASSERT(m_fuelMarker);
  }

  Matrix34 mat(m_front, m_up, m_pos);
  return m_shape->GetMarkerWorldMatrix(m_fuelMarker, mat).pos;
}

void FuelBuilding::ProvideFuel(float _level)
{
  float factor2 = 0.2f * SERVER_ADVANCE_PERIOD;
  float factor1 = 1.0f - factor2;

  m_currentLevel = m_currentLevel * factor1 + _level * factor2;

  m_currentLevel = min(m_currentLevel, 1.0f);
  m_currentLevel = max(m_currentLevel, 0.0f);
}

FuelBuilding* FuelBuilding::GetLinkedBuilding()
{
  Building* building = g_app->m_location->GetBuilding(m_fuelLink);
  if (building)
  {
    if (building->m_type == TypeFuelGenerator || building->m_type == TypeFuelPipe || building->m_type == TypeEscapeRocket || building->
      m_type == TypeFuelStation)
    {
      auto fuelBuilding = static_cast<FuelBuilding*>(building);
      return fuelBuilding;
    }
  }

  return nullptr;
}

bool FuelBuilding::Advance()
{
  FuelBuilding* fuelBuilding = GetLinkedBuilding();
  if (fuelBuilding)
    fuelBuilding->ProvideFuel(m_currentLevel);

  return Building::Advance();
}

bool FuelBuilding::IsInView()
{
  FuelBuilding* fuelBuilding = GetLinkedBuilding();

  if (fuelBuilding)
  {
    LegacyVector3 ourPipePos = GetFuelPosition();
    LegacyVector3 theirPipePos = fuelBuilding->GetFuelPosition();

    LegacyVector3 midPoint = (theirPipePos + ourPipePos) / 2.0f;
    float radius = (theirPipePos - ourPipePos).Mag() / 2.0f;
    radius += m_radius;

    return (g_app->m_camera->SphereInViewFrustum(midPoint, radius));
  }
  return Building::IsInView();
}

void FuelBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_fuelLink = atoi(_in->GetNextToken());
}

void FuelBuilding::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%6d ", m_fuelLink);
}

int FuelBuilding::GetBuildingLink() { return m_fuelLink; }

void FuelBuilding::SetBuildingLink(int _buildingId) { m_fuelLink = _buildingId; }

void FuelBuilding::Destroy(float _intensity)
{
  Building::Destroy(_intensity);
  FuelBuilding* fuelBuilding = GetLinkedBuilding();

  if (fuelBuilding)
  {
    LegacyVector3 ourPipePos = GetFuelPosition();
    LegacyVector3 theirPipePos = fuelBuilding->GetFuelPosition();

    LegacyVector3 pipeVector = (theirPipePos - ourPipePos).Normalise();
    LegacyVector3 right = pipeVector ^ g_upVector;
    LegacyVector3 up = pipeVector ^ right;

    ourPipePos += pipeVector * 10;

    Matrix34 pipeMat(up, pipeVector, ourPipePos);
    g_simEventQueue.Push(SimEvent::MakeExplosion(s_fuelPipe, pipeMat, 1.0f));
  }
}

// ============================================================================

FuelGenerator::FuelGenerator()
  : FuelBuilding(),
    m_pump(nullptr),
    m_pumpTip(nullptr),
    m_pumpMovement(0.0f),
    m_previousPumpPos(0.0f),
    m_surges(0.0f)
{
  m_type = TypeFuelGenerator;

  SetShape(g_app->m_resource->GetShapeStatic("fuelgenerator.shp"));

  m_pump = g_app->m_resource->GetShapeStatic("fuelgeneratorpump.shp");
  m_pumpTip = m_pump->GetMarkerData("MarkerTip");
}

void FuelGenerator::ProvideSurge() { m_surges++; }

bool FuelGenerator::Advance()
{
  //
  // Advance surges

  m_surges -= SERVER_ADVANCE_PERIOD * 1.0f;
  m_surges = min(m_surges, 10);
  m_surges = max(m_surges, 0);

  if (m_surges > 8)
  {
    GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
    if (gb)
      gb->m_online = true;
  }

  //
  // Pump fuel

  float fuelVal = m_surges / 10.0f;
  ProvideFuel(fuelVal);

  //
  // Spawn particles

  float previousPumpPos = m_previousPumpPos;
  LegacyVector3 pumpPos = GetPumpPos();
  m_previousPumpPos = (pumpPos.y - m_pos.y) / -80.0f;

  if (fuelVal > 0.0f && pumpPos.y > m_pos.y - 20.0f)
  {
    pumpPos.x += sfrand(10.0f);
    pumpPos.z += sfrand(10.0f);

    for (int i = 0; i < static_cast<int>(m_surges); ++i)
    {
      LegacyVector3 pumpVel = g_upVector * 20;
      pumpVel += g_upVector * frand(10);

      Matrix34 mat(m_front, g_upVector, pumpPos);
      LegacyVector3 particlePos = m_pump->GetMarkerWorldMatrix(m_pumpTip, mat).pos;
      float size = 150.0f + frand(150.0f);

      g_simEventQueue.Push(SimEvent::MakeParticle(particlePos, pumpVel, SimParticle::TypeDarwinianFire, size));
    }
  }

  //
  // Play sounds

  if (previousPumpPos >= 0.1f && m_previousPumpPos < 0.1f)
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "PumpUp"));
  else if (previousPumpPos <= 0.9f && m_previousPumpPos > 0.9f)
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "PumpDown"));

  return FuelBuilding::Advance();
}

LegacyVector3 FuelGenerator::GetPumpPos()
{
  LegacyVector3 pumpPos = m_pos;
  float pumpHeight = 80;

  pumpPos.y -= pumpHeight;
  pumpPos.y += fabs(cosf(m_pumpMovement)) * pumpHeight;

  return pumpPos;
}

const char* FuelGenerator::GetObjectiveCounter()
{
  static char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s %d%%", LANGUAGEPHRASE("objective_fuelpressure"), static_cast<int>(m_currentLevel * 100));
  return buffer;
}

// ============================================================================

FuelPipe::FuelPipe()
  : FuelBuilding()
{
  m_type = TypeFuelPipe;

  SetShape(g_app->m_resource->GetShapeStatic("fuelpipebase.shp"));
}

bool FuelPipe::Advance()
{
  //
  // Ensure our sound ambiences are playing

  if (m_currentLevel > 0.2f && !m_pumpSoundActive)
  {
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "PumpFuel"));
    m_pumpSoundActive = true;
  }
  else if (m_currentLevel <= 0.2f && m_pumpSoundActive)
  {
    g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id, "FuelPipe PumpFuel"));
    m_pumpSoundActive = false;
  }

  return FuelBuilding::Advance();
}

// ============================================================================

FuelStation::FuelStation()
  : FuelBuilding(),
    m_entrance(nullptr)
{
  m_type = TypeFuelStation;

  SetShape(g_app->m_resource->GetShapeStatic("fuelstation.shp"));

  m_entrance = m_shape->GetMarkerData("MarkerEntrance");
}

bool FuelStation::IsLoading()
{
  Building* building = g_app->m_location->GetBuilding(m_fuelLink);
  if (building && building->m_type == TypeEscapeRocket)
  {
    auto rocket = static_cast<EscapeRocket*>(building);
    if (rocket->m_state == EscapeRocket::StateLoading)
      return true;
  }

  return false;
}

bool FuelStation::Advance()
{
  Building* building = g_app->m_location->GetBuilding(m_fuelLink);
  if (building && building->m_type == TypeEscapeRocket)
  {
    auto rocket = static_cast<EscapeRocket*>(building);
    if (rocket->m_state == EscapeRocket::StateLoading && rocket->SafeToLaunch())
    {
      //
      // Find a random Darwinian and make him board

      Team* team = &g_app->m_location->m_teams[0];
      int numOthers = team->m_others.Size();
      if (numOthers > 0)
      {
        int randomIndex = syncrand() % numOthers;
        if (team->m_others.ValidIndex(randomIndex))
        {
          Entity* entity = team->m_others[randomIndex];
          if (entity && entity->m_type == Entity::TypeDarwinian)
          {
            auto darwinian = static_cast<Darwinian*>(entity);
            float distance = (entity->m_pos - m_pos).Mag();
            if (distance < 300.0f && (darwinian->m_state == Darwinian::StateIdle || darwinian->m_state == Darwinian::StateWorshipSpirit))
              darwinian->BoardRocket(m_id.GetUniqueId());
          }
        }
      }
    }
  }

  return FuelBuilding::Advance();
}

LegacyVector3 FuelStation::GetEntrance()
{
  Matrix34 mat(m_front, g_upVector, m_pos);
  return m_shape->GetMarkerWorldMatrix(m_entrance, mat).pos;
}

bool FuelStation::BoardRocket(WorldObjectId _id)
{
  Building* building = g_app->m_location->GetBuilding(m_fuelLink);
  if (building && building->m_type == TypeEscapeRocket)
  {
    auto rocket = static_cast<EscapeRocket*>(building);
    bool result = rocket->BoardRocket(_id);

    if (result)
    {
      Entity* entity = g_app->m_location->GetEntity(_id);
      LegacyVector3 entityPos = entity ? entity->m_pos : g_zeroVector;
      entityPos.y += 2;

      int numFlashes = 4 + darwiniaRandom() % 4;
      for (int i = 0; i < numFlashes; ++i)
      {
        LegacyVector3 vel(sfrand(15.0f), frand(35.0f), sfrand(15.0f));
        g_simEventQueue.Push(SimEvent::MakeParticle(entityPos, vel, SimParticle::TypeControlFlash));
      }

      g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "LoadPassenger"));
    }

    return result;
  }

  return false;
}

bool FuelStation::PerformDepthSort(LegacyVector3& _centerPos)
{
  _centerPos = m_centerPos;
  return true;
}

// ============================================================================

EscapeRocket::EscapeRocket()
  : FuelBuilding(),
    m_booster(nullptr),
    m_shadowTimer(0.0f),
    m_cameraShake(0.0f),
    m_state(StateRefueling),
    m_fuel(0.0f),
    m_pipeCount(0),
    m_passengers(0),
    m_countdown(-1.0f),
    m_damage(0.0f),
    m_spawnBuildingId(-1),
    m_spawnCompleted(false)
{
  m_type = TypeEscapeRocket;

  SetShape(g_app->m_resource->GetShapeStatic("rocket.shp"));

  m_booster = m_shape->GetMarkerData("MarkerBooster");
  ASSERT_TEXT(m_booster, "MarkerBooster not found in rocket.shp");

  for (int i = 0; i < 3; ++i)
  {
    char name[256];
    snprintf(name, sizeof(name), "MarkerWindow0%d", i + 1);
    m_window[i] = m_shape->GetMarkerData(name);
    ASSERT_TEXT(m_window[i], "%s not found", name);
  }
}

void EscapeRocket::SetupSounds()
{
  const char* requiredSoundName = nullptr;

  //
  // What ambience should be playing?

  switch (m_state)
  {
  case StateRefueling:
    if (m_currentLevel > 0.2f)
      requiredSoundName = "Refueling";
    else
      requiredSoundName = "Unhappy";
    break;

  case StateLoading:
  case StateIgnition:
  case StateReady:
  case StateCountdown:
    if (m_damage < 10)
      requiredSoundName = "Happy";
    else
      requiredSoundName = "Malfunction";
    break;

  case StateExploding:
    if (m_fuel > 0.0f)
      requiredSoundName = "Malfunction";
    else
      requiredSoundName = "Unhappy";
    break;

  case StateFlight:
    requiredSoundName = "Flight";
    break;
  }

  //
  // If the required sound changed, stop old and start new

  if (requiredSoundName != m_activeSoundName)
  {
    // Stop all ambience sounds for this rocket
    g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id));

    if (requiredSoundName)
      g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, requiredSoundName));

    m_activeSoundName = requiredSoundName;

    // Engine sound was also stopped — re-arm so it re-triggers below if needed
    m_engineSoundActive = false;
  }

  //
  // If our engines are on then trigger the event

  bool wantEngine = (m_state == StateReady || m_state == StateCountdown || m_state == StateFlight);

  if (wantEngine && !m_engineSoundActive)
  {
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "EngineBurn"));
    m_engineSoundActive = true;
  }
  else if (!wantEngine && m_engineSoundActive)
  {
    g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id, "EscapeRocket EngineBurn"));
    m_engineSoundActive = false;
  }
}

void EscapeRocket::Initialise(Building* _template)
{
  FuelBuilding::Initialise(_template);

  m_fuel = static_cast<EscapeRocket*>(_template)->m_fuel;
  m_passengers = static_cast<EscapeRocket*>(_template)->m_passengers;
  m_spawnBuildingId = static_cast<EscapeRocket*>(_template)->m_spawnBuildingId;
  m_spawnCompleted = static_cast<EscapeRocket*>(_template)->m_spawnCompleted;
}

const char* EscapeRocket::GetObjectiveCounter()
{
  static char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s %d%%, %s %d%%", LANGUAGEPHRASE("objective_fuel"), static_cast<int>(m_fuel), LANGUAGEPHRASE("objective_passengers"),
          static_cast<int>(m_passengers));
  return buffer;
}

bool EscapeRocket::BoardRocket(WorldObjectId _id)
{
  if (m_state == StateLoading)
  {
    ++m_passengers;
    return true;
  }

  return false;
}

void EscapeRocket::ProvideFuel(float _level)
{
#ifdef CHEATMENU_ENABLED
  if (g_inputManager->controlEvent(ControlScrollSpeedup))
    _level *= 100;
#endif

  FuelBuilding::ProvideFuel(_level);

  if (_level > 0.1f)
    ++m_pipeCount;
}

void EscapeRocket::Refuel()
{
  switch (m_pipeCount)
  {
  case 0: // No incoming fuel
  case 1: // 1 pipe providing fuel
    {
      float targetFuel = 50.0f;
      if (m_fuel < targetFuel)
      {
        float factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.005f;
        float factor2 = 1.0f - factor1;
        m_fuel = m_fuel * factor2 + targetFuel * factor1;
      }
      break;
    }

  case 2: // 2 pipes providing fuel
    {
      float targetFuel = 100.0f;
      float factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.01f;
      float factor2 = 1.0f - factor1;
      m_fuel = m_fuel * factor2 + targetFuel * factor1;
      m_fuel = min(m_fuel, 100.0f);
      break;
    }

  case 3: // 3 pipes providing fuel
    {
      float targetFuel = 100.0f;
      float factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.1;
      float factor2 = 1.0f - factor1;
      m_fuel = m_fuel * factor2 + targetFuel * factor1;
      m_fuel = min(m_fuel, 100.0f);
      break;
    }
  }

  m_pipeCount = 0;
}

void EscapeRocket::AdvanceRefueling()
{
  Refuel();

  m_damage = 0.0f;

  if (m_fuel >= 95.0f)
    m_state = StateLoading;
}

void EscapeRocket::AdvanceLoading()
{
  Refuel();

  if (m_fuel > 95.0f && m_passengers >= 100)
  {
    m_state = StateIgnition;
    m_countdown = 18.0f;
  }
}

void EscapeRocket::AdvanceIgnition()
{
  m_countdown -= SERVER_ADVANCE_PERIOD;

  SetupSpectacle();

  if (m_countdown <= 0.0f)
  {
    m_state = StateReady;
    m_countdown = 120.0f;

    m_cameraShake = 5.0f;

    if (m_spawnCompleted)
    {
      m_countdown = 10.0f;
      g_app->m_taskManagerInterface->SetVisible(false);
      if (g_app->m_script->IsRunningScript())
        g_app->m_script->Skip();
#ifdef DEMOBUILD
      g_app->m_script->RunScript("launchpad_victory_demo.txt");
#else
      g_app->m_script->RunScript("launchpad_victory.txt");
#endif
    }
  }
}

void EscapeRocket::AdvanceReady()
{
  //
  // Spawn attacking Darwinians

  if (!m_spawnCompleted)
  {
    if (m_countdown > 100.0f && m_countdown < 110.0f)
    {
      Building* spawnBuilding = g_app->m_location->GetBuilding(m_spawnBuildingId);
      if (spawnBuilding)
      {
        LegacyVector3 spawnPos = spawnBuilding->m_pos + spawnBuilding->m_front * 40.0f;
        g_app->m_location->SpawnEntities(spawnPos, 1, -1, Entity::TypeDarwinian, 1, g_zeroVector, 40.0f);
      }
    }
  }

  SetupAttackers();
  SetupSpectacle();

  m_countdown -= SERVER_ADVANCE_PERIOD;

  if (m_countdown <= 0.0f)
  {
    m_state = StateCountdown;
    m_countdown = 10.0f;
  }
}

void EscapeRocket::AdvanceCountdown()
{
  m_countdown -= SERVER_ADVANCE_PERIOD * 0.5f;
  m_countdown = max(m_countdown, 0.0f);

  SetupAttackers();
  SetupSpectacle();

  if (m_countdown == 0.0f)
  {
    m_state = StateFlight;
  }
}

void EscapeRocket::AdvanceFlight()
{
  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  float thrust = sqrtf(m_pos.y - landHeight) * 2;
  thrust = max(thrust, 0.1f);

  m_vel.Set(0, thrust, 0);

  m_pos += m_vel * SERVER_ADVANCE_PERIOD;
  m_centerPos += m_vel * SERVER_ADVANCE_PERIOD;

  SetupSpectacle();
}

void EscapeRocket::AdvanceExploding()
{
  m_damage -= SERVER_ADVANCE_PERIOD * 1;

  //
  // Burn fuel

  m_fuel -= SERVER_ADVANCE_PERIOD * 10;
  m_fuel = max(m_fuel, 0.0f);

  //
  // Kill passengers

  if (m_passengers)
  {
    m_passengers--;

    int windowIndex = syncrand() % 3;
    Matrix34 mat(m_front, m_up, m_pos);
    Matrix34 windowMat = m_shape->GetMarkerWorldMatrix(m_window[windowIndex], mat);

    LegacyVector3 vel = windowMat.f;
    float angle = syncsfrand(M_PI * 0.25f);
    vel.RotateAround(windowMat.u * angle);
    vel.SetLength(10.0f + syncfrand(30.0f));

    WorldObjectId id = g_app->m_location->SpawnEntities(windowMat.pos, 0, -1, Entity::TypeDarwinian, 1, vel, 0.0f);
    auto darwinian = static_cast<Darwinian*>(g_app->m_location->GetEntity(id));
    darwinian->m_onGround = false;
    darwinian->SetFire();
  }

  if (m_fuel > 0.0f)
  {
    Matrix34 mat(m_front, g_upVector, m_pos);
    g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, mat, 0.001f));
  }

  //
  // Spawn smoke and fire

  for (int i = 0; i < 3; ++i)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    Matrix34 windowMat = m_shape->GetMarkerWorldMatrix(m_window[i], mat);

    LegacyVector3 vel = windowMat.f;
    float angle = syncsfrand(M_PI * 0.25f);
    vel.RotateAround(windowMat.u * angle);
    vel.SetLength(5.0f + syncfrand(10.0f));
    float fireSize = 150 + syncfrand(150.0f);

    LegacyVector3 smokeVel = vel;
    float smokeSize = fireSize;

    if (m_fuel > 0.0f)
      g_simEventQueue.Push(SimEvent::MakeParticle(windowMat.pos, vel, SimParticle::TypeFire, fireSize));
    g_simEventQueue.Push(SimEvent::MakeParticle(windowMat.pos, smokeVel, SimParticle::TypeMissileTrail, smokeSize));
  }

  if (m_damage <= 0.0f)
  {
    m_damage = 0.0f;
    m_spawnCompleted = true;
    m_state = StateRefueling;
  }
}

void EscapeRocket::SetupSpectacle()
{
  m_shadowTimer -= SERVER_ADVANCE_PERIOD;
  if (m_shadowTimer <= 0.0f)
  {
    for (int t = 0; t < NUM_TEAMS; ++t)
    {
      Team* team = &g_app->m_location->m_teams[t];
      for (int i = 0; i < team->m_others.Size(); ++i)
      {
        if (team->m_others.ValidIndex(i))
        {
          Entity* entity = team->m_others[i];
          if (entity && entity->m_type == Entity::TypeDarwinian)
          {
            auto darwinian = static_cast<Darwinian*>(entity);
            //if( m_state == StateReady ) darwinian->CastShadow( m_id.GetUniqueId() );
            // Causes too much of a slow down, and doesn't add much visually to the scene
            if (t == 0 && darwinian->m_state == Darwinian::StateIdle && (syncrand() % 10) < 2)
              darwinian->WatchSpectacle(m_id.GetUniqueId());
          }
        }
      }
    }

    m_shadowTimer = 1.0f;
  }
}

bool EscapeRocket::IsSpectacle()
{
  return (m_state == StateIgnition || m_state == StateReady || m_state == StateCountdown || m_state == StateFlight);
}

bool EscapeRocket::IsInView() { return FuelBuilding::IsInView() || m_state == StateFlight; }

void EscapeRocket::SetupAttackers()
{
  if (!m_spawnCompleted && syncfrand() < 0.2f)
  {
    Team* team = &g_app->m_location->m_teams[1];
    int numOthers = team->m_others.Size();
    if (numOthers > 0)
    {
      int randomIndex = syncrand() % numOthers;
      if (team->m_others.ValidIndex(randomIndex))
      {
        Entity* entity = team->m_others[randomIndex];
        if (entity && entity->m_type == Entity::TypeDarwinian)
        {
          auto darwinian = static_cast<Darwinian*>(entity);
          float range = (darwinian->m_pos - m_pos).Mag();
          if (range < 350.0f)
            darwinian->AttackBuilding(m_id.GetUniqueId());
        }
      }
    }
  }
}

void EscapeRocket::Damage(float _damage)
{
  FuelBuilding::Damage(_damage);

  if (m_state != StateExploding)
  {
    m_damage -= _damage;

    if (m_damage > 100.0f)
    {
      m_state = StateExploding;
      g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Explode"));
    }
  }
}

bool EscapeRocket::Advance()
{
  switch (m_state)
  {
  case StateRefueling:
    AdvanceRefueling();
    break;
  case StateLoading:
    AdvanceLoading();
    break;
  case StateIgnition:
    AdvanceIgnition();
    break;
  case StateReady:
    AdvanceReady();
    break;
  case StateCountdown:
    AdvanceCountdown();
    break;
  case StateFlight:
    AdvanceFlight();
    break;
  case StateExploding:
    AdvanceExploding();
    break;
  }

  SetupSounds();

  //
  // Create rocket flames
  // Shake the camera

  if (m_cameraShake > 0.0)
  {
    m_cameraShake -= SERVER_ADVANCE_PERIOD;

    float actualShake = m_cameraShake / 5.0f;
    g_app->m_camera->CreateCameraShake(actualShake);
  }

  if (m_state == StateReady || m_state == StateCountdown || m_state == StateFlight)
  {
    Matrix34 mat(m_front, g_upVector, m_pos);
    LegacyVector3 boosterPos = m_shape->GetMarkerWorldMatrix(m_booster, mat).pos;

    for (int i = 0; i < 15; ++i)
    {
      LegacyVector3 pos = boosterPos;
      pos += LegacyVector3(sfrand(20), 10, sfrand(20));

      LegacyVector3 vel(sfrand(50), -frand(150), sfrand(50));
      float size = 500.0f;

      if (i > 10)
      {
        vel.x *= 0.75f;
        vel.z *= 0.75f;
        g_simEventQueue.Push(SimEvent::MakeParticle(pos, vel, SimParticle::TypeMissileTrail, size));
      }
      else
        g_simEventQueue.Push(SimEvent::MakeParticle(pos, vel, SimParticle::TypeMissileFire, size));
    }
  }

  return FuelBuilding::Advance();
}

bool EscapeRocket::SafeToLaunch()
{
  LegacyVector3 testPos = m_pos + LegacyVector3(330, 0, 50);
  float testRadius = 100.0f;

  int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies(testPos.x, testPos.z, testRadius, 0);

  return (numEnemies < 2);
}

void EscapeRocket::Read(TextReader* _in, bool _dynamic)
{
  FuelBuilding::Read(_in, _dynamic);

  m_fuel = atof(_in->GetNextToken());
  m_passengers = atoi(_in->GetNextToken());
  m_spawnBuildingId = atoi(_in->GetNextToken());
  m_spawnCompleted = atoi(_in->GetNextToken());
}

void EscapeRocket::Write(FileWriter* _out)
{
  FuelBuilding::Write(_out);

  _out->printf("%6.2f %6d %6d %6d", m_fuel, m_passengers, m_spawnBuildingId, static_cast<int>(m_spawnCompleted));
}

int EscapeRocket::GetStateId(const char* _state)
{
  static const char* stateNames[] = {"Refueling", "Loading", "Ignition", "Ready", "Countdown", "Exploding", "Flight"};

  for (int i = 0; i < NumStates; ++i)
  {
    if (_stricmp(stateNames[i], _state) == 0)
      return i;
  }

  return -1;
}
