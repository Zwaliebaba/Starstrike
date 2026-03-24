#include "pch.h"
#include "resource.h"
#include "egg.h"
#include "GameContext.h"
#include "camera.h"
#include "location.h"
#include "team.h"

Egg::Egg()
  : Entity(),
    m_state(StateDormant),
    m_spiritId(-1),
    m_timer(0.0f) {}

void Egg::ChangeHealth(int amount)
{
  if (!m_dead)
  {
    if (m_stats[StatHealth] + amount < 0)
    {
      m_stats[StatHealth] = 100;
      m_dead = true;
    }
    else if (m_stats[StatHealth] + amount > 255)
      m_stats[StatHealth] = 255;
    else
      m_stats[StatHealth] += amount;
  }
}

bool Egg::Advance(Unit* _unit)
{
  if (g_context->m_location->m_spirits.ValidIndex(m_spiritId))
  {
    Spirit* spirit = g_context->m_location->m_spirits.GetPointer(m_spiritId);
    spirit->m_pos = m_pos + LegacyVector3(0, 3, 0);
  }

  if (!m_dead)
  {
    if (m_state == StateFertilised)
    {
      m_timer += SERVER_ADVANCE_PERIOD;

      if (m_timer >= 15.0f)
      {
        g_context->m_location->m_spirits.MarkNotUsed(m_spiritId);
        g_context->m_location->SpawnEntities(m_pos, m_id.GetTeamId(), -1, TypeVirii, 4, g_zeroVector, 0.0f, 200.0f);
        return true;
      }
    }
    else if (m_state == StateDormant)
    {
      m_timer += SERVER_ADVANCE_PERIOD;
      float maxLife = EntityBlueprint::GetStat(TypeEgg, StatHealth);
      maxLife *= (1.0f - (m_timer / EGG_DORMANTLIFE));
      if (m_stats[StatHealth] > maxLife)
      {
        int change = m_stats[StatHealth] - maxLife;
        ChangeHealth(change * -1);
      }
    }
  }

  if (m_onGround)
  {
    float groundLevel = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    if (m_pos.y < groundLevel)
      m_pos.y = groundLevel;
    else if (m_pos.y > groundLevel)
      m_onGround = false;
  }

  if (m_dead)
  {
    if (g_context->m_location->m_spirits.ValidIndex(m_spiritId))
    {
      Spirit* spirit = g_context->m_location->m_spirits.GetPointer(m_spiritId);
      spirit->EggDestroyed();
      m_spiritId = -1;
    }
  }

  if (!m_onGround)
    AdvanceInAir(_unit);
  else
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  if (m_pos.y <= 0.0f)
    ChangeHealth(-500);

  return Entity::Advance(_unit);
}

void Egg::Fertilise(int spiritId)
{
  if (g_context->m_location->m_spirits.ValidIndex(spiritId))
  {
    m_spiritId = spiritId;
    Spirit* spirit = g_context->m_location->m_spirits.GetPointer(m_spiritId);
    spirit->InEgg();
    m_state = StateFertilised;
    m_timer = 0.0f;
    m_stats[StatHealth] = EntityBlueprint::GetStat(TypeEgg, StatHealth);
  }
}
