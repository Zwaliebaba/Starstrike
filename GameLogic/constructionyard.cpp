#include "pch.h"
#include "math_utils.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "preferences.h"
#include "constructionyard.h"
#include "researchitem.h"
#include "armour.h"
#include "GameContext.h"
#include "main.h"
#include "global_world.h"
#include "location.h"
#include "camera.h"
#include "team.h"

ConstructionYard::ConstructionYard()
  : Building(),
    m_numPrimitives(0),
    m_numSurges(0),
    m_numTanksProduced(0),
    m_fractionPopulated(0.0f),
    m_timer(-1.0f),
    m_alpha(0.0f)
{
  m_type = TypeYard;
  SetShape(Resource::GetShapeStatic("constructionyard.shp"));

  m_rung = Resource::GetShapeStatic("constructionyardrung.shp");
  m_primitive = Resource::GetShapeStatic("mineprimitive1.shp");

  for (int i = 0; i < YARD_NUMPRIMITIVES; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerPrimitive0%d", i + 1);
    m_primitives[i] = m_shape->GetMarkerData(name);
    DEBUG_ASSERT(m_primitives[i]);
  }

  for (int i = 0; i < YARD_NUMRUNGSPIKES; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerSpike0%d", i + 1);
    m_rungSpikes[i] = m_rung->GetMarkerData(name);
    DEBUG_ASSERT(m_rungSpikes[i]);
  }
}

bool ConstructionYard::Advance()
{
  m_fractionPopulated = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

  if (m_fractionPopulated > 0.0f && m_numSurges > 0 && m_numPrimitives > 0)
  {
    GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
    if (gb && !gb->m_online)
    {
      gb->m_online = true;
      g_context->m_globalWorld->EvaluateEvents();
    }
  }

  if (m_timer < 0.0f)
  {
    // Not currently building anything
    if (m_numPrimitives == YARD_NUMPRIMITIVES && m_numSurges > 50)
      m_timer = 30.0f;
  }
  else
  {
    // Building a tank
    m_timer -= m_fractionPopulated * SERVER_ADVANCE_PERIOD;
    if (m_timer <= 0.0f)
    {
      if (IsPopulationLocked())
        m_timer = 30.0f;
      else
      {
        m_numPrimitives = 0;
        m_numSurges = 1;

        Matrix34 mat(m_front, g_upVector, m_pos);
        Matrix34 prim = m_shape->GetMarkerWorldMatrix(m_primitives[5], mat);
        WorldObjectId objId = g_context->m_location->SpawnEntities(prim.pos, 2, -1, Entity::TypeArmour, 1, g_zeroVector, 0.0f);
        Entity* entity = g_context->m_location->GetEntity(objId);
        auto armour = static_cast<Armour*>(entity);
        armour->m_front.Set(0, 0, 1);
        armour->m_vel.Zero();
        armour->SetWayPoint(m_pos + LegacyVector3(0, 0, 500));

        ++m_numTanksProduced;
        m_timer = -1.0f;
      }
    }
  }

  return Building::Advance();
}

Matrix34 ConstructionYard::GetRungMatrix1()
{
  if (m_numSurges > 0)
  {
    float rungHeight = 55.0f + sinf(g_gameTime) * 10.0f * m_fractionPopulated;
    LegacyVector3 rungPos = m_pos + LegacyVector3(0, rungHeight, 0);
    LegacyVector3 front = m_front;
    front.RotateAroundY(cosf(g_gameTime * 0.5f) * 0.5f * m_fractionPopulated);

    Matrix34 mat(front, g_upVector, rungPos);
    return mat;
  }
  Matrix34 mat(m_front, g_upVector, m_pos + LegacyVector3(0, 45, 0));
  return mat;
}

Matrix34 ConstructionYard::GetRungMatrix2()
{
  if (m_numSurges > 0)
  {
    float rungHeight = 110.0f + sinf(g_gameTime * 0.8) * 15.0f * m_fractionPopulated;
    LegacyVector3 rungPos = m_pos + LegacyVector3(0, rungHeight, 0);
    LegacyVector3 front = m_front;
    front.RotateAroundY(cosf(g_gameTime * 0.4f) * 0.6f * m_fractionPopulated);
    auto mat = Matrix34(front, g_upVector, rungPos);
    return mat;
  }
  auto mat = Matrix34(m_front, g_upVector, m_pos + LegacyVector3(0, 75, 0));
  return mat;
}

bool ConstructionYard::IsPopulationLocked()
{
  Team* team = g_context->m_location->GetMyTeam();

  int numArmour = 0;
  for (int i = 0; i < team->m_specials.Size(); ++i)
  {
    WorldObjectId id = *team->m_specials.GetPointer(i);
    Entity* entity = g_context->m_location->GetEntity(id);
    if (entity && entity->m_type == Entity::TypeArmour)
      ++numArmour;
  }

  return (numArmour >= 5);
}

bool ConstructionYard::AddPrimitive()
{
  if (m_numPrimitives < YARD_NUMPRIMITIVES)
  {
    ++m_numPrimitives;
    return true;
  }

  return false;
}

void ConstructionYard::AddPowerSurge() { ++m_numSurges; }

// ============================================================================

DisplayScreen::DisplayScreen()
  : Building()
{
  m_type = TypeDisplayScreen;
  SetShape(Resource::GetShapeStatic("displayscreen.shp"));

  m_armour = Resource::GetShapeStatic("armour.shp");

  for (int i = 0; i < DISPLAYSCREEN_NUMRAYS; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerRay0%d", i + 1);
    m_rays[i] = m_shape->GetMarkerData(name);
  }
}
