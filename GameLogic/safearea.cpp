#include "pch.h"
#include "file_writer.h"
#include "text_stream_readers.h"
#include "hi_res_time.h"
#include "language_table.h"
#include "safearea.h"
#include "GameContext.h"
#include "location.h"
#include "team.h"
#include "entity_grid.h"
#include "global_world.h"

SafeArea::SafeArea()
  : Building(),
    m_size(50.0f),
    m_entitiesRequired(0),
    m_entityTypeRequired(Entity::TypeDarwinian),
    m_recountTimer(0.0f),
    m_entitiesCounted(0) { m_type = TypeSafeArea; }

void SafeArea::Initialise(Building* _template)
{
  Building::Initialise(_template);

  m_size = static_cast<SafeArea*>(_template)->m_size;
  m_entitiesRequired = static_cast<SafeArea*>(_template)->m_entitiesRequired;
  m_entityTypeRequired = static_cast<SafeArea*>(_template)->m_entityTypeRequired;

  m_radius = m_size;
  m_centerPos = m_pos + LegacyVector3(0, m_radius / 2, 0);
}

bool SafeArea::Advance()
{
  //
  // Is the world awake yet ?

  if (!g_context->m_location)
    return false;
  if (!g_context->m_location->m_teams)
    return false;
  if (g_context->m_location->m_teams[m_id.GetTeamId()].m_teamType == Team::TeamTypeUnused)
    return false;

  if (GetHighResTime() > m_recountTimer)
  {
    int numFound;
    WorldObjectId* ids = g_context->m_location->m_entityGrid->GetFriends(m_pos.x, m_pos.z, m_size, &numFound, m_id.GetTeamId());
    m_entitiesCounted = 0;

    for (int i = 0; i < numFound; ++i)
    {
      WorldObjectId id = ids[i];
      Entity* entity = g_context->m_location->GetEntity(id);
      if (entity && entity->m_type == m_entityTypeRequired && !entity->m_dead)
        ++m_entitiesCounted;
    }

    m_recountTimer = GetHighResTime() + 2.0f;

    if ((m_id.GetTeamId() == 1 && m_entitiesCounted <= m_entitiesRequired) || (m_id.GetTeamId() != 1 && m_entitiesCounted >=
      m_entitiesRequired))
    {
      GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
      if (gb && !gb->m_online)
      {
        gb->m_online = true;
        g_context->m_globalWorld->EvaluateEvents();
      }
    }
  }

  return false;
}

bool SafeArea::DoesSphereHit([[maybe_unused]] const LegacyVector3& _pos, [[maybe_unused]] float _radius) { return false; }

bool SafeArea::DoesShapeHit([[maybe_unused]] ShapeStatic* _shape, [[maybe_unused]] Matrix34 _transform) { return false; }

bool SafeArea::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                          LegacyVector3* _norm)
{
  if (g_context->m_editing)
    return Building::DoesRayHit(_rayStart, _rayDir, _rayLen, _pos, _norm);
  return false;
}

const char* SafeArea::GetObjectiveCounter()
{
  static std::string result;
  result = std::format("{} : {}", LANGUAGEPHRASE("objective_currentcount"), m_entitiesCounted);
  return result.c_str();
}

void SafeArea::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_size = atof(_in->GetNextToken());
  m_entitiesRequired = atoi(_in->GetNextToken());
  m_entityTypeRequired = atoi(_in->GetNextToken());
}

void SafeArea::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-6.2f", m_size);
  _out->printf(" %d", m_entitiesRequired);
  _out->printf(" %d", m_entityTypeRequired);
}
