#include "pch.h"
#include "resource.h"
#include "math_utils.h"
#include "ShapeStatic.h"
#include "GameApp.h"
#include "location.h"
#include "SimEventQueue.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "lander.h"

Lander::Lander()
  : Entity(),
    m_state(StateSailing),
    m_spawnTimer(0.0f)
{
  m_type = TypeLander;

  m_shape = g_app->m_resource->GetShapeStatic("lander.shp");
  DEBUG_ASSERT(m_shape);
}

bool Lander::Advance(Unit* _unit)
{
  m_front.Set(-1, 0, 0);

  if (m_dead)
  {
    bool amIDead = AdvanceDead(_unit);
    if (amIDead)
    {
      g_app->m_location->Bang(m_pos, 30.0f, 50.0f);
      return true;
    }
  }

  switch (m_state)
  {
  case StateSailing:
    return AdvanceSailing();
    break;
  case StateLanded:
    return AdvanceLanded();
    break;
  }

  float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
  float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
  if (m_pos.x < 0.0f)
    m_pos.x = 0.0f;
  if (m_pos.z < 0.0f)
    m_pos.z = 0.0f;
  if (m_pos.x >= worldSizeX)
    m_pos.x = worldSizeX;
  if (m_pos.z >= worldSizeZ)
    m_pos.z = worldSizeZ;

  return false;
}

void Lander::ChangeHealth(int amount) { g_simEventQueue.Push(SimEvent::MakeParticle(m_pos, g_zeroVector, SimParticle::TypeMuzzleFlash)); }

bool Lander::AdvanceSailing()
{
  m_vel = m_front * m_stats[StatSpeed];
  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  float groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  m_pos.y = groundLevel;
  if (m_pos.y < 0.0f)
    m_pos.y = 0.0f;

  if (groundLevel > 0.0f)
  {
    m_vel.Zero();
    m_state = StateLanded;
  }

  return false;
}

bool Lander::AdvanceLanded()
{
  m_vel.Zero();

  m_spawnTimer -= SERVER_ADVANCE_PERIOD;
  if (m_spawnTimer <= 0.0f)
  {
    int unitId;
    int numToSpawn = syncfrand(2.0f) + 2.0f;
    Unit* unit = g_app->m_location->m_teams[0].NewUnit(TypeLaserTroop, numToSpawn, &unitId, m_pos);
    g_app->m_location->SpawnEntities(m_pos, m_id.GetTeamId(), unitId, TypeLaserTroop, numToSpawn, g_zeroVector, 0);

    LegacyVector3 offset(0.0f, 0.0f, syncsfrand(200.0f));
    unit->SetWayPoint(m_pos + m_front * 750.0f + offset);

    m_spawnTimer = m_stats[StatRate];
  }

  return false;
}
