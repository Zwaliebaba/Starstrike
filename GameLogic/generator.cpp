#include "pch.h"
#include "file_writer.h"
#include "hi_res_time.h"
#include "math_utils.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "text_renderer.h"
#include "text_stream_readers.h"
#include "language_table.h"
#include "generator.h"
#include "constructionyard.h"
#include "controltower.h"
#include "rocket.h"
#include "GameApp.h"
#include "location.h"
#include "camera.h"
#include "global_world.h"
#include "GameSimEventQueue.h"
#include "main.h"

// ****************************************************************************
// Class PowerBuilding
// ****************************************************************************

PowerBuilding::PowerBuilding()
  : Building(),
    m_powerLink(-1),
    m_powerLocation(nullptr) {}

void PowerBuilding::Initialise(Building* _template)
{
  Building::Initialise(_template);
  m_powerLink = static_cast<PowerBuilding*>(_template)->m_powerLink;
}

LegacyVector3 PowerBuilding::GetPowerLocation()
{
  if (!m_powerLocation)
  {
    m_powerLocation = m_shape->GetMarkerData("MarkerPowerLocation");
    DEBUG_ASSERT(m_powerLocation);
  }

  Matrix34 rootMat(m_front, m_up, m_pos);
  Matrix34 worldPos = m_shape->GetMarkerWorldMatrix(m_powerLocation, rootMat);
  return worldPos.pos;
}

bool PowerBuilding::IsInView()
{
  Building* powerLink = g_app->m_location->GetBuilding(m_powerLink);

  if (powerLink)
  {
    LegacyVector3 midPoint = (powerLink->m_centerPos + m_centerPos) / 2.0f;
    float radius = (powerLink->m_centerPos - m_centerPos).Mag() / 2.0f;
    radius += m_radius;

    return (g_app->m_camera->SphereInViewFrustum(midPoint, radius));
  }
  return Building::IsInView();
}

bool PowerBuilding::Advance()
{
  for (int i = 0; i < m_surges.Size(); ++i)
  {
    float* thisSurge = m_surges.GetPointer(i);
    *thisSurge += SERVER_ADVANCE_PERIOD * 2;
    if (*thisSurge >= 1.0f)
    {
      m_surges.RemoveData(i);
      --i;

      Building* powerLink = g_app->m_location->GetBuilding(m_powerLink);
      if (powerLink)
      {
        auto powerBuilding = static_cast<PowerBuilding*>(powerLink);
        powerBuilding->TriggerSurge(0.0f);
      }
    }
  }
  return Building::Advance();
}

void PowerBuilding::TriggerSurge(float _initValue)
{
  m_surges.PutDataAtStart(_initValue);

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "TriggerSurge"));
}

void PowerBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);
  m_powerLink = atoi(_in->GetNextToken());
}

void PowerBuilding::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_powerLink);
}

int PowerBuilding::GetBuildingLink() { return m_powerLink; }

void PowerBuilding::SetBuildingLink(int _buildingId) { m_powerLink = _buildingId; }

// ****************************************************************************
// Class Generator
// ****************************************************************************

Generator::Generator()
  : PowerBuilding(),
    m_timerSync(0.0f),
    m_numThisSecond(0),
    m_enabled(false),
    m_throughput(0.0f)
{
  m_type = TypeGenerator;
  SetShape(g_app->m_resource->GetShapeStatic("generator.shp"));

  m_counter = m_shape->GetMarkerData("MarkerCounter");
}

void Generator::TriggerSurge(float _initValue)
{
  if (m_enabled)
  {
    PowerBuilding::TriggerSurge(_initValue);
    ++m_numThisSecond;
  }
}

const char* Generator::GetObjectiveCounter()
{
  static char result[256];
  snprintf(result, sizeof(result), "%s : %d Gq/s", LANGUAGEPHRASE("objective_output"), static_cast<int>(m_throughput * 10));
  return result;
}

void Generator::ReprogramComplete()
{
  m_enabled = true;
  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Enable"));
}

bool Generator::Advance()
{
  if (!m_enabled)
  {
    m_surges.Empty();
    m_throughput = 0.0f;
    m_numThisSecond = 0;

    //
    // Check to see if our control tower has been captured.
    // This can happen if a user captures the control tower, exits the level and saves,
    // then returns to the level.  The tower is captured and cannot be changed, but
    // the m_enabled state of this building has been lost.

    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        if (building && building->m_type == TypeControlTower)
        {
          auto tower = static_cast<ControlTower*>(building);
          if (tower->GetBuildingLink() == m_id.GetUniqueId() && tower->m_id.GetTeamId() == m_id.GetTeamId())
          {
            m_enabled = true;
            break;
          }
        }
      }
    }
  }
  else
  {
    if (GetHighResTime() >= m_timerSync + 1.0f)
    {
      float newAverage = m_numThisSecond;
      m_numThisSecond = 0;
      m_timerSync = GetHighResTime();
      m_throughput = m_throughput * 0.8f + newAverage * 0.2f;
    }

    if (m_throughput > 6.5f)
    {
      GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
      gb->m_online = true;
    }
  }

  return PowerBuilding::Advance();
}

// ****************************************************************************
// Class Pylon
// ****************************************************************************

Pylon::Pylon()
  : PowerBuilding()
{
  m_type = TypePylon;
  SetShape(g_app->m_resource->GetShapeStatic("pylon.shp"));
}

bool Pylon::Advance() { return PowerBuilding::Advance(); }

// ****************************************************************************
// Class PylonStart
// ****************************************************************************

PylonStart::PylonStart()
  : PowerBuilding(),
    m_reqBuildingId(-1)
{
  m_type = TypePylonStart;
  SetShape(g_app->m_resource->GetShapeStatic("pylon.shp"));
};

void PylonStart::Initialise(Building* _template)
{
  PowerBuilding::Initialise(_template);

  m_reqBuildingId = static_cast<PylonStart*>(_template)->m_reqBuildingId;
}

bool PylonStart::Advance()
{
  //
  // Is the Generator online?

  bool generatorOnline = false;

  int generatorLocationId = g_app->m_globalWorld->GetLocationId("generator");
  GlobalBuilding* globalRefinery = nullptr;
  for (int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i)
  {
    if (g_app->m_globalWorld->m_buildings.ValidIndex(i))
    {
      GlobalBuilding* gb = g_app->m_globalWorld->m_buildings[i];
      if (gb && gb->m_locationId == generatorLocationId && gb->m_type == TypeGenerator && gb->m_online)
      {
        generatorOnline = true;
        break;
      }
    }
  }

  if (generatorOnline)
  {
    //
    // Is our required building online yet?
    GlobalBuilding* globalBuilding = g_app->m_globalWorld->GetBuilding(m_reqBuildingId, g_app->m_locationId);
    if (globalBuilding && globalBuilding->m_online)
    {
      if (syncfrand() > 0.7f)
        TriggerSurge(0.0f);
    }
  }

  return PowerBuilding::Advance();
}

void PylonStart::Read(TextReader* _in, bool _dynamic)
{
  PowerBuilding::Read(_in, _dynamic);

  m_reqBuildingId = atoi(_in->GetNextToken());
}

void PylonStart::Write(FileWriter* _out)
{
  PowerBuilding::Write(_out);

  _out->printf("%-8d", m_reqBuildingId);
}

// ****************************************************************************
// Class PylonEnd
// ****************************************************************************

PylonEnd::PylonEnd()
  : PowerBuilding()
{
  m_type = TypePylonEnd;
  SetShape(g_app->m_resource->GetShapeStatic("pylon.shp"));
};

void PylonEnd::TriggerSurge(float _initValue)
{
  Building* building = g_app->m_location->GetBuilding(m_powerLink);

  if (building && building->m_type == TypeYard)
  {
    auto yard = static_cast<ConstructionYard*>(building);
    yard->AddPowerSurge();
  }

  if (building && building->m_type == TypeFuelGenerator)
  {
    auto fuel = static_cast<FuelGenerator*>(building);
    fuel->ProvideSurge();
  }
}

// ****************************************************************************
// Class SolarPanel
// ****************************************************************************

SolarPanel::SolarPanel()
  : PowerBuilding(),
    m_operating(false)
{
  m_type = TypeSolarPanel;
  SetShape(g_app->m_resource->GetShapeStatic("solarpanel.shp"));

  memset(m_glowMarker, 0, SOLARPANEL_NUMGLOWS * sizeof(ShapeMarkerData*));

  for (int i = 0; i < SOLARPANEL_NUMGLOWS; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerGlow0%d", i + 1);
    m_glowMarker[i] = m_shape->GetMarkerData(name);
    DEBUG_ASSERT(m_glowMarker[i]);
  }

  for (int i = 0; i < SOLARPANEL_NUMSTATUSMARKERS; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerStatus0%d", i + 1);
    m_statusMarkers[i] = m_shape->GetMarkerData(name);
  }
}

void SolarPanel::Initialise(Building* _template)
{
  _template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue(_template->m_pos.x, _template->m_pos.z);
  auto right = LegacyVector3(1, 0, 0);
  _template->m_front = right ^ _template->m_up;

  PowerBuilding::Initialise(_template);
}

bool SolarPanel::Advance()
{
  float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

  if (syncfrand(20.0f) <= fractionOccupied)
    TriggerSurge(0.0f);

  if (fractionOccupied > 0.6f)
  {
    if (!m_operating)
      g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Operate"));
    m_operating = true;
  }

  if (fractionOccupied < 0.3f)
  {
    if (m_operating)
      g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id));
    m_operating = false;
  }

  return PowerBuilding::Advance();
}

