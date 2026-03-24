#include "pch.h"
#include "cave.h"
#include "GameContext.h"
#include "GameSimEventQueue.h"
#include "ShapeStatic.h"
#include "entity_grid.h"
#include "location.h"
#include "math_utils.h"
#include "resource.h"
#include "team.h"
#include "unit.h"

Cave::Cave()
  : Building(),
    m_troopType(-1),
    m_unitId(-1),
    m_spawnTimer(0.0f),
    m_dead(false)
{
  m_type = TypeCave;
  m_troopType = Entity::TypeVirii;

  SetShape(Resource::GetShapeStatic("cave.shp"));

  m_spawnPoint = m_shape->GetMarkerData("MarkerSpawnPoint");
  DEBUG_ASSERT(m_spawnPoint);
}

bool Cave::Advance()
{
  if (m_dead)
    return true;

  if (m_id.GetTeamId() < 0 || m_id.GetTeamId() >= NUM_TEAMS)
    return false;
  if (g_context->m_location->m_teams[m_id.GetTeamId()].m_teamType == Team::TeamTypeUnused)
    return false;

  m_spawnTimer -= SERVER_ADVANCE_PERIOD;

  if (m_spawnTimer <= 0.0f)
  {
    //
    // Only spawn if the area is sufficiently empty

    Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_spawnPoint, rootMat);
    LegacyVector3 spawnPoint = worldMat.pos;
    spawnPoint.y = g_context->m_location->m_landscape.m_heightMap->GetValue(spawnPoint.x, spawnPoint.z);

    int numFound;
    WorldObjectId* objs = g_context->m_location->m_entityGrid->GetFriends(spawnPoint.x, spawnPoint.z, 100.0f, &numFound, m_id.GetTeamId());
    if (numFound < 30)
      g_context->m_location->SpawnEntities(spawnPoint, m_id.GetTeamId(), m_id.GetUniqueId(), m_troopType, 1, g_zeroVector, 0.0f);

    m_spawnTimer = syncfrand(0.5f);
  }

  return false;
}

void Cave::Damage(float _damage)
{
  if (_damage > 80.0f)
  {
    Matrix34 mat(m_front, g_upVector, m_pos);
    g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, mat, 1.0f));
    m_dead = true;
  }
}
