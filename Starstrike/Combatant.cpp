#include "pch.h"
#include "Combatant.h"
#include "CombatGroup.h"

static void SetCombatant(CombatGroup* g, Combatant* c)
{
  if (!g)
    return;

  g->SetCombatant(c);

  ListIter<CombatGroup> iter = g->GetComponents();
  while (++iter)
    SetCombatant(iter.value(), c);
}

Combatant::Combatant(std::string_view com_name, std::string_view fname, int team)
  : name(com_name),
    iff(team),
    score(0),
    force(nullptr)
{
  for (int i = 0; i < 6; i++)
    target_factor[i] = 1;

  target_factor[2] = 1000;

  if (!fname.empty())
    force = CombatGroup::LoadOrderOfBattle(fname, iff, this);
}

Combatant::Combatant(std::string_view com_name, CombatGroup* f)
  : name(com_name),
    iff(0),
    score(0),
    force(f)
{
  for (int i = 0; i < 6; i++)
    target_factor[i] = 1;

  target_factor[2] = 1000;

  if (force)
  {
    SetCombatant(force, this);
    iff = force->GetIFF();
  }
}

Combatant::~Combatant()
{
  mission_list.clear();
  target_list.clear();
  defend_list.clear();
  delete force;
}

CombatGroup* Combatant::FindGroup(int type, int id)
{
  if (force)
    return force->FindGroup(type, id);

  return nullptr;
}

void Combatant::AddMission(const Mission* mission) { mission_list.append(mission); }

double Combatant::GetTargetStratFactor(int type)
{
  switch (type)
  {
    case CombatGroup::FLEET:
    case CombatGroup::CARRIER_GROUP:
    case CombatGroup::BATTLE_GROUP:
    case CombatGroup::DESTROYER_SQUADRON:
      return target_factor[0];

    case CombatGroup::WING:
    case CombatGroup::ATTACK_SQUADRON:
    case CombatGroup::INTERCEPT_SQUADRON:
    case CombatGroup::FIGHTER_SQUADRON:
      return target_factor[1];

    case CombatGroup::BATTERY:
    case CombatGroup::MISSILE:
      return target_factor[2];

    case CombatGroup::BATTALION:
    case CombatGroup::STARBASE:
    case CombatGroup::C3I:
    case CombatGroup::COMM_RELAY:
    case CombatGroup::EARLY_WARNING:
    case CombatGroup::FWD_CONTROL_CTR:
    case CombatGroup::ECM:
      return target_factor[3];

    case CombatGroup::SUPPORT:
    case CombatGroup::COURIER:
    case CombatGroup::MEDICAL:
    case CombatGroup::SUPPLY:
    case CombatGroup::REPAIR:
      return target_factor[4];
  }

  return target_factor[5];
}

void Combatant::SetTargetStratFactor(int type, double factor)
{
  switch (type)
  {
    case CombatGroup::FLEET:
    case CombatGroup::CARRIER_GROUP:
    case CombatGroup::BATTLE_GROUP:
    case CombatGroup::DESTROYER_SQUADRON:
      target_factor[0] = factor;
      break;

    case CombatGroup::WING:
    case CombatGroup::ATTACK_SQUADRON:
    case CombatGroup::INTERCEPT_SQUADRON:
    case CombatGroup::FIGHTER_SQUADRON:
      target_factor[1] = factor;
      break;

    case CombatGroup::BATTALION:
    case CombatGroup::STARBASE:
    case CombatGroup::BATTERY:
    case CombatGroup::MISSILE:
      target_factor[2] = factor;
      break;

    case CombatGroup::C3I:
    case CombatGroup::COMM_RELAY:
    case CombatGroup::EARLY_WARNING:
    case CombatGroup::FWD_CONTROL_CTR:
    case CombatGroup::ECM:
      target_factor[3] = factor;
      break;

    case CombatGroup::SUPPORT:
    case CombatGroup::COURIER:
    case CombatGroup::MEDICAL:
    case CombatGroup::SUPPLY:
    case CombatGroup::REPAIR:
      target_factor[4] = factor;
      break;

    default:
      target_factor[5] = factor;
      break;
  }
}
