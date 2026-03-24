#include "pch.h"
#include "spawnpoint.h"
#include "GameContext.h"
#include "ShapeStatic.h"
#include "camera.h"
#include "entity_grid.h"
#include "file_writer.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "language_table.h"
#include "location.h"
#include "math_utils.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "team.h"
#include "text_stream_readers.h"

SpawnBuilding::SpawnBuilding()
  : Building(),
    m_spiritLink(nullptr),
    m_visibilityRadius(0.0f) {}

SpawnBuilding::~SpawnBuilding()
{
  m_links.EmptyAndDelete();
  //SAFE_DELETE(m_spiritLink); probably not necessary
}

void SpawnBuilding::Initialise(Building* _template)
{
  Building::Initialise(_template);

  auto spawn = static_cast<SpawnBuilding*>(_template);
  for (int i = 0; i < spawn->m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = spawn->m_links[i];
    SetBuildingLink(link->m_targetBuildingId);
  }
}

bool SpawnBuilding::IsInView()
{
  if (m_visibilityRadius == 0.0f || g_context->m_editing)
  {
    if (m_links.Size() == 0)
    {
      m_visibilityRadius = m_radius;
      m_visibilityMidpoint = m_centerPos;
      return Building::IsInView();
    }
    m_visibilityMidpoint = m_centerPos;
    int numLinks = 1;
    m_visibilityRadius = 0.0f;

    // Find mid point

    for (int i = 0; i < m_links.Size(); ++i)
    {
      SpawnBuildingLink* link = m_links[i];
      auto building = static_cast<SpawnBuilding*>(g_context->m_location->GetBuilding(link->m_targetBuildingId));
      if (building)
      {
        m_visibilityMidpoint += building->m_centerPos;
        ++numLinks;
      }
    }

    m_visibilityMidpoint /= static_cast<float>(numLinks);

    // Find radius

    for (int i = 0; i < m_links.Size(); ++i)
    {
      SpawnBuildingLink* link = m_links[i];
      auto building = static_cast<SpawnBuilding*>(g_context->m_location->GetBuilding(link->m_targetBuildingId));
      if (building)
      {
        float distance = (building->m_centerPos - m_visibilityMidpoint).Mag();
        distance += building->m_radius / 2.0f;
        m_visibilityRadius = std::max(m_visibilityRadius, distance);
      }
    }
  }

  // Determine visibility

  return g_context->m_camera->SphereInViewFrustum(m_visibilityMidpoint, m_visibilityRadius);
}

void SpawnBuilding::SetBuildingLink(int _buildingId)
{
  auto link = new SpawnBuildingLink();
  link->m_targetBuildingId = _buildingId;
  m_links.PutData(link);
}

void SpawnBuilding::ClearLinks() { m_links.EmptyAndDelete(); }

LList<int>* SpawnBuilding::ExploreLinks()
{
  auto result = new LList<int>();

  if (m_type == TypeSpawnPoint)
    result->PutData(m_id.GetUniqueId());

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    link->m_targets.Empty();

    auto target = static_cast<SpawnBuilding*>(g_context->m_location->GetBuilding(link->m_targetBuildingId));
    if (target)
    {
      LList<int>* availableLinks = target->ExploreLinks();
      for (int j = 0; j < availableLinks->Size(); ++j)
      {
        int thisLink = availableLinks->GetData(j);
        result->PutData(thisLink);
        link->m_targets.PutData(thisLink);
      }
      delete availableLinks;
    }
  }

  return result;
}

LegacyVector3 SpawnBuilding::GetSpiritLink() const
{
  if (!m_spiritLink)
  {
    m_spiritLink = m_shape->GetMarkerData("MarkerSpiritLink");
    DEBUG_ASSERT(m_spiritLink);
  }

  Matrix34 mat(m_front, g_upVector, m_pos);

  return m_shape->GetMarkerWorldMatrix(m_spiritLink, mat).pos;
}

void SpawnBuilding::TriggerSpirit(SpawnBuildingSpirit* _spirit)
{
  int numAdds = 0;

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    for (int j = 0; j < link->m_targets.Size(); ++j)
    {
      int thisLink = link->m_targets[j];
      if (link->m_targets[j] == _spirit->m_targetBuildingId)
      {
        link->m_spirits.PutData(_spirit);
        _spirit->m_currentProgress = 0.0f;
        ++numAdds;
      }
    }
  }

  if (numAdds == 0)
  {
    // We have nowhere else to go
    // Or maybe we have arrived
    delete _spirit;
  }
}

bool SpawnBuilding::Advance()
{
  //
  // Advance all spirits in transit

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    for (int j = 0; j < link->m_spirits.Size(); ++j)
    {
      SpawnBuildingSpirit* spirit = link->m_spirits[j];
      spirit->m_currentProgress += SERVER_ADVANCE_PERIOD;
      if (spirit->m_currentProgress >= 1.0f)
      {
        link->m_spirits.RemoveData(j);
        --j;
        auto building = static_cast<SpawnBuilding*>(g_context->m_location->GetBuilding(link->m_targetBuildingId));
        if (building)
          building->TriggerSpirit(spirit);
      }
    }
  }

  return Building::Advance();
}

void SpawnBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  while (_in->TokenAvailable())
  {
    int nextLink = atoi(_in->GetNextToken());
    SetBuildingLink(nextLink);
  }
}

void SpawnBuilding::Write(FileWriter* _out)
{
  Building::Write(_out);

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    _out->printf("%-6d", link->m_targetBuildingId);
  }
}

// ============================================================================

SpawnLink::SpawnLink()
  : SpawnBuilding()
{
  m_type = TypeSpawnLink;

  SetShape(Resource::GetShapeStatic("spawnlink.shp"));
}

// ============================================================================

int MasterSpawnPoint::s_masterSpawnPointId = -1;

MasterSpawnPoint::MasterSpawnPoint()
  : SpawnBuilding(),
    m_exploreLinks(false)
{
  m_type = TypeSpawnPointMaster;

  SetShape(Resource::GetShapeStatic("masterspawnpoint.shp"));
}

MasterSpawnPoint* MasterSpawnPoint::GetMasterSpawnPoint()
{
  Building* building = g_context->m_location->GetBuilding(s_masterSpawnPointId);
  if (building && building->m_type == TypeSpawnPointMaster)
    return static_cast<MasterSpawnPoint*>(building);

  return nullptr;
}

void MasterSpawnPoint::RequestSpirit(int _targetBuildingId)
{
  auto spirit = new SpawnBuildingSpirit();
  spirit->m_targetBuildingId = _targetBuildingId;
  TriggerSpirit(spirit);
}

bool MasterSpawnPoint::Advance()
{
  s_masterSpawnPointId = m_id.GetUniqueId();

  //
  // Explore our links first time through

  if (!m_exploreLinks)
  {
    m_exploreLinks = true;
    float startTime = GetHighResTime();
    ExploreLinks();
    float timeTaken = GetHighResTime() - startTime;
    DebugTrace("Time to Explore all Spawn Point links : {} ms\n", static_cast<int>(timeTaken * 1000.0f));
  }

  //
  // Accelerate spirits to heaven

  if (m_isGlobal)
  {
    for (int i = 0; i < g_context->m_location->m_spirits.Size(); ++i)
    {
      if (g_context->m_location->m_spirits.ValidIndex(i))
      {
        Spirit* s = g_context->m_location->m_spirits.GetPointer(i);
        if (s->m_state == Spirit::StateBirth || s->m_state == Spirit::StateFloating)
          s->SkipStage();
      }
    }
  }

  //
  // Have the red guys been wiped out?

  if (g_context->m_location->m_teams[1].m_teamType != Team::TeamTypeUnused)
  {
    int numRed = g_context->m_location->m_teams[1].m_others.NumUsed();
    if (numRed < 10)
    {
      GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
      if (gb)
        gb->m_online = true;
    }
  }

  return SpawnBuilding::Advance();
}

const char* MasterSpawnPoint::GetObjectiveCounter()
{
  static char result[256];

  if (g_context->m_location->m_teams[1].m_teamType != Team::TeamTypeUnused)
  {
    int numRed = g_context->m_location->m_teams[1].m_others.NumUsed();
    snprintf(result, sizeof(result), "%s : %d", LANGUAGEPHRASE("objective_redpopulation"), numRed);
  }
  else
    snprintf(result, sizeof(result), "%s", LANGUAGEPHRASE("objective_redpopulation"));

  return result;
}

// ============================================================================

SpawnPoint::SpawnPoint()
  : SpawnBuilding(),
    m_evaluateTimer(0.0f),
    m_spawnTimer(0.0f),
    m_populationLock(-1),
    m_numFriendsNearby(0)
{
  m_type = TypeSpawnPoint;

  SetShape(Resource::GetShapeStatic("spawnpoint.shp"));
  m_doorMarker = m_shape->GetMarkerData("MarkerDoor");

  m_evaluateTimer = syncfrand(2.0f);
  m_spawnTimer = 2.0f + syncfrand(2.0f);
}

bool SpawnPoint::PopulationLocked()
{
  //
  // If we haven't yet looked up a nearby Pop lock,
  // do so now

  if (m_populationLock == -1)
  {
    m_populationLock = -2;

    for (int i = 0; i < g_context->m_location->m_buildings.Size(); ++i)
    {
      if (g_context->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_context->m_location->m_buildings[i];
        if (building && building->m_type == TypeSpawnPopulationLock)
        {
          auto lock = static_cast<SpawnPopulationLock*>(building);
          float distance = (building->m_pos - m_pos).Mag();
          if (distance < lock->m_searchRadius)
          {
            m_populationLock = lock->m_id.GetUniqueId();
            break;
          }
        }
      }
    }
  }

  //
  // If we are inside a Population Lock, query it now

  if (m_populationLock > 0)
  {
    auto lock = static_cast<SpawnPopulationLock*>(g_context->m_location->GetBuilding(m_populationLock));
    if (lock && m_id.GetTeamId() != 255 && lock->m_teamCount[m_id.GetTeamId()] >= lock->m_maxPopulation)
      return true;
  }

  return false;
}

void SpawnPoint::RecalculateOwnership()
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

void SpawnPoint::TriggerSpirit(SpawnBuildingSpirit* _spirit)
{
  if (m_id.GetTeamId() == 0)
    int b = 10;

  if (m_id.GetUniqueId() == _spirit->m_targetBuildingId)
  {
    // This spirit has arrived at its destination
    if (m_id.GetTeamId() != 255)
    {
      Matrix34 mat(m_front, m_up, m_pos);
      Matrix34 doorMat = m_shape->GetMarkerWorldMatrix(m_doorMarker, mat);
      g_context->m_location->SpawnEntities(doorMat.pos, m_id.GetTeamId(), -1, Entity::TypeDarwinian, 1, g_zeroVector, 0.0f);
    }

    delete _spirit;
  }
  else
    SpawnBuilding::TriggerSpirit(_spirit);
}

bool SpawnPoint::PerformDepthSort(LegacyVector3& _centerPos)
{
  _centerPos = m_centerPos;
  return true;
}

bool SpawnPoint::Advance()
{
  //
  // Re-evalulate our situation

  m_evaluateTimer -= SERVER_ADVANCE_PERIOD;
  if (m_evaluateTimer <= 0.0f)
  {
    START_PROFILE(g_context->m_profiler, "Evaluate");

    RecalculateOwnership();
    m_evaluateTimer = 2.0f;

    END_PROFILE(g_context->m_profiler, "Evaluate");
  }

  //
  // Time to request more spirits for our Darwinians?

  START_PROFILE(g_context->m_profiler, "SpawnDarwinians");
  if (m_id.GetTeamId() != 255 && !PopulationLocked())
  {
    m_spawnTimer -= SERVER_ADVANCE_PERIOD;
    if (m_spawnTimer <= 0.0f)
    {
      MasterSpawnPoint* masterSpawn = MasterSpawnPoint::GetMasterSpawnPoint();
      if (masterSpawn)
        masterSpawn->RequestSpirit(m_id.GetUniqueId());

      float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

      m_spawnTimer = 2.0f + 5.0f * (1.0f - fractionOccupied);

      // Reds spawn a bit quicker
      if (m_id.GetTeamId() == 1)
        m_spawnTimer -= 1.0 * (g_context->m_difficultyLevel / 10.0);
      else
        m_spawnTimer -= 0.5 * (g_context->m_difficultyLevel / 10.0);
    }
  }
  END_PROFILE(g_context->m_profiler, "SpawnDarwinians");

  return SpawnBuilding::Advance();
}

// ============================================================================

float SpawnPopulationLock::s_overpopulationTimer = 0.0f;
int SpawnPopulationLock::s_overpopulation = 0;

SpawnPopulationLock::SpawnPopulationLock()
  : Building(),
    m_searchRadius(500.0f),
    m_maxPopulation(100),
    m_originalMaxPopulation(100),
    m_recountTeamId(-1)
{
  m_type = TypeSpawnPopulationLock;

  memset(m_teamCount, 0, NUM_TEAMS * sizeof(int));

  m_recountTimer = syncfrand(10.0f);
  m_recountTeamId = 10;
}

void SpawnPopulationLock::Initialise(Building* _template)
{
  Building::Initialise(_template);

  auto lock = static_cast<SpawnPopulationLock*>(_template);
  m_searchRadius = lock->m_searchRadius;
  m_maxPopulation = lock->m_maxPopulation;

  m_originalMaxPopulation = m_maxPopulation;
}

bool SpawnPopulationLock::Advance()
{
  //
  // Every once in a while, have a look at the global over-population rate
  // And reduce all local maxPopulations accordingly
  // This only really applies to Green darwinians, as the player can arrange
  // it so an unlimited army gathers on a single island

  if (GetHighResTime() > s_overpopulationTimer)
  {
    s_overpopulationTimer = GetHighResTime() + 1.0f;

    int totalOverpopulation = 0;

    for (int i = 0; i < g_context->m_location->m_buildings.Size(); ++i)
    {
      if (g_context->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_context->m_location->m_buildings[i];
        if (building && building->m_type == TypeSpawnPopulationLock)
        {
          auto lock = static_cast<SpawnPopulationLock*>(building);
          if (lock->m_teamCount[0] > lock->m_originalMaxPopulation)
            totalOverpopulation += (lock->m_teamCount[0] - lock->m_originalMaxPopulation);
        }
      }
    }

    s_overpopulation = totalOverpopulation / 2;
  }

  m_maxPopulation = m_originalMaxPopulation - s_overpopulation;
  m_maxPopulation = std::max(m_maxPopulation, 0);

  //
  // Recount the number of entities nearby

  m_recountTimer -= SERVER_ADVANCE_PERIOD;
  if (m_recountTimer <= 0.0f)
  {
    m_recountTimer = 10.0f;
    m_recountTeamId = 10;
  }

  int potentialTeamId = static_cast<int>(m_recountTimer);
  if (potentialTeamId < m_recountTeamId && potentialTeamId < NUM_TEAMS)
  {
    m_recountTeamId = potentialTeamId;
    m_teamCount[potentialTeamId] = g_context->m_location->m_entityGrid->GetNumFriends(m_pos.x, m_pos.z, m_searchRadius, m_recountTeamId);
  }

  return Building::Advance();
}

void SpawnPopulationLock::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_searchRadius = atof(_in->GetNextToken());
  m_maxPopulation = atoi(_in->GetNextToken());
}

void SpawnPopulationLock::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8.2f %-6d", m_searchRadius, m_maxPopulation);
}

bool SpawnPopulationLock::DoesSphereHit(const LegacyVector3& _pos, float _radius) { return false; }

bool SpawnPopulationLock::DoesShapeHit(ShapeStatic* _shape, Matrix34 _transform) { return false; }
