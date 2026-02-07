#include "pch.h"
#include "GroundAI.h"
#include "CarrierAI.h"
#include "Contact.h"
#include "Game.h"
#include "Physical.h"
#include "Player.h"
#include "Shield.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "SteerAI.h"
#include "System.h"
#include "Weapon.h"
#include "WeaponGroup.h"

GroundAI::GroundAI(SimObject* s)
  : ship(static_cast<Ship*>(s)),
    target(nullptr),
    subtarget(nullptr),
    exec_time(0),
    carrier_ai(nullptr)
{
  Sim* sim = Sim::GetSim();
  Ship* pship = sim->GetPlayerShip();
  int player_team = 1;
  int ai_level = 1;

  if (pship)
    player_team = pship->GetIFF();

  Player* player = Player::GetCurrentPlayer();
  if (player)
  {
    if (ship && ship->GetIFF() && ship->GetIFF() != player_team)
      ai_level = player->AILevel();
    else if (player->AILevel() == 0)
      ai_level = 1;
  }

  // evil alien ships are *always* smart:
  if (ship && ship->GetIFF() > 1 && ship->Design()->auto_roll > 1)
    ai_level = 2;

  if (ship && ship->GetHangar() && ship->GetCommandAILevel() > 0)
    carrier_ai = NEW CarrierAI(ship, ai_level);
}

GroundAI::~GroundAI() { delete carrier_ai; }

void GroundAI::SetTarget(SimObject* targ, System* sub)
{
  if (target != targ)
  {
    target = targ;

    if (target)
      Observe(target);
  }

  subtarget = sub;
}

bool GroundAI::Update(SimObject* obj)
{
  if (obj == target)
  {
    target = nullptr;
    subtarget = nullptr;
  }

  return SimObserver::Update(obj);
}

std::string GroundAI::GetObserverName() const
{
  static char name[64];
  sprintf_s(name, "GroundAI(%s)", ship->Name().c_str());
  return name;
}

void GroundAI::SelectTarget()
{
  SimObject* potential_target = nullptr;

  // pick the closest combatant ship with a different IFF code:
  double target_dist = 1.0e15;

  Ship* current_ship_target = nullptr;

  ListIter<Contact> c_iter = ship->ContactList();
  while (++c_iter)
  {
    Contact* contact = c_iter.value();
    int c_iff = contact->GetIFF(ship);
    Ship* c_ship = contact->GetShip();
    Shot* c_shot = contact->GetShot();
    bool rogue = false;

    if (c_ship)
      rogue = c_ship->IsRogue();

    if (rogue || c_iff > 0 && c_iff != ship->GetIFF() && c_iff < 1000)
    {
      if (c_ship && !c_ship->InTransition())
      {
        // found an enemy, check distance:
        double dist = (ship->Location() - c_ship->Location()).length();

        if (!current_ship_target || (c_ship->Class() <= current_ship_target->Class() && dist < target_dist))
        {
          current_ship_target = c_ship;
          target_dist = dist;
        }
      }
    }

    potential_target = current_ship_target;
  }

  SetTarget(potential_target);
}

int GroundAI::Type() const { return SteerAI::GROUND; }

void GroundAI::ExecFrame(double secs)
{
  constexpr int exec_period = 1000;

  if (static_cast<int>(Game::GameTime()) - exec_time > exec_period)
  {
    exec_time = static_cast<int>(Game::GameTime());
    SelectTarget();
  }

  if (ship)
  {
    Shield* shield = ship->GetShield();

    if (shield)
      shield->SetPowerLevel(100);

    ListIter<WeaponGroup> iter = ship->Weapons();
    while (++iter)
    {
      auto group = iter.value();

      if (group->NumWeapons() > 1 && group->CanTarget(Ship::DROPSHIPS))
        group->SetFiringOrders(Weapon::POINT_DEFENSE);
      else
        group->SetFiringOrders(Weapon::AUTO);

      group->SetTarget(target, nullptr);
    }

    if (carrier_ai)
      carrier_ai->ExecFrame(secs);
  }
}
