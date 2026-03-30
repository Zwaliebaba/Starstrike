#include "pch.h"
#include "math_utils.h"
#include "file_writer.h"
#include "matrix33.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "text_stream_readers.h"
#include "preferences.h"
#include "main.h"
#include "GameContext.h"
#include "location.h"
#include "obstruction_grid.h"
#include "team.h"
#include "GameSimEventQueue.h"
#include "building.h"
#include "laserfence.h"

LaserFence::LaserFence()
  : Building(),
    m_status(0.0f),
    m_nextLaserFenceId(-1),
    m_sparkTimer(0.0f),
    m_radiusSet(false),
    m_marker1(nullptr),
    m_marker2(nullptr),
    m_nextToggled(false),
    m_mode(ModeDisabled),
    m_scale(1.0f)
{
  m_type = TypeLaserFence;

  SetShape(Resource::GetShapeStatic("laserfence.shp"));

  m_marker1 = m_shape->GetMarkerData("MarkerFence01");
  m_marker2 = m_shape->GetMarkerData("MarkerFence02");

  ASSERT_TEXT(m_marker1, "MarkerFence01 not found in laserfence.shp");
  ASSERT_TEXT(m_marker2, "MarkerFence02 not found in laserfence.shp");
}

void LaserFence::Initialise(Building* _template)
{
  Building::Initialise(_template);
  DEBUG_ASSERT(_template->m_type == Building::TypeLaserFence);

  m_nextLaserFenceId = static_cast<LaserFence*>(_template)->m_nextLaserFenceId;
  m_scale = static_cast<LaserFence*>(_template)->m_scale;
  m_mode = static_cast<LaserFence*>(_template)->m_mode;

  m_sparkTimer = frand(10.0f);
}

void LaserFence::SetDetail(int _detail)
{
  m_radiusSet = false;
  Building::SetDetail(_detail);
}

void LaserFence::Spark()
{
  LegacyVector3 sparkPos = m_pos;
  sparkPos.y += frand(m_scale * 50.0f);

  auto nextFence = static_cast<LaserFence*>(g_context->m_location->GetBuilding(m_nextLaserFenceId));

  int numSparks = 5.0f + frand(5.0f);
  for (int i = 0; i < numSparks; ++i)
  {
    LegacyVector3 particleVel;
    if (nextFence)
      particleVel = (m_pos - nextFence->m_pos) ^ g_upVector;
    else
      particleVel = LegacyVector3(sfrand(10.0f), sfrand(5.0f), sfrand(10.0f));

    particleVel.SetLength(40.0f + frand(20.0f));
    particleVel += LegacyVector3(frand() * 20.0f, sfrand() * 20.0f, sfrand() * 20.0f);
    float size = 25.0f + frand(25.0f);
    g_simEventQueue.Push(SimEvent::MakeParticle(sparkPos, particleVel, SimParticle::TypeSpark, size));
  }

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Spark"));
}

bool LaserFence::Advance()
{
  if (!m_radiusSet)
  {
    Building* building = g_context->m_location->GetBuilding(m_nextLaserFenceId);
    if (building)
    {
      m_centerPos = (building->m_pos + m_pos) / 2.0f;
      m_radius = (building->m_pos - m_pos).Mag() / 2.0f + m_radius;
    }
    m_radiusSet = true;
  }

  if (m_mode != ModeDisabled)
  {
    m_sparkTimer -= SERVER_ADVANCE_PERIOD;
    if (m_sparkTimer < 0.0f)
    {
      m_sparkTimer = 8.0f + syncfrand(4.0f);
      Spark();
    }
  }

  switch (m_mode)
  {
  case ModeEnabling:
    m_status += LASERFENCE_RAISESPEED * SERVER_ADVANCE_PERIOD;
    if (m_status >= 0.5f && m_nextLaserFenceId != -1 && !m_nextToggled)
    {
      auto nextFence = static_cast<LaserFence*>(g_context->m_location->GetBuilding(m_nextLaserFenceId));
      if (nextFence)
      {
        nextFence->Enable();
        m_nextToggled = true;
      }
    }
    if (m_status >= 1.0f)
    {
      m_status = 1.0f;
      m_mode = ModeEnabled;
      if (m_nextLaserFenceId == -1)
        g_context->m_location->m_obstructionGrid->CalculateAll();
    }
    break;

  case ModeDisabling:
    if (m_status <= 0.5f && m_nextLaserFenceId != -1 && !m_nextToggled)
    {
      auto nextFence = static_cast<LaserFence*>(g_context->m_location->GetBuilding(m_nextLaserFenceId));
      if (nextFence)
      {
        nextFence->Disable();
        m_nextToggled = true;
      }
    }
    m_status -= LASERFENCE_RAISESPEED * SERVER_ADVANCE_PERIOD;
    if (m_status <= 0.0f)
    {
      m_status = 0.0f;
      m_mode = ModeDisabled;
      if (m_nextLaserFenceId == -1)
        g_context->m_location->m_obstructionGrid->CalculateAll();
    }
    break;

  case ModeEnabled:
    {
      if (m_status < 1.0f)
      {
        m_status = 1.0f;
        if (m_nextLaserFenceId == -1)
          g_context->m_location->m_obstructionGrid->CalculateAll();
      }
      break;
    }
  case ModeDisabled:
    m_status = 0.0f;
    break;
  }

  return Building::Advance();
}

float LaserFence::GetFenceFullHeight()
{
  Matrix34 mat(m_front, g_upVector, m_pos);
  mat.u *= m_scale;
  mat.r *= m_scale;
  mat.f *= m_scale;
  LegacyVector3 marker1 = m_shape->GetMarkerWorldMatrix(m_marker1, mat).pos;
  LegacyVector3 marker2 = m_shape->GetMarkerWorldMatrix(m_marker2, mat).pos;

  return (marker2.y - marker1.y);
}

bool LaserFence::IsInView()
{
  return Building::IsInView();

  // No need to do the usual midpoint / radius calculation here
  // as it has already been done in LaserFence::Advance.
  // Our midpoint and radius values are frigged in order to
  // correctly fill the obstruction grid.
}

bool LaserFence::PerformDepthSort(LegacyVector3& _centerPos)
{
  if (m_mode == ModeDisabled)
    return false;

  if (m_nextLaserFenceId != -1)
  {
    _centerPos = m_centerPos;
    return true;
  }

  return false;
}

void LaserFence::Enable()
{
  if (m_mode != ModeEnabled && m_mode != ModeNeverOn)
    m_mode = ModeEnabling;

  if (m_mode == ModeNeverOn)
  {
    auto nextFence = static_cast<LaserFence*>(g_context->m_location->GetBuilding(m_nextLaserFenceId));
    if (nextFence)
      nextFence->Toggle();
  }

  m_nextToggled = false;
}

void LaserFence::Disable()
{
  if (m_mode != ModeDisabled && m_mode != ModeNeverOn)
    m_mode = ModeDisabling;

  if (m_mode == ModeNeverOn)
  {
    auto nextFence = static_cast<LaserFence*>(g_context->m_location->GetBuilding(m_nextLaserFenceId));
    if (nextFence)
      nextFence->Toggle();
  }

  m_nextToggled = false;
}

void LaserFence::Toggle()
{
  switch (m_mode)
  {
  case ModeDisabled:
  case ModeDisabling:
    Enable();
    break;

  case ModeEnabled:
  case ModeEnabling:
    Disable();
    break;
  }

  m_nextToggled = false;
}

bool LaserFence::IsEnabled() { return (m_mode == ModeEnabled) || (m_mode == ModeEnabling); }

void LaserFence::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  char* word = _in->GetNextToken();
  m_nextLaserFenceId = atoi(word);

  if (_in->TokenAvailable())
    m_scale = atof(_in->GetNextToken());
  if (_in->TokenAvailable())
    m_mode = atof(_in->GetNextToken());
}

void LaserFence::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_nextLaserFenceId);
  _out->printf("%-6.2f", m_scale);
  _out->printf("%d", m_mode);
}

int LaserFence::GetBuildingLink() { return m_nextLaserFenceId; }

void LaserFence::SetBuildingLink(int _buildingId) { m_nextLaserFenceId = _buildingId; }

void LaserFence::Electrocute([[maybe_unused]] const LegacyVector3& _pos) { g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Electrocute")); }

bool LaserFence::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  if (m_mode == ModeDisabled || g_context->m_editing)
  {
    SpherePackage sphere(_pos, _radius);
    Matrix34 transform(m_front, m_up, m_pos);
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;
    return m_shape->SphereHit(&sphere, transform);
  }

  //
  // Fake this for now using 2 ray hits
  // One running across the sphere in the x direction, the other in the z direction

  LegacyVector3 ray1Start = _pos - LegacyVector3(_radius, 0, 0);
  LegacyVector3 ray1Dir(1, 0, 0);
  float ray1Len = _radius * 2;
  if (DoesRayHit(ray1Start, ray1Dir, ray1Len))
    return true;

  LegacyVector3 ray2Start = _pos - LegacyVector3(0, 0, _radius);
  LegacyVector3 ray2Dir(0, 0, 1);
  float ray2Len = _radius * 2;
  if (DoesRayHit(ray2Start, ray2Dir, ray2Len))
    return true;

  //
  // Fence is ok
  // Test against the shape

  SpherePackage sphere(_pos, _radius);
  Matrix34 transform(m_front, m_up, m_pos);
  transform.f *= m_scale;
  transform.u *= m_scale;
  transform.r *= m_scale;
  return m_shape->SphereHit(&sphere, transform);
}

bool LaserFence::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                            LegacyVector3* _norm)
{
  if (m_mode == ModeDisabled || g_context->m_editing)
  {
    RayPackage ray(_rayStart, _rayDir, _rayLen);
    Matrix34 transform(m_front, m_up, m_pos);
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;
    return m_shape->RayHit(&ray, transform, true);
  }

  if (m_nextLaserFenceId != -1 && m_status > 0.0f)
  {
    Building* nextFence = g_context->m_location->GetBuilding(m_nextLaserFenceId);
    float maxHeight = GetFenceFullHeight();
    float fenceHeight = maxHeight * m_status;
    LegacyVector3 pos1 = m_pos - LegacyVector3(0, fenceHeight / 3, 0);
    LegacyVector3 pos2 = m_pos + LegacyVector3(0, fenceHeight, 0);
    LegacyVector3 pos3 = nextFence->m_pos - LegacyVector3(0, fenceHeight / 3, 0);
    LegacyVector3 pos4 = nextFence->m_pos + LegacyVector3(0, fenceHeight, 0);

    bool hitTri1 = false;
    bool hitTri2 = false;

    hitTri1 = RayTriIntersection(_rayStart, _rayDir, pos1, pos2, pos4, _rayLen, _pos);
    if (!hitTri1)
      hitTri2 = RayTriIntersection(_rayStart, _rayDir, pos4, pos3, pos1, _rayLen, _pos);

    if (hitTri1 || hitTri2)
    {
      if (_norm)
      {
        LegacyVector3 v1 = pos1 - pos2;
        LegacyVector3 v2 = pos1 - pos3;
        *_norm = (v2 ^ v1).Normalise();
      }
      return true;
    }
  }

  //return Building::DoesRayHit( _rayStart, _rayDir, _rayLen, _pos, _norm );

  RayPackage ray(_rayStart, _rayDir, _rayLen);
  Matrix34 transform(m_front, m_up, m_pos);
  transform.f *= m_scale;
  transform.u *= m_scale;
  transform.r *= m_scale;
  return m_shape->RayHit(&ray, transform, true);
}

bool LaserFence::DoesShapeHit(ShapeStatic* _shape, Matrix34 _transform)
{
  return DoesSphereHit(_transform.pos, _shape->m_rootFragment->m_radius);
}

LegacyVector3 LaserFence::GetTopPosition()
{
  Matrix34 mat(m_front, g_upVector, m_pos);
  mat.u *= m_scale;
  mat.r *= m_scale;
  mat.f *= m_scale;
  return m_shape->GetMarkerWorldMatrix(m_marker2, mat).pos;
}
