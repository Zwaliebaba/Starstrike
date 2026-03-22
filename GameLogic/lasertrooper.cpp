#include "pch.h"
#include "math_utils.h"
#include "GameAppSim.h"
#include "location.h"
#include "team.h"
#include "unit.h"
#include "entity_grid.h"
#include "camera.h"
#include "lasertrooper.h"

void LaserTrooper::Begin()
{
  Entity::Begin();

  m_victoryDance = -1.0f;
}

bool LaserTrooper::Advance(Unit* _unit)
{
  if (m_targetPos == g_zeroVector)
    m_targetPos = m_pos;

  if (m_enabled && m_onGround && !m_dead)
  {
    LegacyVector3 movementDir = (m_targetPos - m_pos).Normalise();
    float distance = (m_targetPos - m_pos).Mag();
    float speed = m_stats[StatSpeed];
    if (speed * SERVER_ADVANCE_PERIOD > distance)
      speed = distance / SERVER_ADVANCE_PERIOD;
    m_vel = movementDir * speed;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_front = m_vel;
    m_front.Normalise();

    if (EnterTeleports())
      return true;
  }

  if (!m_onGround)
    AdvanceInAir(_unit);
  if (m_inWater != -1)
    AdvanceInWater(_unit);

  if (m_reloading > 0.0f)
  {
    m_reloading -= SERVER_ADVANCE_PERIOD;
    if (m_reloading < 0.0f)
      m_reloading = 0.0f;
  }

  if (m_dead)
  {
    bool amIDead = AdvanceDead(_unit);
    if (amIDead)
      return true;
  }

  if (m_victoryDance != -1.0f)
    AdvanceVictoryDance();

  _unit->UpdateEntityPosition(m_pos, m_radius);

  return false;
}

void LaserTrooper::AdvanceVictoryDance()
{
  if (syncfrand(100.0f) < 1.0f)
  {
    m_vel.Zero();
    m_vel.y += 10.0f + syncfrand(10.0f);
    m_onGround = false;
  }
}
