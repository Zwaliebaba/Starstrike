#include "pch.h"
#include "researchitem.h"
#include "GameSimEventQueue.h"
#include "camera.h"
#include "file_writer.h"
#include "GameContext.h"
#include "global_world.h"
#include "globals.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "preferences.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "taskmanager_interface.h"
#include "text_stream_readers.h"

ResearchItem::ResearchItem()
  : Building(),
    m_reprogrammed(100.0f),
    m_end1(nullptr),
    m_end2(nullptr),
    m_researchType(-1),
    m_level(1),
    m_inLibrary(false)
{
  m_type = TypeResearchItem;
  m_researchType = GlobalResearch::TypeEngineer;

  Building::SetShape(Resource::GetShapeStatic("researchitem.shp"));

  m_front.RotateAroundY(frand(2.0f * M_PI));

  m_end1 = m_shape->GetMarkerData("MarkerGrab1");
  m_end2 = m_shape->GetMarkerData("MarkerGrab2");
}

void ResearchItem::Initialise(Building* _template)
{
  Building::Initialise(_template);

  m_researchType = static_cast<ResearchItem*>(_template)->m_researchType;
  m_level = static_cast<ResearchItem*>(_template)->m_level;
}

void ResearchItem::SetDetail([[maybe_unused]] int _detail)
{
  m_pos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  m_pos.y += 20.0f;

  Matrix34 mat(m_front, m_up, m_pos);
  m_centerPos = m_shape->CalculateCenter(mat);
  m_radius = m_shape->CalculateRadius(mat, m_centerPos);
}

bool ResearchItem::Advance()
{
  if (m_vel.Mag() > 1.0f)
  {
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    m_vel *= (1.0f - SERVER_ADVANCE_PERIOD * 0.5f);

    Matrix34 mat(m_front, g_upVector, m_pos);
    m_centerPos = m_shape->CalculateCenter(mat);
  }
  else
    m_vel.Zero();

  if (m_researchType > -1 && g_context->m_globalWorld->m_research->HasResearch(m_researchType) && g_context->m_globalWorld->m_research->
    CurrentLevel(m_researchType) >= m_level)
    return true;

  if (m_reprogrammed <= 0.0f)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, mat, 1.0f));

    int existingLevel = g_context->m_globalWorld->m_research->CurrentLevel(m_researchType);

    g_context->m_globalWorld->m_research->AddResearch(m_researchType);
    g_context->m_globalWorld->m_research->m_researchLevel[m_researchType] = m_level;

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "AquireResearch"));

    if (existingLevel == 0)
      g_context->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageResearch, m_researchType, 4.0f);
    else
      g_context->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageResearchUpgrade, m_researchType, 4.0f);

    return true;
  }

  return false;
}

bool ResearchItem::NeedsReprogram() { return (m_reprogrammed > 0.0f); }

bool ResearchItem::Reprogram()
{
  m_reprogrammed -= SERVER_ADVANCE_PERIOD * 3.0f;

  return (m_reprogrammed <= 0.0f);
}

void ResearchItem::GetEndPositions(LegacyVector3& _end1, LegacyVector3& _end2)
{
  Matrix34 mat(m_front, m_up, m_pos);

  _end1 = m_shape->GetMarkerWorldMatrix(m_end1, mat).pos;
  _end2 = m_shape->GetMarkerWorldMatrix(m_end2, mat).pos;
}

void ResearchItem::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_researchType = GlobalResearch::GetType(_in->GetNextToken());

  if (_in->TokenAvailable())
    m_level = atoi(_in->GetNextToken());
}

void ResearchItem::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%s ", GlobalResearch::GetTypeName(m_researchType));
  _out->printf("%6d", m_level);
}

bool ResearchItem::DoesSphereHit(const LegacyVector3& _pos, float _radius) { return false; }

bool ResearchItem::DoesShapeHit(ShapeStatic* _shape, Matrix34 _transform) { return false; }

bool ResearchItem::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                              LegacyVector3* norm) { return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen); }

bool ResearchItem::IsInView()
{
  if (Building::IsInView())
    return true;

  if (g_context->m_camera->PosInViewFrustum(m_pos + LegacyVector3(0, g_context->m_camera->GetPos().y, 0)))
    return true;

  return false;
}
