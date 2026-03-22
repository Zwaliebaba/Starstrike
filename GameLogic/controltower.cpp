#include "pch.h"
#include "file_writer.h"
#include "matrix34.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "text_stream_readers.h"
#include "math_utils.h"
#include "hi_res_time.h"
#include "clienttoserver.h"
#include "GameSimEventQueue.h"
#include "GameAppSim.h"
#include "camera.h"
#include "location.h"
#include "team.h"
#include "main.h"
#include "global_world.h"
#include "controltower.h"
#include "trunkport.h"

ShapeStatic* ControlTower::s_dishShape = nullptr;

ControlTower::ControlTower()
  : Building(),
    m_ownership(100.0f),
    m_controlBuildingId(-1),
    m_checkTargetTimer(0.0f)
{
  m_radius = 4.0f;
  m_type = TypeControlTower;

  Building::SetShape(g_app->m_resource->GetShapeStatic("controltower.shp"));
  m_lightPos = m_shape->GetMarkerData("MarkerLightPos");
  m_dishPos = m_shape->GetMarkerData("MarkerDishPos");

  for (int i = 0; i < 3; ++i)
  {
    m_beingReprogrammed[i] = false;
    char markerName[64];

    snprintf(markerName, sizeof(markerName), "MarkerReprogrammer%d", i);
    m_reprogrammer[i] = m_shape->GetMarkerData(markerName);

    snprintf(markerName, sizeof(markerName), "MarkerConsole%d", i);
    m_console[i] = m_shape->GetMarkerData(markerName);
  }

  if (!s_dishShape)
    s_dishShape = g_app->m_resource->GetShapeStatic("controltowerdish.shp");
}

void ControlTower::Initialise(Building* _template)
{
  Building::Initialise(_template);

  m_controlBuildingId = static_cast<ControlTower*>(_template)->m_controlBuildingId;
}

bool ControlTower::Advance()
{
  //
  // Calculate our Dish matrix (once only, during first Advance)
  // Also look to see if our building is already captured

  if (m_dishMatrix == Matrix34())
  {
    Building* targetBuilding = g_app->m_location->GetBuilding(m_controlBuildingId);
    if (targetBuilding)
    {
      Matrix34 mat(m_front, g_upVector, m_pos);
      LegacyVector3 dishPos = m_shape->GetMarkerWorldMatrix(m_dishPos, mat).pos;
      LegacyVector3 dishFront = (dishPos - targetBuilding->m_centerPos).Normalise();
      LegacyVector3 dishRight = dishFront ^ g_upVector;
      LegacyVector3 dishUp = dishRight ^ dishFront;
      m_dishMatrix = Matrix34(dishFront, dishUp, dishPos);

      if (targetBuilding->m_id.GetTeamId() == 255)
        m_ownership = 0.0f;
      else
        m_ownership = 100.0f;
    }
  }

  //
  // If we are connected to a TrunkPort, somebody else may have opened that port from another map.
  // Every once in a while, check and turn on if it has happened

  m_checkTargetTimer -= SERVER_ADVANCE_PERIOD;

  if (m_checkTargetTimer <= 0.0f)
  {
    Building* building = g_app->m_location->GetBuilding(m_controlBuildingId);
    if (building && building->m_type == TypeTrunkPort && m_id.GetTeamId() != 2)
    {
      auto tp = static_cast<TrunkPort*>(building);
      if (tp->m_openTimer > 0.0f)
      {
        m_id.SetTeamId(2);
        m_ownership = 100.0f;
      }
    }
    m_checkTargetTimer = 2.0f;
  }

  //
  // Spawn particles if we are being reprogrammed

  for (int i = 0; i < 3; ++i)
  {
    if (m_beingReprogrammed[i])
    {
      Matrix34 rootMat(m_front, g_upVector, m_pos);
      Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_console[i], rootMat);
      LegacyVector3 particleVel = worldMat.pos - m_pos;
      particleVel += LegacyVector3(sfrand() * 10.0f, sfrand() * 5.0f, sfrand() * 10.0f);
      g_simEventQueue.Push(SimEvent::MakeParticle(worldMat.pos, particleVel, SimParticle::TypeBlueSpark));
    }
  }

  return Building::Advance();
}

int ControlTower::GetAvailablePosition(LegacyVector3& _pos, LegacyVector3& _front)
{
  for (int i = 0; i < 3; ++i)
  {
    if (!m_beingReprogrammed[i])
    {
      Matrix34 rootMat(m_front, g_upVector, m_pos);
      Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_reprogrammer[i], rootMat);

      _pos = worldMat.pos;
      _front = worldMat.f;

      return i;
    }
  }

  return -1;
}

void ControlTower::GetConsolePosition(int _position, LegacyVector3& _pos)
{
  DEBUG_ASSERT(_position >= 0 && _position < 3);

  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_console[_position], rootMat);

  _pos = worldMat.pos;
}

void ControlTower::BeginReprogram(int _position)
{
  DEBUG_ASSERT(!m_beingReprogrammed[_position]);
  m_beingReprogrammed[_position] = true;
}

bool ControlTower::Reprogram(int _teamId)
{
  if (_teamId != m_id.GetTeamId())
  {
    // Removing someone elses control
    m_ownership -= 0.1f;
    if (m_ownership <= 0.0f)
    {
      m_id.SetTeamId(_teamId);
      Building* targetBuilding = g_app->m_location->GetBuilding(m_controlBuildingId);
      if (targetBuilding && targetBuilding->m_id.GetTeamId() != m_id.GetTeamId())
        targetBuilding->SetTeamId(m_id.GetTeamId());
    }
  }
  else
  {
    // Increasing our own control
    if (m_ownership < 100.0f)
    {
      m_ownership += 0.1f;
      if (m_ownership > 100.0f)
        m_ownership = 100.0f;

      Building* targetBuilding = g_app->m_location->GetBuilding(m_controlBuildingId);
      if (targetBuilding)
      {
        targetBuilding->Reprogram(m_ownership);

        if (m_ownership == 100.0f)
        {
          g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "ReprogramComplete"));
          //g_app->m_sepulveda
          targetBuilding->ReprogramComplete();
          SetTeamId(_teamId);
          g_app->m_globalWorld->m_research->GiveResearchPoints(GLOBALRESEARCH_POINTS_CONTROLTOWER);

          GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
          if (gb)
          {
            gb->m_online = true;
            g_app->m_globalWorld->EvaluateEvents();
          }
          return true;
        }
      }
    }
    else
      return true;
  }

  return false;
}

void ControlTower::EndReprogram(int _position)
{
  DEBUG_ASSERT(m_beingReprogrammed[_position]);
  m_beingReprogrammed[_position] = false;
}

bool ControlTower::IsInView()
{
  if (Building::IsInView())
    return true;

  //
  // Check against the tall thin control line to heaven

  LegacyVector3 towerPos = m_pos;
  towerPos.y = g_app->m_camera->GetPos().y;
  return g_app->m_camera->PosInViewFrustum(towerPos);
}

void ControlTower::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  char* word = _in->GetNextToken();
  m_controlBuildingId = atoi(word);
}

void ControlTower::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%6d", m_controlBuildingId);
}

int ControlTower::GetBuildingLink() { return m_controlBuildingId; }

void ControlTower::SetBuildingLink(int _buildingId) { m_controlBuildingId = _buildingId; }
