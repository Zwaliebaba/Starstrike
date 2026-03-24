#include "pch.h"
#include "factory.h"
#include "GameContext.h"
#include "file_writer.h"
#include "globals.h"
#include "location.h"
#include "math_utils.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "team.h"
#include "text_stream_readers.h"
#include "unit.h"

Factory::Factory()
  : Building(),
    m_troopType(Entity::TypeInvalid),
    m_initialCapacity(10),
    m_unitId(-1),
    m_numToCreate(-1),
    m_numCreated(-1),
    m_timeToCreate(0.0f),
    m_timeSoFar(0.0f),
    m_state(StateUnused)
{
  m_type = TypeFactory;
  SetShape(Resource::GetShapeStatic("factory.shp"));
}

void Factory::Initialise(Building* _template)
{
  DEBUG_ASSERT(_template->m_type == Building::TypeFactory);
  auto factory = static_cast<Factory*>(_template);
  m_initialCapacity = factory->m_initialCapacity;

  Building::Initialise(_template);

  ShapeMarkerData* markerSpiritStore = m_shape->GetMarkerData("MarkerSpiritStore");
  Matrix34 rootTransform(m_front, g_upVector, m_pos);
  const Matrix34& storeMat = m_shape->GetMarkerWorldMatrix(markerSpiritStore, rootTransform);

  LegacyVector3 spiritStorePos = storeMat.pos + LegacyVector3(0, 11.0f, 0);
  m_spiritStore.Initialise(m_initialCapacity, 200, spiritStorePos, 5.0f, 10.0f, 5.0f);
}

void Factory::RequestUnit(unsigned char _troopType, int _numToCreate)
{
  m_troopType = _troopType;

  for (int i = 0; i < Entity::NumStats; ++i)
    m_stats[i] = EntityBlueprint::GetStat(m_troopType, i);

  m_numToCreate = _numToCreate;
  m_numCreated = 0;
  m_timeToCreate = m_numToCreate * 0.1f;
  m_timeSoFar = 0.0f;

  if (_troopType < Entity::TypeEngineer || _troopType == Entity::TypeInsertionSquadie)
  {
    Team* team = &g_context->m_location->m_teams[m_id.GetTeamId()];
    Unit* unit = team->NewUnit(_troopType, _numToCreate, &m_unitId, m_pos);
    unit->SetWayPoint(m_pos + m_front * 30.0f);
  }
  else
    m_unitId = -1;

  m_state = StateCreating;
}

bool Factory::Advance()
{
  switch (m_state)
  {
  case StateUnused:
    AdvanceStateUnused();
    break;
  case StateCreating:
    AdvanceStateCreating();
    break;
  case StateRecharging:
    AdvanceStateRecharging();
    break;
  }

  m_spiritStore.Advance();

  return Building::Advance();
}

void Factory::AdvanceStateUnused() {}

void Factory::AdvanceStateCreating()
{
  float fractionComplete = m_timeSoFar / m_timeToCreate;
  if (fractionComplete > 1.0f)
    fractionComplete = 1.0f;

  int numThatShouldBeCreated = static_cast<int>(m_numToCreate * fractionComplete);
  int numToCreate = (numThatShouldBeCreated - m_numCreated);
  int numActuallyCreated = 0;

  for (int i = 0; i < numToCreate; ++i)
  {
    if (m_spiritStore.NumSpirits() > 0)
    {
      LegacyVector3 pos(m_pos + LegacyVector3(syncsfrand(5.0f), 20.0f + syncsfrand(5.0f), 20.0f));
      LegacyVector3 vel(syncsfrand(1.0f), syncsfrand(1.0f), 5.0f + syncsfrand(1.0f));

      m_spiritStore.RemoveSpirits(1);
      g_context->m_location->SpawnEntities(pos, m_id.GetTeamId(), m_unitId, m_troopType, 1, vel, 0.0f);

      ++numActuallyCreated;
    }
  }

  if (numActuallyCreated > 0 || m_spiritStore.NumSpirits() > 0)
  {
    m_numCreated += numActuallyCreated;

    m_timeSoFar += SERVER_ADVANCE_PERIOD;

    if (fractionComplete >= 1.0f)
    {
      m_timeSoFar = 0.0f;
      m_timeToCreate = 3.0f;
      m_state = StateRecharging;
    }
  }
}

void Factory::AdvanceStateRecharging()
{
  m_timeSoFar += SERVER_ADVANCE_PERIOD;

  if (m_timeSoFar >= m_timeToCreate)
    m_state = StateUnused;
}

void Factory::SetTeamId(int _teamId)
{
  Building::SetTeamId(_teamId);

  m_state = StateUnused;
}

void Factory::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  char* word = _in->GetNextToken();
  m_initialCapacity = atoi(word);
}

void Factory::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_initialCapacity);
}
