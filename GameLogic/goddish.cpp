#include "pch.h"
#include "resource.h"
#include "math_utils.h"
#include "preferences.h"
#include "goddish.h"
#include "spam.h"
#include "darwinian.h"
#include "GameContext.h"
#include "globals.h"
#include "global_world.h"
#include "location.h"
#include "team.h"
#include "camera.h"
#include "main.h"
#include "GameSimEventQueue.h"

GodDish::GodDish()
  : Building(),
    m_activated(false),
    m_timer(0.0f),
    m_numSpawned(0),
    m_spawnSpam(false)
{
  m_type = TypeGodDish;
  SetShape(Resource::GetShapeStatic("goddish.shp"));
}

void GodDish::Initialise(Building* _template) { Building::Initialise(_template); }

bool GodDish::Advance()
{
  if (m_activated)
    m_timer += SERVER_ADVANCE_PERIOD;
  else
    m_timer *= (1.0f - SERVER_ADVANCE_PERIOD * 2.0f);

  return Building::Advance();
}

bool GodDish::IsInView()
{
  if (m_activated)
    return true;

  return Building::IsInView();
}

void GodDish::Activate()
{
  m_activated = true;
  m_timer = 0.0f;

  //
  // Make all green darwinians watch us

  Team* team = &g_context->m_location->m_teams[0];

  for (int i = 0; i < team->m_others.Size(); ++i)
  {
    if (team->m_others.ValidIndex(i))
    {
      Entity* entity = team->m_others[i];
      if (entity && entity->m_type == Entity::TypeDarwinian)
      {
        auto darwinian = static_cast<Darwinian*>(entity);
        darwinian->WatchSpectacle(m_id.GetUniqueId());
        darwinian->CastShadow(m_id.GetUniqueId());
      }
    }
  }

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "ConnectToGod"));
}

void GodDish::DeActivate()
{
  m_activated = false;

  g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id));
  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "DisconnectFromGod"));
}

void GodDish::SpawnSpam(bool _isResearch)
{
  Spam spamTemplate;
  int buildingId = g_context->m_globalWorld->GenerateBuildingId();
  spamTemplate.m_id.SetUniqueId(buildingId);
  spamTemplate.m_id.SetUnitId(UNIT_BUILDINGS);

  auto spam = static_cast<Spam*>(CreateBuilding(TypeSpam));
  spam->Initialise(&spamTemplate);
  g_context->m_location->m_buildings.PutData(spam);

  spam->SendFromHeaven();
  if (_isResearch)
    spam->SetAsResearch();
  spam->m_pos = m_pos;
  spam->m_pos += LegacyVector3(0, 1500 * 0.75f, 900 * 0.75f);
  spam->m_vel = (m_pos - spam->m_pos);
  spam->m_vel.SetLength(80.0f);
}

void GodDish::TriggerSpam()
{
  for (int i = 0; i < g_context->m_location->m_buildings.Size(); ++i)
  {
    if (g_context->m_location->m_buildings.ValidIndex(i))
    {
      Building* b = g_context->m_location->m_buildings[i];
      if (b && b->m_type == TypeSpam)
      {
        auto spam = static_cast<Spam*>(b);
        spam->SpawnInfection();
      }
    }
  }
}
