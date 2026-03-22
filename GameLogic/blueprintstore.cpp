#include "pch.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "text_stream_readers.h"
#include "file_writer.h"
#include "language_table.h"
#include "blueprintstore.h"
#include "GameAppSim.h"
#include "location.h"
#include "camera.h"
#include "main.h"
#include "team.h"
#include "global_world.h"

BlueprintBuilding::BlueprintBuilding()
  : Building(),
    m_buildingLink(-1),
    m_infected(0.0f),
    m_segment(0),
    m_marker(nullptr) { m_vel.Zero(); }

void BlueprintBuilding::Initialise(Building* _template)
{
  Building::Initialise(_template);

  m_marker = m_shape->GetMarkerData("MarkerBlueprint");
  DEBUG_ASSERT(m_marker);

  auto blueprintBuilding = static_cast<BlueprintBuilding*>(_template);

  m_buildingLink = blueprintBuilding->m_buildingLink;

  if (m_id.GetTeamId() == 1)
    m_infected = 100.0f;
  else
    m_infected = 0.0f;
}

bool BlueprintBuilding::Advance()
{
  auto blueprintBuilding = static_cast<BlueprintBuilding*>(g_app->m_location->GetBuilding(m_buildingLink));
  if (blueprintBuilding)
  {
    if (m_infected > 80.0f)
      blueprintBuilding->SendBlueprint(m_segment, true);
    if (m_infected < 20.0f)
      blueprintBuilding->SendBlueprint(m_segment, false);
  }

  return Building::Advance();
}

Matrix34 BlueprintBuilding::GetMarker(float _predictionTime)
{
  LegacyVector3 pos = m_pos + m_vel * _predictionTime;
  Matrix34 mat(m_front, g_upVector, pos);

  if (m_marker)
  {
    Matrix34 markerMat = m_shape->GetMarkerWorldMatrix(m_marker, mat);
    return markerMat;
  }
  return mat;
}

bool BlueprintBuilding::IsInView()
{
  Building* link = g_app->m_location->GetBuilding(m_buildingLink);

  if (link)
  {
    LegacyVector3 midPoint = (link->m_centerPos + m_centerPos) / 2.0f;
    float radius = (link->m_centerPos - m_centerPos).Mag();
    radius += m_radius;
    return (g_app->m_camera->SphereInViewFrustum(midPoint, radius));
  }
  return Building::IsInView();
}

void BlueprintBuilding::SendBlueprint(int _segment, bool _infected)
{
  m_segment = _segment;

  if (_infected)
    m_infected += SERVER_ADVANCE_PERIOD * 10.0f;
  else
    m_infected -= SERVER_ADVANCE_PERIOD * 10.0f;

  m_infected = max(m_infected, 0.0f);
  m_infected = min(m_infected, 100.0f);
}

void BlueprintBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_buildingLink = atoi(_in->GetNextToken());
}

void BlueprintBuilding::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_buildingLink);
}

int BlueprintBuilding::GetBuildingLink() { return m_buildingLink; }

void BlueprintBuilding::SetBuildingLink(int _buildingId) { m_buildingLink = _buildingId; }

// ============================================================================

BlueprintStore::BlueprintStore()
  : BlueprintBuilding()
{
  m_type = TypeBlueprintStore;

  SetShape(g_app->m_resource->GetShapeStatic("blueprintstore.shp"));
}

const char* BlueprintStore::GetObjectiveCounter()
{
  static char result[256];

  float totalInfection = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
    totalInfection += m_segments[i];

  snprintf(result, sizeof(result), "%s %d%%", LANGUAGEPHRASE("objective_totalinfection"),
           static_cast<int>(100.0f * totalInfection / float(BLUEPRINTSTORE_NUMSEGMENTS * 100.0f)));

  return result;
}

void BlueprintStore::Initialise(Building* _template)
{
  BlueprintBuilding::Initialise(_template);

  if (m_id.GetTeamId() == 1)
  {
    for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
      m_segments[i] = 100.0f;
  }
  else
  {
    for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
      m_segments[i] = 0.0f;
  }
}

void BlueprintStore::SendBlueprint(int _segment, bool _infected)
{
  float oldValue = m_segments[_segment];

  if (_infected)
    oldValue += SERVER_ADVANCE_PERIOD * 1.0f;
  else
    oldValue -= SERVER_ADVANCE_PERIOD * 1.0f;

  oldValue = max(oldValue, 0.0f);
  oldValue = min(oldValue, 100.0f);

  m_segments[_segment] = oldValue;
}

bool BlueprintStore::Advance()
{
  int fullyInfected = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (m_segments[i] == 100.0f)
      fullyInfected++;
  }
  int clean = BLUEPRINTSTORE_NUMSEGMENTS - fullyInfected;

  //
  // Spread our existing infection

  if (clean > 0)
  {
    float infectionChange = (fullyInfected * 0.9f) / static_cast<float>(clean);

    for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
    {
      if (m_segments[i] < 100.0f)
      {
        float oldValue = m_segments[i];
        oldValue += SERVER_ADVANCE_PERIOD * infectionChange;
        oldValue = max(oldValue, 0.0f);
        oldValue = min(oldValue, 100.0f);
        m_segments[i] = oldValue;
      }
    }
  }

  //
  // Are we clean?

  bool totallyClean = true;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (m_segments[i] > 0.0f)
    {
      totallyClean = false;
      break;
    }
  }

  if (totallyClean)
  {
    GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
    if (gb)
      gb->m_online = true;
  }

  return BlueprintBuilding::Advance();
}

int BlueprintStore::GetNumInfected()
{
  int result = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (!(m_segments[i] < 100.0f))
      ++result;
  }
  return result;
}

int BlueprintStore::GetNumClean()
{
  int result = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (!(m_segments[i] > 0.0f))
      ++result;
  }
  return result;
}

void BlueprintStore::GetDisplay(LegacyVector3& _pos, LegacyVector3& _right, LegacyVector3& _up, float& _size)
{
  _size = 50.0f;

  LegacyVector3 front = m_front;
  front.RotateAroundY(sinf(g_gameTime) * 0.3f);

  LegacyVector3 up = g_upVector;
  up.RotateAround(m_front * cosf(g_gameTime) * 0.1f);

  _pos = m_pos + up * 50.0f - front * _size;
  _right = front;
  _up = up;
}

// ============================================================================

BlueprintConsole::BlueprintConsole()
  : BlueprintBuilding()
{
  m_type = TypeBlueprintConsole;

  SetShape(g_app->m_resource->GetShapeStatic("blueprintconsole.shp"));
}

void BlueprintConsole::Initialise(Building* _template)
{
  BlueprintBuilding::Initialise(_template);

  auto console = static_cast<BlueprintConsole*>(_template);
  m_segment = console->m_segment;
}

void BlueprintConsole::RecalculateOwnership()
{
  int teamCount[NUM_TEAMS];
  memset(teamCount, 0, NUM_TEAMS * sizeof(int));

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    WorldObjectId id = GetPortOccupant(i);
    if (id.IsValid())
      teamCount[id.GetTeamId()]++;
  }

  int winningTeam = -1;
  for (int i = 0; i < NUM_TEAMS; ++i)
  {
    if (teamCount[i] > 2 && winningTeam == -1)
      winningTeam = i;
    else if (winningTeam != -1 && teamCount[i] > 2 && teamCount[i] > teamCount[winningTeam])
      winningTeam = i;
  }

  if (winningTeam == -1)
    SetTeamId(255);
  else
    SetTeamId(winningTeam);
}

void BlueprintConsole::Read(TextReader* _in, bool _dynamic)
{
  BlueprintBuilding::Read(_in, _dynamic);

  m_segment = atoi(_in->GetNextToken());
}

void BlueprintConsole::Write(FileWriter* _out)
{
  BlueprintBuilding::Write(_out);

  _out->printf("%-8d", m_segment);
}

bool BlueprintConsole::Advance()
{
  RecalculateOwnership();

  bool infected = (m_id.GetTeamId() == 1);
  bool clean = (m_id.GetTeamId() == 0);

  if (infected)
    SendBlueprint(m_segment, true);
  if (clean)
    SendBlueprint(m_segment, false);

  return BlueprintBuilding::Advance();
}

// ============================================================================

BlueprintRelay::BlueprintRelay()
  : BlueprintBuilding(),
    m_altitude(400.0f)
{
  m_type = TypeBlueprintRelay;

  SetShape(g_app->m_resource->GetShapeStatic("blueprintrelay.shp"));
}

void BlueprintRelay::Initialise(Building* _template)
{
  BlueprintBuilding::Initialise(_template);

  auto blueprintRelay = static_cast<BlueprintRelay*>(_template);
  m_altitude = blueprintRelay->m_altitude;

  m_pos.y = m_altitude;
  Matrix34 mat(m_front, m_up, m_pos);
  m_centerPos = m_shape->CalculateCenter(mat);
}

void BlueprintRelay::SetDetail(int _detail)
{
  m_pos.y = m_altitude;

  Matrix34 mat(m_front, m_up, m_pos);
  m_centerPos = m_shape->CalculateCenter(mat);
  m_radius = m_shape->CalculateRadius(mat, m_centerPos);
}

bool BlueprintRelay::Advance()
{
  float ourTime = g_gameTime + m_id.GetUniqueId() + m_id.GetIndex();

  LegacyVector3 oldPos = m_pos;

  m_pos.x += sinf(ourTime / 1.5f) * 1.0f;
  m_pos.y += sinf(ourTime / 1.3f) * 1.0f;
  m_pos.z += cosf(ourTime / 1.7f) * 1.0f;

  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

  m_centerPos = m_pos;

  return BlueprintBuilding::Advance();
}

void BlueprintRelay::Read(TextReader* _in, bool _dynamic)
{
  BlueprintBuilding::Read(_in, _dynamic);

  m_altitude = atof(_in->GetNextToken());
}

void BlueprintRelay::Write(FileWriter* _out)
{
  BlueprintBuilding::Write(_out);

  _out->printf("%-2.2f", m_altitude);
}
