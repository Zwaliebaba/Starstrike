#include "pch.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "debug_utils.h"
#include "file_writer.h"
#include "hi_res_time.h"
#include "math_utils.h"
#include "resource.h"
#include "shape.h"
#include "text_stream_readers.h"
#include "preferences.h"
#include "language_table.h"
#include "spiritreceiver.h"
#include "app.h"
#include "location.h"
#include "camera.h"
#include "global_world.h"
#include "particle_system.h"
#include "renderer.h"
#include "soundsystem.h"

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
    m_spiritLocation = m_shape->m_rootFragment->LookupMarker("MarkerSpiritLink");
    DarwiniaDebugAssert(m_spiritLocation);
  }

  Matrix34 rootMat(m_front, m_up, m_pos);
  Matrix34 worldPos = m_spiritLocation->GetWorldMatrix(rootMat);
  return worldPos.pos;
}

bool ReceiverBuilding::IsInView()
{
  Building* spiritLink = g_app->m_location->GetBuilding(m_spiritLink);

  if (spiritLink)
  {
    LegacyVector3 midPoint = (spiritLink->m_centrePos + m_centrePos) / 2.0f;
    float radius = (spiritLink->m_centrePos - m_centrePos).Mag() / 2.0f;
    radius += m_radius;
    return (g_app->m_camera->SphereInViewFrustum(midPoint, radius));
  }
  return Building::IsInView();
}

void ReceiverBuilding::ListSoundEvents(LList<char*>* _list)
{
  Building::ListSoundEvents(_list);

  _list->PutData("TriggerSpirit");
}

void ReceiverBuilding::Render(float _predictionTime)
{
  Matrix34 mat(m_front, m_up, m_pos);
  m_shape->Render(_predictionTime, mat);
}

void ReceiverBuilding::RenderAlphas(float _predictionTime)
{
  Building::RenderAlphas(_predictionTime);

  _predictionTime -= 0.1f;

  Building* spiritLink = g_app->m_location->GetBuilding(m_spiritLink);

  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);

  if (spiritLink)
  {
    //
    // Render the spirit line itself

    auto receiverBuilding = static_cast<ReceiverBuilding*>(spiritLink);

    LegacyVector3 ourPos = GetSpiritLocation();
    LegacyVector3 theirPos = receiverBuilding->GetSpiritLocation();

    LegacyVector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
    LegacyVector3 ourPosRight = camToOurPos ^ (theirPos - ourPos);

    LegacyVector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
    LegacyVector3 theirPosRight = camToTheirPos ^ (theirPos - ourPos);

    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
    g_imRenderer->Color4f(0.9f, 0.9f, 0.5f, 1.0f);

    float size = 0.5f;

    if (buildingDetail == 1)
    {
      g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/laser.bmp"));

      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

      size = 1.0f;
    }

    ourPosRight.SetLength(size);
    theirPosRight.SetLength(size);

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2f(0.1f, 0);
    g_imRenderer->Vertex3fv((ourPos - ourPosRight).GetData());
    g_imRenderer->TexCoord2f(0.1f, 1);
    g_imRenderer->Vertex3fv((ourPos + ourPosRight).GetData());
    g_imRenderer->TexCoord2f(0.9f, 1);
    g_imRenderer->Vertex3fv((theirPos + theirPosRight).GetData());
    g_imRenderer->TexCoord2f(0.9f, 0);
    g_imRenderer->Vertex3fv((theirPos - theirPosRight).GetData());
    g_imRenderer->End();


    g_imRenderer->UnbindTexture();

    //
    // Render any surges

    BeginRenderUnprocessedSpirits();
    for (int i = 0; i < m_spirits.Size(); ++i)
    {
      float thisSpirit = m_spirits[i];
      thisSpirit += _predictionTime * 0.8f;
      if (thisSpirit < 0.0f)
        thisSpirit = 0.0f;
      if (thisSpirit > 1.0f)
        thisSpirit = 1.0f;
      LegacyVector3 thisSpiritPos = ourPos + (theirPos - ourPos) * thisSpirit;
      RenderUnprocessedSpirit(thisSpiritPos, 1.0f);
    }
    EndRenderUnprocessedSpirits();
  }
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
  g_app->m_soundSystem->TriggerBuildingEvent(this, "TriggerSpirit");
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

static float s_nearPlaneStart;

void ReceiverBuilding::BeginRenderUnprocessedSpirits()
{
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);

  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
  if (buildingDetail == 1)
    g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/glow.bmp"));

  s_nearPlaneStart = g_app->m_renderer->GetNearPlane();
  g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart * 1.1f, g_app->m_renderer->GetFarPlane());
}

void ReceiverBuilding::RenderUnprocessedSpirit(const LegacyVector3& _pos, float _life)
{
  LegacyVector3 position = _pos;
  LegacyVector3 camUp = g_app->m_camera->GetUp();
  LegacyVector3 camRight = g_app->m_camera->GetRight();
  float scale = 2.0f * _life;
  float alphaValue = _life;

  g_imRenderer->Color4f(0.6f, 0.2f, 0.1f, alphaValue);
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex3fv((position + camUp * 3 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex3fv((position + camRight * 3 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex3fv((position - camUp * 3 * scale).GetData());
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex3fv((position - camRight * 3 * scale).GetData());
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex3fv((position + camUp * 3 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex3fv((position + camRight * 3 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex3fv((position - camUp * 3 * scale).GetData());
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex3fv((position - camRight * 3 * scale).GetData());
  g_imRenderer->End();


  g_imRenderer->Color4f(0.6f, 0.2f, 0.1f, alphaValue);
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex3fv((position + camUp * 1 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex3fv((position + camRight * 1 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex3fv((position - camUp * 1 * scale).GetData());
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex3fv((position - camRight * 1 * scale).GetData());
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex3fv((position + camUp * 1 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex3fv((position + camRight * 1 * scale).GetData());
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex3fv((position - camUp * 1 * scale).GetData());
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex3fv((position - camRight * 1 * scale).GetData());
  g_imRenderer->End();


  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
  if (buildingDetail == 1)
  {
    g_imRenderer->Color4f(0.6f, 0.2f, 0.1f, alphaValue / 1.5f);
    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex3fv((position + camUp * 30 * scale).GetData());
    g_imRenderer->TexCoord2i(1, 0);
    g_imRenderer->Vertex3fv((position + camRight * 30 * scale).GetData());
    g_imRenderer->TexCoord2i(1, 1);
    g_imRenderer->Vertex3fv((position - camUp * 30 * scale).GetData());
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex3fv((position - camRight * 30 * scale).GetData());
    g_imRenderer->End();

    g_imRenderer->UnbindTexture();
  }
}

void ReceiverBuilding::RenderUnprocessedSpirit_basic(const LegacyVector3& _pos, float _life)
{
  LegacyVector3 position = _pos;
  LegacyVector3 camUp = g_app->m_camera->GetUp();
  LegacyVector3 camRight = g_app->m_camera->GetRight();
  float scale = 2.0f * _life;
  float alphaValue = _life;

  g_imRenderer->Color4f(0.6f, 0.2f, 0.1f, alphaValue);

  g_imRenderer->Color4f(0.6f, 0.2f, 0.1f, alphaValue);
}

void ReceiverBuilding::RenderUnprocessedSpirit_detail(const LegacyVector3& _pos, float _life)
{
  LegacyVector3 position = _pos;
  LegacyVector3 camUp = g_app->m_camera->GetUp();
  LegacyVector3 camRight = g_app->m_camera->GetRight();
  float scale = 2.0f * _life;
  float alphaValue = _life;

  g_imRenderer->Color4f(0.6f, 0.2f, 0.1f, alphaValue / 1.5f);
}

void ReceiverBuilding::EndRenderUnprocessedSpirits()
{
  g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart, g_app->m_renderer->GetFarPlane());

  g_imRenderer->UnbindTexture();
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
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
  SetShape(g_app->m_resource->GetShape("spiritprocessor.shp"));
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

char* SpiritProcessor::GetObjectiveCounter()
{
  static char result[256];
  sprintf(result, "%s : %2.2f", LANGUAGEPHRASE("objective_throughput"), m_throughput);
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

void SpiritProcessor::Render(float _predictionTime)
{
  ReceiverBuilding::Render(_predictionTime);

  //g_gameFont.DrawText3DCentre( m_pos + LegacyVector3(0,215,0), 10.0f, "NumThisSecond : %d", m_numThisSecond );
  //g_gameFont.DrawText3DCentre( m_pos + LegacyVector3(0,200,0), 10.0f, "Throughput    : %2.2f", m_throughput );
}

void SpiritProcessor::RenderAlphas(float _predictionTime)
{
  ReceiverBuilding::RenderAlphas(_predictionTime);

  //
  // Render all floating spirits

  BeginRenderUnprocessedSpirits();

  _predictionTime -= SERVER_ADVANCE_PERIOD;

  for (int i = 0; i < m_floatingSpirits.Size(); ++i)
  {
    UnprocessedSpirit* spirit = m_floatingSpirits[i];
    LegacyVector3 pos = spirit->m_pos;
    pos += spirit->m_vel * _predictionTime;
    pos += spirit->m_hover * _predictionTime;
    float life = spirit->GetLife();
    RenderUnprocessedSpirit(pos, life);
  }
  EndRenderUnprocessedSpirits();
}

// ****************************************************************************
// Class ReceiverLink
// ****************************************************************************

ReceiverLink::ReceiverLink()
  : ReceiverBuilding()
{
  m_type = TypeReceiverLink;
  SetShape(g_app->m_resource->GetShape("receiverlink.shp"));
}

bool ReceiverLink::Advance() { return ReceiverBuilding::Advance(); }

// ****************************************************************************
// Class ReceiverSpiritSpawner
// ****************************************************************************

ReceiverSpiritSpawner::ReceiverSpiritSpawner()
  : ReceiverBuilding()
{
  m_type = TypeReceiverSpiritSpawner;
  SetShape(g_app->m_resource->GetShape("receiverlink.shp"));
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
  SetShape(g_app->m_resource->GetShape("spiritreceiver.shp"));
  m_headMarker = m_shape->m_rootFragment->LookupMarker("MarkerHead");

  for (int i = 0; i < SPIRITRECEIVER_NUMSTATUSMARKERS; ++i)
  {
    char name[64];
    sprintf(name, "MarkerStatus0%d", i + 1);
    m_statusMarkers[i] = m_shape->m_rootFragment->LookupMarker(name);
  }

  m_headShape = g_app->m_resource->GetShape("spiritreceiverhead.shp");
  m_spiritLink = m_headShape->m_rootFragment->LookupMarker("MarkerSpiritLink");
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

void SpiritReceiver::Render(float _predictionTime)
{
  if (g_app->m_editing)
  {
    m_up = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
    LegacyVector3 right(1, 0, 0);
    m_front = right ^ m_up;
  }

  ReceiverBuilding::Render(_predictionTime);

  Matrix34 mat(m_front, m_up, m_pos);
  LegacyVector3 headPos = m_headMarker->GetWorldMatrix(mat).pos;
  LegacyVector3 up = g_upVector;
  LegacyVector3 right(1, 0, 0);
  LegacyVector3 front = up ^ right;
  Matrix34 headMat(front, up, headPos);
  m_headShape->Render(_predictionTime, headMat);
}

LegacyVector3 SpiritReceiver::GetSpiritLocation()
{
  Matrix34 mat(m_front, m_up, m_pos);
  LegacyVector3 headPos = m_headMarker->GetWorldMatrix(mat).pos;
  LegacyVector3 up = g_upVector;
  LegacyVector3 right(1, 0, 0);
  LegacyVector3 front = up ^ right;
  Matrix34 headMat(front, up, headPos);
  LegacyVector3 spiritLinkPos = m_spiritLink->GetWorldMatrix(headMat).pos;
  return spiritLinkPos;
}

void SpiritReceiver::RenderPorts()
{
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/starburst.bmp"));
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    Matrix34 rootMat(m_front, m_up, m_pos);
    Matrix34 worldMat = m_statusMarkers[i]->GetWorldMatrix(rootMat);

    //
    // Render the status light

    float size = 6.0f;
    LegacyVector3 camR = g_app->m_camera->GetRight() * size;
    LegacyVector3 camU = g_app->m_camera->GetUp() * size;

    LegacyVector3 statusPos = worldMat.pos;

    if (GetPortOccupant(i).IsValid())
    {
      g_imRenderer->Color4f(0.3f, 1.0f, 0.3f, 1.0f);
    }
    else
    {
      g_imRenderer->Color4f(1.0f, 0.3f, 0.3f, 1.0f);
    }

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex3fv((statusPos - camR - camU).GetData());
    g_imRenderer->TexCoord2i(1, 0);
    g_imRenderer->Vertex3fv((statusPos + camR - camU).GetData());
    g_imRenderer->TexCoord2i(1, 1);
    g_imRenderer->Vertex3fv((statusPos + camR + camU).GetData());
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex3fv((statusPos - camR + camU).GetData());
    g_imRenderer->End();

  }

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
  g_imRenderer->UnbindTexture();
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
}

void SpiritReceiver::RenderAlphas(float _predictionTime)
{
  ReceiverBuilding::RenderAlphas(_predictionTime);

  //RenderHitCheck();

  float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());
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
