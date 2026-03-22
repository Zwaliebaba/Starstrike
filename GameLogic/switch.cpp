#include "pch.h"
#include "file_writer.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "text_stream_readers.h"
#include "clienttoserver.h"
#include "GameAppSim.h"
#include "location.h"
#include "camera.h"
#include "global_world.h"
#include "script.h"
#include "building.h"
#include "laserfence.h"
#include "switch.h"

// ****************************************************************************
// Class FenceSwitch
// ****************************************************************************

// *** Constructor
FenceSwitch::FenceSwitch()
  : Building(),
    m_linkedBuildingId(-1),
    m_linkedBuildingId2(-1),
    m_switchable(false),
    m_timer(20.0f),
    m_connectionLocation(nullptr),
    m_locked(false),
    m_lockable(0),
    m_switchValue(-1)
{
  m_type = TypeFenceSwitch;
  SetShape(g_app->m_resource->GetShapeStatic("fenceswitch.shp"));
  strncpy(m_script, "none", sizeof(m_script));
  m_script[sizeof(m_script) - 1] = '\0';
}

// *** Initialise
void FenceSwitch::Initialise(Building* _template)
{
  Building::Initialise(_template);
  DEBUG_ASSERT(_template->m_type == Building::TypeFenceSwitch);
  m_linkedBuildingId = static_cast<FenceSwitch*>(_template)->m_linkedBuildingId;
  m_linkedBuildingId2 = static_cast<FenceSwitch*>(_template)->m_linkedBuildingId2;
  m_switchValue = static_cast<FenceSwitch*>(_template)->m_switchValue;
  m_lockable = static_cast<FenceSwitch*>(_template)->m_lockable;
  m_locked = static_cast<FenceSwitch*>(_template)->m_locked;
  strncpy(m_script, static_cast<FenceSwitch*>(_template)->m_script, sizeof(m_script));
  m_script[sizeof(m_script) - 1] = '\0';
}

void FenceSwitch::SetDetail(int _detail)
{
  if (m_timer < 10.0f)
    m_timer = 10.0f;
  Building::SetDetail(_detail);
}

// *** Advance
bool FenceSwitch::Advance()
{
  if (m_timer > 0.0f)
  {
    m_timer -= SERVER_ADVANCE_PERIOD;
    return Building::Advance();
  }

  bool friendly = true;
  for (int i = 0; i < GetNumPorts(); ++i)
  {
    if (m_ports[i]->m_occupant.GetTeamId() == 1)
    {
      friendly = false;
      break;
    }
  }

  if (friendly && GetNumPorts() == GetNumPortsOccupied())
    SetTeamId(2);
  else
    SetTeamId(1);

  Building* b = g_app->m_location->GetBuilding(m_linkedBuildingId);

  bool switched = false;

  if (!m_locked)
  {
    if (b->m_type == TypeLaserFence)
    {
      auto fence = static_cast<LaserFence*>(b);

      if (GetNumPorts() == GetNumPortsOccupied())
      {
        GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
        if (gb && !gb->m_online)
        {
          if (friendly)
          {
            m_switchable = true;
            if (m_switchValue == m_linkedBuildingId)
            {
              if (!fence->IsEnabled())
              {
                fence->Enable();
                switched = true;
              }

              Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
              if (b2 && b2->m_type == TypeLaserFence)
              {
                auto fence2 = static_cast<LaserFence*>(b2);
                if (fence2->IsEnabled())
                  fence2->Disable();
              }
            }
            else
            {
              if (fence->IsEnabled())
              {
                fence->Disable();
                switched = true;
              }

              Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
              if (b2 && b2->m_type == TypeLaserFence)
              {
                auto fence2 = static_cast<LaserFence*>(b2);
                if (!fence2->IsEnabled())
                  fence2->Enable();
              }
            }
          }
          else
          {
            m_switchable = false;
            m_switchValue = m_linkedBuildingId;

            if (!fence->IsEnabled())
            {
              fence->Enable();
              switched = true;
            }

            Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
            if (b2 && b2->m_type == TypeLaserFence)
            {
              auto fence2 = static_cast<LaserFence*>(b2);
              if (fence2->IsEnabled())
                fence2->Disable();
            }
          }
          gb->m_online = true;
          g_app->m_globalWorld->EvaluateEvents();
        }
      }
      else
      {
        GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
        if (gb && gb->m_online)
        {
          m_switchValue = m_linkedBuildingId;
          m_switchable = false;

          if (m_linkedBuildingId2 == -1)
          {
            if (fence->IsEnabled())
            {
              fence->Disable();
              switched = true;
            }
          }
          else
          {
            if (!fence->IsEnabled())
            {
              fence->Enable();
              switched = true;
            }

            Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
            if (b2 && b2->m_type == TypeLaserFence)
            {
              auto fence2 = static_cast<LaserFence*>(b2);
              if (fence2->IsEnabled())
                fence2->Disable();
            }
          }
          gb->m_online = false;
          g_app->m_globalWorld->EvaluateEvents();
        }
      }
    }
  }

  if (switched)
  {
    if (strstr(m_script, ".txt"))
      g_app->m_script->RunScript(m_script);
    if (m_lockable)
      m_locked = true;
  }

  return Building::Advance();
}

// *** GetBuildingLink
int FenceSwitch::GetBuildingLink() { return m_linkedBuildingId; }

// *** SetBuildingLink
void FenceSwitch::SetBuildingLink(int _buildingId)
{
  if (m_linkedBuildingId == -1)
    m_linkedBuildingId = _buildingId;
  else if (m_linkedBuildingId2 == -1)
    m_linkedBuildingId2 = _buildingId;
  else
    m_linkedBuildingId = _buildingId;
}

// *** Read
void FenceSwitch::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_linkedBuildingId = atoi(_in->GetNextToken());
  m_linkedBuildingId2 = atoi(_in->GetNextToken());
  m_switchValue = atoi(_in->GetNextToken());
  m_lockable = atoi(_in->GetNextToken());
  strncpy(m_script, _in->GetNextToken(), sizeof(m_script));
  m_script[sizeof(m_script) - 1] = '\0';
  if (_in->TokenAvailable())
  {
    int locked = atoi(_in->GetNextToken());
    if (locked == 1)
      m_locked = true;
  }

  if (m_switchValue == -1)
    m_switchValue = m_linkedBuildingId;
}

// *** Write
void FenceSwitch::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_linkedBuildingId);
  _out->printf("%-8d", m_linkedBuildingId2);
  _out->printf("%-8d", m_switchValue);

  _out->printf("%-3d", m_lockable);

  _out->printf("%s  ", m_script);

  int locked = m_locked ? 1 : 0;
  _out->printf("%-3d", locked);
}

void FenceSwitch::Switch()
{
  if (!m_switchable)
    return;

  if ((m_lockable == 1 && !m_locked) || !m_lockable)
  {
    if (m_switchValue == m_linkedBuildingId)
      m_switchValue = m_linkedBuildingId2;
    else
      m_switchValue = m_linkedBuildingId;

    GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
    gb->m_online = false;
    g_app->m_globalWorld->EvaluateEvents();
    /*if( strstr( m_script, ".txt" ) )
    {
        g_app->m_script->RunScript( m_script );
    }*/
  }
}

LegacyVector3 FenceSwitch::GetConnectionLocation()
{
  if (!m_connectionLocation)
  {
    m_connectionLocation = m_shape->GetMarkerData("MarkerConnectionLocation");
    DEBUG_ASSERT(m_connectionLocation);
  }

  Matrix34 rootMat(m_front, m_up, m_pos);
  Matrix34 worldPos = m_shape->GetMarkerWorldMatrix(m_connectionLocation, rootMat);
  return worldPos.pos;
}

bool FenceSwitch::IsInView()
{
  if (m_linkedBuildingId != -1)
  {
    LegacyVector3 startPoint = m_pos;
    Building* b = g_app->m_location->GetBuilding(m_linkedBuildingId);
    if (b)
    {
      LegacyVector3 endPoint = b->m_pos;
      LegacyVector3 midPoint = (startPoint + endPoint) / 2.0f;

      float radius = (startPoint - endPoint).Mag() / 2.0f;
      radius += m_radius;

      if (g_app->m_camera->SphereInViewFrustum(midPoint, radius))
        return true;
    }
  }

  if (m_linkedBuildingId2 != -1)
  {
    LegacyVector3 startPoint = m_pos;
    Building* b = g_app->m_location->GetBuilding(m_linkedBuildingId2);
    if (b)
    {
      LegacyVector3 endPoint = b->m_pos;
      LegacyVector3 midPoint = (startPoint + endPoint) / 2.0f;

      float radius = (startPoint - endPoint).Mag() / 2.0f;
      radius += m_radius;

      if (g_app->m_camera->SphereInViewFrustum(midPoint, radius))
        return true;
    }
  }

  return Building::IsInView();
}
