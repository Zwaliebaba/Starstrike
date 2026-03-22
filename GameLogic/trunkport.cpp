#include "pch.h"
#include "resource.h"
#include "file_writer.h"
#include "text_stream_readers.h"
#include "ShapeStatic.h"
#include "hi_res_time.h"
#include "text_renderer.h"
#include "profiler.h"
#include "language_table.h"
#include "trunkport.h"
#include "SimEventQueue.h"
#include "GameApp.h"
#include "location.h"
#include "global_world.h"
#include "main.h"

TrunkPort::TrunkPort()
  : Building(),
    m_targetLocationId(-1),
    m_heightMapSize(TRUNKPORT_HEIGHTMAP_MAXSIZE),
    m_heightMap(nullptr),
    m_openTimer(0.0f)
{
  m_type = TypeTrunkPort;
  SetShape(g_app->m_resource->GetShapeStatic("trunkport.shp"));

  m_destination1 = m_shape->GetMarkerData("MarkerDestination1");
  m_destination2 = m_shape->GetMarkerData("MarkerDestination2");
}

void TrunkPort::SetDetail(int _detail)
{
  m_heightMapSize = static_cast<int>(TRUNKPORT_HEIGHTMAP_MAXSIZE / (float)_detail);

  //
  // Pre-Generate our height map

  if (m_heightMap)
    delete m_heightMap;
  m_heightMap = new LegacyVector3[m_heightMapSize * m_heightMapSize];
  memset(m_heightMap, 0, m_heightMapSize * m_heightMapSize * sizeof(LegacyVector3));

  ShapeMarkerData* marker = m_shape->GetMarkerData("MarkerSurface");
  DEBUG_ASSERT(marker);

  Matrix34 transform(m_front, g_upVector, m_pos);
  LegacyVector3 worldPos = m_shape->GetMarkerWorldMatrix(marker, transform).pos;

  float size = 90.0f;
  LegacyVector3 up = g_upVector * size;
  LegacyVector3 right = (m_front ^ g_upVector).Normalise() * size;

  for (int x = 0; x < m_heightMapSize; ++x)
  {
    for (int z = 0; z < m_heightMapSize; ++z)
    {
      float fractionX = static_cast<float>(x) / static_cast<float>(m_heightMapSize - 1);
      float fractionZ = static_cast<float>(z) / static_cast<float>(m_heightMapSize - 1);

      fractionX -= 0.5f;
      fractionZ -= 0.5f;

      LegacyVector3 basePos = worldPos;
      basePos += right * fractionX;
      basePos += up * fractionZ;

      //basePos += right * 0.02f;
      //basePos += up * 0.02f;

      //basePos += right * 0.05f;
      //basePos += up * 0.05f;

      m_heightMap[z * m_heightMapSize + x] = basePos;
    }
  }
}

bool TrunkPort::Advance()
{
  GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
  if (gb && gb->m_online && m_openTimer == 0.0f)
  {
    m_openTimer = GetHighResTime();
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, m_type, "PowerUp"));
  }

  return Building::Advance();
}

void TrunkPort::Initialise(Building* _template)
{
  Building::Initialise(_template);

  m_targetLocationId = static_cast<TrunkPort*>(_template)->m_targetLocationId;
}

bool TrunkPort::PerformDepthSort(LegacyVector3& _centerPos)
{
  _centerPos = m_centerPos;

  return true;
}

void TrunkPort::ReprogramComplete()
{
  GlobalLocation* location = g_app->m_globalWorld->GetLocation(m_targetLocationId);
  if (location)
  {
    location->m_available = true;

    // Look for a "receiver" trunk port and set that to the same state
    for (int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i)
    {
      GlobalBuilding* building = g_app->m_globalWorld->m_buildings[i];
      if (building->m_type == TypeTrunkPort && building->m_locationId == m_targetLocationId && building->m_link == g_app->m_locationId)
        building->m_online = true;
    }
  }

  Building::ReprogramComplete();
}

void TrunkPort::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  if (_in->TokenAvailable())
    m_targetLocationId = atoi(_in->GetNextToken());
}

void TrunkPort::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_targetLocationId);
}
