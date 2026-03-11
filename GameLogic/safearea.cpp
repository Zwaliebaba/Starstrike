#include "pch.h"
#include "file_writer.h"
#include "text_stream_readers.h"
#include "hi_res_time.h"
#include "language_table.h"
#include "safearea.h"
#include "GameApp.h"
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
  m_centrePos = m_pos + LegacyVector3(0, m_radius / 2, 0);
}

bool SafeArea::Advance()
{
  //
  // Is the world awake yet ?

  if (!g_app->m_location)
    return false;
  if (!g_app->m_location->m_teams)
    return false;
  if (g_app->m_location->m_teams[m_id.GetTeamId()].m_teamType == Team::TeamTypeUnused)
    return false;

  if (GetHighResTime() > m_recountTimer)
  {
    int numFound;
    WorldObjectId* ids = g_app->m_location->m_entityGrid->GetFriends(m_pos.x, m_pos.z, m_size, &numFound, m_id.GetTeamId());
    m_entitiesCounted = 0;

    for (int i = 0; i < numFound; ++i)
    {
      WorldObjectId id = ids[i];
      Entity* entity = g_app->m_location->GetEntity(id);
      if (entity && entity->m_type == m_entityTypeRequired && !entity->m_dead)
        ++m_entitiesCounted;
    }

    m_recountTimer = GetHighResTime() + 2.0f;

    if ((m_id.GetTeamId() == 1 && m_entitiesCounted <= m_entitiesRequired) || (m_id.GetTeamId() != 1 && m_entitiesCounted >=
      m_entitiesRequired))
    {
      GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
      if (gb && !gb->m_online)
      {
        gb->m_online = true;
        g_app->m_globalWorld->EvaluateEvents();
      }
    }
  }

  return false;
}

void SafeArea::Render(float predictionTime)
{
}

bool SafeArea::DoesSphereHit(const LegacyVector3& _pos, float _radius) { return false; }

bool SafeArea::DoesShapeHit(Shape* _shape, Matrix34 _transform) { return false; }

bool SafeArea::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                          LegacyVector3* _norm)
{
  if (g_app->m_editing)
    return Building::DoesRayHit(_rayStart, _rayDir, _rayLen, _pos, _norm);
  return false;
}

char* SafeArea::GetObjectiveCounter()
{
  static char result[256];
  sprintf(result, "%s : %d", LANGUAGEPHRASE("objective_currentcount"), m_entitiesCounted);
  return result;
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
