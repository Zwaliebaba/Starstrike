#include "pch.h"
#include "CarrierAI.h"
#include "Contact.h"
#include "Element.h"
#include "FlightDeck.h"
#include "FlightPlanner.h"
#include "Game.h"
#include "Hangar.h"
#include "Instruction.h"
#include "Mission.h"
#include "NetUtil.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"

CarrierAI::CarrierAI(Ship* s, int level)
  : m_ship(s),
    exec_time(0),
    hold_time(0),
    ai_level(level)
{
  if (m_ship)
  {
    m_sim = Sim::GetSim();
    m_hangar = m_ship->GetHangar();

    for (int i = 0; i < 4; i++)
      patrol_elem[i] = nullptr;

    if (m_ship)
      m_flightPlanner = std::make_unique<FlightPlanner>(m_ship);

    hold_time = static_cast<int>(Game::GameTime());
  }
}

void CarrierAI::ExecFrame(double secs)
{
  constexpr int INIT_HOLD = 15000;
  constexpr int EXEC_PERIOD = 3000;

  if (!m_sim || !m_ship || !m_hangar)
    return;

  if ((static_cast<int>(Game::GameTime()) - hold_time >= INIT_HOLD) && (static_cast<int>(Game::GameTime()) - exec_time >
    EXEC_PERIOD))
  {
    CheckHostileElements();
    CheckPatrolCoverage();

    exec_time = static_cast<int>(Game::GameTime());
  }
}

bool CarrierAI::CheckPatrolCoverage()
{
  constexpr DWORD PATROL_PERIOD = 900 * 1000;

  // pick up existing patrol elements:

  ListIter<Element> iter = m_sim->GetElements();
  while (++iter)
  {
    Element* elem = iter.value();

    if (elem->GetCarrier() == m_ship && (elem->Type() == Mission::PATROL || elem->Type() == Mission::SWEEP || elem->Type()
      == Mission::AIR_PATROL || elem->Type() == Mission::AIR_SWEEP) && !elem->IsSquadron() && !elem->IsFinished())
    {
      bool found = false;
      int open = -1;

      for (int i = 0; i < 4; i++)
      {
        if (patrol_elem[i] == elem)
          found = true;

        else if (patrol_elem[i] == nullptr && open < 0)
          open = i;
      }

      if (!found && open >= 0)
        patrol_elem[open] = elem;
    }
  }

  // manage the four screening patrols:

  for (int i = 0; i < 4; i++)
  {
    Element* elem = patrol_elem[i];

    if (elem)
    {
      if (elem->IsFinished())
        patrol_elem[i] = nullptr;

      else
        LaunchElement(elem);
    }

    else if (Game::GameTime() - m_hangar->GetLastPatrolLaunch() > PATROL_PERIOD || m_hangar->GetLastPatrolLaunch() == 0)
    {
      Element* patrol = CreatePackage(0, 2, Mission::PATROL, {}, "ACM Medium Range");
      if (patrol)
      {
        patrol_elem[i] = patrol;

        if (m_flightPlanner)
          m_flightPlanner->CreatePatrolRoute(patrol, i);

        m_hangar->SetLastPatrolLaunch(Game::GameTime());
        return true;
      }
    }
  }

  return false;
}

bool CarrierAI::CheckHostileElements()
{
  List<Element> assigned;
  ListIter<Element> iter = m_sim->GetElements();
  while (++iter)
  {
    Element* elem = iter.value();

    // if this element is hostile to us
    // or if the element is a target objective
    // of the carrier, or is hostile to any
    // of our squadrons...

    bool hostile = false;

    if (elem->IsHostileTo(m_ship) || elem->IsObjectiveTargetOf(m_ship))
      hostile = true;
    else
    {
      for (int i = 0; i < m_hangar->NumSquadrons() && !hostile; i++)
      {
        int squadron_iff = m_hangar->SquadronIFF(i);

        if (elem->IsHostileTo(squadron_iff))
          hostile = true;
      }
    }

    if (hostile)
    {
      m_sim->GetAssignedElements(elem, assigned);

      // is one of our fighter elements already assigned to this target?
      bool found = false;
      ListIter<Element> a_iter = assigned;
      while (++a_iter && !found)
      {
        Element* a = a_iter.value();

        if (a->GetCarrier() == m_ship)
          found = true;
      }

      // nobody is assigned yet, create an attack package
      if (!found && CreateStrike(elem))
      {
        hold_time = static_cast<int>(Game::GameTime()) + 30000;
        return true;
      }
    }
  }

  return false;
}

bool CarrierAI::CreateStrike(Element* elem)
{
  Element* strike = nullptr;
  Ship* target = elem->GetShip(1);

  if (target && !target->IsGroundUnit())
  {
    Contact* contact = m_ship->FindContact(target);
    if (contact && contact->GetIFF(m_ship) > 0)
    {
      // fighter intercept
      if (target->IsDropship())
      {
        int squadron = 0;
        if (m_hangar->NumShipsReady(1) >= m_hangar->NumShipsReady(0))
          squadron = 1;

        int count = 2;

        if (count < elem->NumShips())
          count = elem->NumShips();

        strike = CreatePackage(squadron, count, Mission::INTERCEPT, elem->Name(), "ACM Medium Range");

        if (strike)
        {
          strike->SetAssignment(elem);

          if (m_flightPlanner)
            m_flightPlanner->CreateStrikeRoute(strike, elem);
        }
      }

      // starship or station assault
      else
      {
        int squadron = 0;
        if (m_hangar->NumSquadrons() > 1)
          squadron = 1;
        if (m_hangar->NumSquadrons() > 2)
          squadron = 2;

        int count = 2;

        if (target->Class() > Ship::FRIGATE)
        {
          count = 4;
          strike = CreatePackage(squadron, count, Mission::ASSAULT, elem->Name(), "Hvy Ship Strike");
        }
        else
        {
          count = 2;
          strike = CreatePackage(squadron, count, Mission::ASSAULT, elem->Name(), "Ship Strike");
        }

        if (strike)
        {
          strike->SetAssignment(elem);

          if (m_flightPlanner)
            m_flightPlanner->CreateStrikeRoute(strike, elem);

          // strike escort if target has fighter protection:
          if (target->GetHangar())
          {
            if (squadron > 1)
              squadron--;
            Element* escort = CreatePackage(squadron, 2, Mission::ESCORT_STRIKE, strike->Name(), "ACM Short Range");

            if (escort && m_flightPlanner)
              m_flightPlanner->CreateEscortRoute(escort, strike);
          }
        }
      }
    }
  }

  return strike != nullptr;
}

Element* CarrierAI::CreatePackage(int squadron, int size, int code, std::string_view target, std::string_view loadname)
{
  if (squadron < 0 || size < 1 || code < Mission::PATROL || m_hangar->NumShipsReady(squadron) < size)
    return nullptr;

  Sim* sim = Sim::GetSim();
  auto call = sim->FindAvailCallsign(m_ship->GetIFF());
  Element* elem = sim->CreateElement(call, m_ship->GetIFF(), code);
  FlightDeck* deck = nullptr;
  int queue = 1000;
  int* load = nullptr;
  const ShipDesign* design = m_hangar->SquadronDesign(squadron);

  elem->SetSquadron(m_hangar->SquadronName(squadron));
  elem->SetCarrier(m_ship);

  if (!target.empty())
  {
    int i_code = 0;

    switch (code)
    {
      case Mission::ASSAULT:
        i_code = Instruction::ASSAULT;
        break;
      case Mission::STRIKE:
        i_code = Instruction::STRIKE;
        break;

      case Mission::AIR_INTERCEPT:
      case Mission::INTERCEPT:
        i_code = Instruction::INTERCEPT;
        break;

      case Mission::ESCORT:
      case Mission::ESCORT_STRIKE:
      case Mission::ESCORT_FREIGHT:
        i_code = Instruction::ESCORT;
        break;

      case Mission::DEFEND:
        i_code = Instruction::DEFEND;
        break;
    }

    auto objective = NEW Instruction(i_code, target);
    if (objective)
      elem->AddObjective(objective);
  }

  if (design && !loadname.empty())
  {
    ListIter<ShipLoad> sl = (List<ShipLoad>&)design->loadouts;
    while (++sl)
    {
      if (loadname == sl->name)
      {
        load = sl->load;
        elem->SetLoadout(load);
      }
    }
  }

  for (int i = 0; i < m_ship->NumFlightDecks(); i++)
  {
    FlightDeck* d = m_ship->GetFlightDeck(i);

    if (d && d->IsLaunchDeck())
    {
      int dq = m_hangar->PreflightQueue(d);

      if (dq < queue)
      {
        queue = dq;
        deck = d;
      }
    }
  }

  int npackage = 0;
  int slots[4];

  for (int i = 0; i < 4; i++)
    slots[i] = -1;

  for (int slot = 0; slot < m_hangar->SquadronSize(squadron); slot++)
  {
    const HangarSlot* s = m_hangar->GetSlot(squadron, slot);

    if (m_hangar->GetState(s) == Hangar::STORAGE)
    {
      if (npackage < 4)
        slots[npackage] = slot;

      m_hangar->GotoAlert(squadron, slot, deck, elem, load, code > Mission::SWEEP);
      npackage++;

      if (npackage >= size)
        break;
    }
  }

  NetUtil::SendElemCreate(elem, squadron, slots, code <= Mission::SWEEP);

  return elem;
}

bool CarrierAI::LaunchElement(Element* elem)
{
  bool result = false;

  if (!elem)
    return result;

  for (int squadron = 0; squadron < m_hangar->NumSquadrons(); squadron++)
  {
    for (int slot = 0; slot < m_hangar->SquadronSize(squadron); slot++)
    {
      const HangarSlot* s = m_hangar->GetSlot(squadron, slot);

      if (m_hangar->GetState(s) == Hangar::ALERT && m_hangar->GetPackageElement(s) == elem)
      {
        m_hangar->Launch(squadron, slot);
        NetUtil::SendShipLaunch(m_ship, squadron, slot);

        result = true;
      }
    }
  }

  return result;
}
