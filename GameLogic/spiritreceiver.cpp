#include "pch.h"

#include "file_writer.h"
#include "hi_res_time.h"
#include "math_utils.h"
#include "ShapeStatic.h"
#include "text_stream_readers.h"
#include "spiritreceiver.h"
#include "GameApp.h"
#include "language_table.h"
#include "location.h"
#include "resource.h"
#include "camera.h"
#include "global_world.h"
#include "GameSimEventQueue.h"

// ****************************************************************************
// Class ReceiverBuilding
// ****************************************************************************

ReceiverBuilding::ReceiverBuilding()
  : Building(),
    m_spiritLink(-1),
    m_spiritLocation(nullptr) {}

void ReceiverBuilding::Initialise(Building* _template)
{
  Building::Initialise(_template);

  m_spiritLink = static_cast<ReceiverBuilding*>(_template)->m_spiritLink;
}

LegacyVector3 ReceiverBuilding::GetSpiritLocation()
{
  if (!m_spiritLocation)
  {
    m_spiritLocation = m_shape->GetMarkerData("MarkerSpiritLink");
    DEBUG_ASSERT(m_spiritLocation);
  }

  Matrix34 rootMat(m_front, m_up, m_pos);
  Matrix34 worldPos = m_shape->GetMarkerWorldMatrix(m_spiritLocation, rootMat);
  return worldPos.pos;
}

bool ReceiverBuilding::IsInView()
{
  Building* spiritLink = g_app->m_location->GetBuilding(m_spiritLink);

  if (spiritLink)
  {
    LegacyVector3 midPoint = (spiritLink->m_centerPos + m_centerPos) / 2.0f;
    float radius = (spiritLink->m_centerPos - m_centerPos).Mag() / 2.0f;
    radius += m_radius;
    return (g_app->m_camera->SphereInViewFrustum(midPoint, radius));
  }
  return Building::IsInView();
}

bool ReceiverBuilding::Advance()
{
  for (int i = 0; i < m_spirits.Size(); ++i)
  {
    float* thisSpirit = m_spirits.GetPointer(i);
    *thisSpirit += SERVER_ADVANCE_PERIOD * 0.8f;
    if (*thisSpirit >= 1.0f)
    {
      m_spirits.RemoveData(i);
      --i;

      Building* spiritLink = g_app->m_location->GetBuilding(m_spiritLink);
      if (spiritLink)
      {
        auto receiverBuilding = static_cast<ReceiverBuilding*>(spiritLink);
        receiverBuilding->TriggerSpirit(0.0f);
      }
    }
  }
  return Building::Advance();
}

void ReceiverBuilding::TriggerSpirit(float _initValue)
{
  m_spirits.PutDataAtStart(_initValue);
  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "TriggerSpirit"));
}

void ReceiverBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);
  m_spiritLink = atoi(_in->GetNextToken());
}

void ReceiverBuilding::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_spiritLink);
}

int ReceiverBuilding::GetBuildingLink() { return m_spiritLink; }

void ReceiverBuilding::SetBuildingLink(int _buildingId) { m_spiritLink = _buildingId; }

SpiritProcessor* ReceiverBuilding::GetSpiritProcessor()
{
  static int processorId = -1;

  auto processor = static_cast<SpiritProcessor*>(g_app->m_location->GetBuilding(processorId));

  if (!processor || processor->m_type != TypeSpiritProcessor)
  {
    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        if (building->m_type == TypeSpiritProcessor)
        {
          processor = static_cast<SpiritProcessor*>(building);
          processorId = processor->m_id.GetUniqueId();
          break;
        }
      }
    }
  }

  return processor;
}

// ****************************************************************************
// Class SpiritProcessor
// ****************************************************************************

SpiritProcessor::SpiritProcessor()
  : ReceiverBuilding(),
    m_timerSync(0.0f),
    m_numThisSecond(0),
    m_spawnSync(0.0f),
    m_throughput(0.0f)
{
  m_type = TypeSpiritProcessor;
  SetShape(g_app->m_resource->GetShapeStatic("spiritprocessor.shp"));
}

void SpiritProcessor::Initialise(Building* _building)
{
  ReceiverBuilding::Initialise(_building);

  //
  // Spawn some unprocessed spirits

  for (int i = 0; i < 150; ++i)
  {
    float sizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    float posY = syncfrand(1000.0f);
    auto spawnPos = LegacyVector3(syncfrand(sizeX), posY, syncfrand(sizeZ));

    auto spirit = new UnprocessedSpirit();
    spirit->m_pos = spawnPos;
    m_floatingSpirits.PutData(spirit);
  }
}

const char* SpiritProcessor::GetObjectiveCounter()
{
  static char result[256];
  snprintf(result, sizeof(result), "%s : %2.2f", LANGUAGEPHRASE("objective_throughput"), m_throughput);
  return result;
}

void SpiritProcessor::TriggerSpirit(float _initValue)
{
  ReceiverBuilding::TriggerSpirit(_initValue);

  ++m_numThisSecond;
}

bool SpiritProcessor::IsInView() { return true; }

bool SpiritProcessor::Advance()
{
  //
  // Calculate our throughput

  m_timerSync -= SERVER_ADVANCE_PERIOD;

  if (m_timerSync <= 0.0f)
  {
    float newAverage = m_numThisSecond;
    newAverage *= 7.0f;
    m_numThisSecond = 0;
    m_timerSync = 10.0f;
    if (newAverage > m_throughput)
      m_throughput = newAverage;
    else
      m_throughput = m_throughput * 0.8f + newAverage * 0.2f;
  }

  if (m_throughput > 50.0f)
  {
    GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
    gb->m_online = true;
  }

  //
  // Advance all unprocessed spirits

  for (int i = 0; i < m_floatingSpirits.Size(); ++i)
  {
    UnprocessedSpirit* spirit = m_floatingSpirits[i];
    bool finished = spirit->Advance();
    if (finished)
    {
      m_floatingSpirits.RemoveData(i);
      delete spirit;
      --i;
    }
  }

  //
  // Spawn more unprocessed spirits

  m_spawnSync -= SERVER_ADVANCE_PERIOD;
  if (m_spawnSync <= 0.0f)
  {
    m_spawnSync = 0.2f;

    float sizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    float posY = 700.0f + syncfrand(300.0f);
    auto spawnPos = LegacyVector3(syncfrand(sizeX), posY, syncfrand(sizeZ));
    auto spirit = new UnprocessedSpirit();
    spirit->m_pos = spawnPos;
    m_floatingSpirits.PutData(spirit);
  }

  return ReceiverBuilding::Advance();
}

// ****************************************************************************
// Class ReceiverLink
// ****************************************************************************

ReceiverLink::ReceiverLink()
  : ReceiverBuilding()
{
  m_type = TypeReceiverLink;
  SetShape(g_app->m_resource->GetShapeStatic("receiverlink.shp"));
}

bool ReceiverLink::Advance() { return ReceiverBuilding::Advance(); }

// ****************************************************************************
// Class ReceiverSpiritSpawner
// ****************************************************************************

ReceiverSpiritSpawner::ReceiverSpiritSpawner()
  : ReceiverBuilding()
{
  m_type = TypeReceiverSpiritSpawner;
  SetShape(g_app->m_resource->GetShapeStatic("receiverlink.shp"));
}

bool ReceiverSpiritSpawner::Advance()
{
  if (syncfrand(10.0f) < 1.0f)
    TriggerSpirit(0.0f);

  return ReceiverBuilding::Advance();
}

// ****************************************************************************
// Class SpiritReceiver
// ****************************************************************************

SpiritReceiver::SpiritReceiver()
  : ReceiverBuilding(),
    m_headMarker(nullptr),
    m_headShape(nullptr),
    m_spiritLink(nullptr)
{
  m_type = TypeSpiritReceiver;
  SetShape(g_app->m_resource->GetShapeStatic("spiritreceiver.shp"));
  m_headMarker = m_shape->GetMarkerData("MarkerHead");

  for (int i = 0; i < SPIRITRECEIVER_NUMSTATUSMARKERS; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerStatus0%d", i + 1);
    m_statusMarkers[i] = m_shape->GetMarkerData(name);
  }

  m_headShape = g_app->m_resource->GetShapeStatic("spiritreceiverhead.shp");
  m_spiritLink = m_headShape->GetMarkerData("MarkerSpiritLink");
}

void SpiritReceiver::Initialise(Building* _template)
{
  _template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue(_template->m_pos.x, _template->m_pos.z);
  auto right = LegacyVector3(1, 0, 0);
  _template->m_front = right ^ _template->m_up;

  ReceiverBuilding::Initialise(_template);
}

bool SpiritReceiver::Advance()
{
  float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

  //
  // Search for spirits nearby

  SpiritProcessor* processor = GetSpiritProcessor();
  if (processor && fractionOccupied > 0.0f)
  {
    for (int i = 0; i < processor->m_floatingSpirits.Size(); ++i)
    {
      UnprocessedSpirit* spirit = processor->m_floatingSpirits[i];
      if (spirit->m_state == UnprocessedSpirit::StateUnprocessedFloating)
      {
        LegacyVector3 themToUs = (m_pos - spirit->m_pos);
        float distance = themToUs.Mag();
        LegacyVector3 targetPos = m_pos;
        if (distance < 100.0f)
        {
          float fraction = 1.0f - distance / 100.0f;
          targetPos += LegacyVector3(0, 100.0f * fraction, 0);
          themToUs = targetPos - spirit->m_pos;
          distance = themToUs.Mag();
        }

        if (distance < 10.0f)
        {
          processor->m_floatingSpirits.RemoveData(i);
          delete spirit;
          --i;
          TriggerSpirit(0.0f);
        }
        else if (distance < 200.0f)
        {
          float fraction = 1.0f - distance / 200.0f;
          fraction *= fractionOccupied;
          themToUs.SetLength(20.0f * fraction);
          spirit->m_vel += themToUs * SERVER_ADVANCE_PERIOD;
        }
      }
    }
  }

  return ReceiverBuilding::Advance();
}

LegacyVector3 SpiritReceiver::GetSpiritLocation()
{
  Matrix34 mat(m_front, m_up, m_pos);
  LegacyVector3 headPos = m_shape->GetMarkerWorldMatrix(m_headMarker, mat).pos;
  LegacyVector3 up = g_upVector;
  LegacyVector3 right(1, 0, 0);
  LegacyVector3 front = up ^ right;
  Matrix34 headMat(front, up, headPos);
  LegacyVector3 spiritLinkPos = m_headShape->GetMarkerWorldMatrix(m_spiritLink, headMat).pos;
  return spiritLinkPos;
}

// ****************************************************************************
// Class UnprocessedSpirit
// ****************************************************************************

UnprocessedSpirit::UnprocessedSpirit()
  : WorldObject()
{
  m_state = StateUnprocessedFalling;

  m_positionOffset = syncfrand(10.0f);
  m_xaxisRate = syncfrand(2.0f);
  m_yaxisRate = syncfrand(2.0f);
  m_zaxisRate = syncfrand(2.0f);

  m_timeSync = GetHighResTime();
}

bool UnprocessedSpirit::Advance()
{
  m_vel *= 0.9f;

  //
  // Make me float around slowly

  m_positionOffset += SERVER_ADVANCE_PERIOD;
  m_xaxisRate += syncsfrand(1.0f);
  m_yaxisRate += syncsfrand(1.0f);
  m_zaxisRate += syncsfrand(1.0f);
  if (m_xaxisRate > 2.0f)
    m_xaxisRate = 2.0f;
  if (m_xaxisRate < 0.0f)
    m_xaxisRate = 0.0f;
  if (m_yaxisRate > 2.0f)
    m_yaxisRate = 2.0f;
  if (m_yaxisRate < 0.0f)
    m_yaxisRate = 0.0f;
  if (m_zaxisRate > 2.0f)
    m_zaxisRate = 2.0f;
  if (m_zaxisRate < 0.0f)
    m_zaxisRate = 0.0f;
  m_hover.x = sinf(m_positionOffset) * m_xaxisRate;
  m_hover.y = sinf(m_positionOffset) * m_yaxisRate;
  m_hover.z = sinf(m_positionOffset) * m_zaxisRate;

  switch (m_state)
  {
  case StateUnprocessedFalling:
    {
      float heightAboveGround = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
      if (heightAboveGround > 15.0f)
      {
        float fractionAboveGround = heightAboveGround / 100.0f;
        fractionAboveGround = min(fractionAboveGround, 1.0f);
        m_hover.y = (-10.0f - syncfrand(10.0f)) * fractionAboveGround;
      }
      else
      {
        m_state = StateUnprocessedFloating;
        m_timeSync = 30.0f + syncsfrand(30.0f);
      }
      break;
    }

  case StateUnprocessedFloating:
    m_timeSync -= SERVER_ADVANCE_PERIOD;
    if (m_timeSync <= 0.0f)
    {
      m_state = StateUnprocessedDeath;
      m_timeSync = 10.0f;
    }
    break;

  case StateUnprocessedDeath:
    m_timeSync -= SERVER_ADVANCE_PERIOD;
    if (m_timeSync <= 0.0f)
      return true;
    break;
  }

  LegacyVector3 oldPos = m_pos;

  m_pos += m_vel * SERVER_ADVANCE_PERIOD;
  m_pos += m_hover * SERVER_ADVANCE_PERIOD;
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

  return false;
}

float UnprocessedSpirit::GetLife()
{
  switch (m_state)
  {
  case StateUnprocessedFalling:
    {
      float timePassed = GetHighResTime() - m_timeSync;
      float life = timePassed / 10.0f;
      life = min(life, 1.0f);
      return life;
    }

  case StateUnprocessedFloating:
    return 1.0f;

  case StateUnprocessedDeath:
    {
      float timeLeft = m_timeSync;
      float life = timeLeft / 10.0f;
      life = min(life, 1.0f);
      life = max(life, 0.0f);
      return life;
    }
  }

  return 0.0f;
}
