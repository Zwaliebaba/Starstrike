#include "pch.h"
#include "Element.h"
#include "Instruction.h"
#include "NetUtil.h"
#include "RadioHandler.h"
#include "RadioMessage.h"
#include "Ship.h"
#include "Sim.h"

static int id_key = 1000;

Element::Element(std::string_view call_sign, int a_iff, int a_type)
  : id(id_key++),
    iff(a_iff),
    type(a_type),
    player(0),
    command_ai(1),
    respawns(0),
    intel(0),
    name(call_sign),
    count(0),
    commander(nullptr),
    assignment(nullptr),
    carrier(nullptr),
    combat_group(nullptr),
    combat_unit(nullptr),
    launch_time(0),
    hold_time(0),
    rogue(false),
    playable(true),
    zone_lock(false)
{
  if (call_sign.empty())
  {
    name = "Pkg " + std::to_string(id);
  }

  SetLoadout(nullptr);
}

Element::~Element()
{
  flight_plan.destroy();
  objectives.destroy();

  for (int i = 0; i < ships.size(); i++)
    ships[i]->SetElement(nullptr);

  respawns = 0;
}

int Element::AddShip(Ship* ship, int index)
{
  if (ship && !ships.contains(ship))
  {
    Observe(ship);

    if (index < 0)
    {
      ships.append(ship);
      index = ships.size();
    }
    else
      ships.insert(ship, index - 1);

    ship->SetElement(this);

    if (respawns < ship->RespawnCount())
      respawns = ship->RespawnCount();
  }

  return index;
}

void Element::DelShip(Ship* ship)
{
  if (ship && ships.contains(ship))
  {
    ships.remove(ship);
    ship->SetElement(nullptr);

    if (ships.isEmpty())
      respawns = ship->RespawnCount();
  }
}

Ship* Element::GetShip(int index)
{
  if (index >= 1 && index <= ships.size())
    return ships[index - 1];

  return nullptr;
}

int Element::GetShipClass()
{
  if (ships.size())
    return ships[0]->Class();

  return 0;
}

int Element::FindIndex(const Ship* s) { return ships.index(s) + 1; }

bool Element::Contains(const Ship* s) { return ships.contains(s); }

bool Element::IsActive() const
{
  bool active = false;

  for (int i = 0; i < ships.size() && !active; i++)
  {
    Ship* s = ships[i];
    if (s->Life() && s->MissionClock())
      active = true;
  }

  return active;
}

bool Element::IsFinished() const
{
  bool finished = false;

  if (launch_time > 0 && respawns < 1)
  {
    finished = true;

    if (ships.size() > 0)
    {
      for (int i = 0; i < ships.size() && finished; i++)
      {
        Ship* s = ships[i];
        if (s->RespawnCount() > 0 || s->MissionClock() == 0 || s->Life() && !s->GetInbound())
          finished = false;
      }
    }
  }

  return finished;
}

bool Element::IsNetObserver() const
{
  bool observer = !IsSquadron();

  for (int i = 0; i < ships.size() && observer; i++)
  {
    Ship* s = ships[i];

    if (!s->IsNetObserver())
      observer = false;
  }

  return observer;
}

bool Element::IsSquadron() const { return count > 0; }

bool Element::IsStatic() const
{
  if (IsSquadron() || IsFinished())
    return false;

  const Ship* s = ships.at(0);
  if (s && s->IsStatic())
    return true;

  return false;
}

bool Element::IsHostileTo(const Ship* s) const
{
  if (iff <= 0 || iff >= 100 || !s || launch_time == 0 || IsFinished())
    return false;

  if (IsSquadron())
    return false;

  if (s->IsRogue())
    return true;

  int s_iff = s->GetIFF();

  if (s_iff <= 0 || s_iff >= 100 || s_iff == iff)
    return false;

  if (ships.size() > 0 && ships[0]->GetRegion() != s->GetRegion())
    return false;

  return true;
}

bool Element::IsHostileTo(int iff_code) const
{
  if (iff <= 0 || iff >= 100 || launch_time == 0 || IsFinished())
    return false;

  if (IsSquadron())
    return false;

  if (iff_code <= 0 || iff_code >= 100 || iff_code == iff)
    return false;

  return true;
}

bool Element::IsObjectiveTargetOf(const Ship* s) const
{
  if (!s || launch_time == 0 || IsFinished())
    return false;

  const char* e_name = Name().data();
  size_t e_len = Name().length();

  Instruction* orders = s->GetRadioOrders();
  if (orders && orders->Action() > Instruction::SWEEP)
  {
    auto o_name = orders->TargetName();
    size_t o_len = std::min(e_len, o_name.size());

    if (!strncmp(e_name, o_name.c_str(), o_len))
      return true;
  }

  Element* elem = s->GetElement();
  if (elem)
  {
    for (int i = 0; i < elem->NumObjectives(); i++)
    {
      Instruction* obj = elem->GetObjective(i);

      if (obj)
      {
        auto o_name = obj->TargetName();
        size_t o_len = 0;

          o_len = std::min(e_len, o_name.size());

        if (!strncmp(e_name, o_name.c_str(), o_len))
          return true;
      }
    }
  }

  return false;
}

void Element::SetLaunchTime(DWORD t)
{
  if (launch_time == 0 || t == 0)
    launch_time = t;
}

double Element::GetHoldTime() { return hold_time; }

void Element::SetHoldTime(double t)
{
  if (t >= 0)
    hold_time = t;
}

bool Element::GetZoneLock() { return zone_lock; }

void Element::SetZoneLock(bool z) { zone_lock = z; }

void Element::SetLoadout(int* l)
{
  if (l)
    CopyMemory(load, l, sizeof(load));
  else
  {
    for (int i = 0; i < 16; i++)
      load[i] = -1;
  }
}

bool Element::Update(SimObject* obj)
{
  // false alarm, keep watching:
  if (obj->Life() != 0)
  {
    DebugTrace("Element (%s) false update on (%s) life = %f\n", Name().data(), obj->Name(), obj->Life());
    return false;
  }

  auto s = static_cast<Ship*>(obj);
  ships.remove(s);

  if (ships.isEmpty())
    respawns = s->RespawnCount();

  return SimObserver::Update(obj);
}

std::string Element::GetObserverName() const { return std::string("Element ") + Name(); }

void Element::AddNavPoint(Instruction* pt, Instruction* afterPoint, bool send)
{
  if (pt && !flight_plan.contains(pt))
  {
    int index = -1;

    if (afterPoint)
    {
      index = flight_plan.index(afterPoint);

      if (index > -1)
        flight_plan.insert(pt, index + 1);
      else
        flight_plan.append(pt);
    }

    else
      flight_plan.append(pt);

    if (send)
      NetUtil::SendNavData(true, this, index, pt);
  }
}

void Element::DelNavPoint(Instruction* pt, bool send)
{
  // XXX MEMORY LEAK
  // This is a small memory leak, but I'm not sure if it is
  // safe to delete the navpoint when removing it from the
  // flight plan.  Other ships in the element might have
  // pointers to the object...?

  if (pt)
  {
    int index = flight_plan.index(pt);
    flight_plan.remove(pt);

    if (send)
      NetUtil::SendNavDelete(this, index);
  }
}

void Element::ClearFlightPlan(bool send)
{
  hold_time = 0;
  flight_plan.destroy();
  objectives.destroy();
  instructions.clear();

  if (send)
    NetUtil::SendNavDelete(this, -1);
}

Instruction* Element::GetNextNavPoint()
{
  if (hold_time <= 0 && flight_plan.size() > 0)
  {
    ListIter<Instruction> iter = flight_plan;
    while (++iter)
    {
      Instruction* navpt = iter.value();

      if (navpt->Status() == Instruction::COMPLETE && navpt->HoldTime() > 0)
        return navpt;

      if (navpt->Status() <= Instruction::ACTIVE)
        return navpt;
    }
  }

  return nullptr;
}

int Element::GetNavIndex(const Instruction* n)
{
  int index = 0;

  if (flight_plan.size() > 0)
  {
    ListIter<Instruction> navpt = flight_plan;
    while (++navpt)
    {
      index++;
      if (navpt.value() == n)
        return index;
    }
  }

  return 0;
}

List<Instruction>& Element::GetFlightPlan() { return flight_plan; }

int Element::FlightPlanLength() { return flight_plan.size(); }

void Element::ClearObjectives() { objectives.destroy(); }

void Element::AddObjective(Instruction* obj) { objectives.append(obj); }

Instruction* Element::GetObjective(int index)
{
  if (objectives.isEmpty())
    return nullptr;

  if (index < 0)
    index = 0;

  else if (index >= objectives.size())
    index = index % objectives.size();

  return objectives.at(index);
}

Instruction* Element::GetTargetObjective()
{
  for (int i = 0; i < objectives.size(); i++)
  {
    Instruction* obj = objectives[i];

    if (obj->Status() <= Instruction::ACTIVE)
    {
      switch (obj->Action())
      {
        case Instruction::INTERCEPT:
        case Instruction::STRIKE:
        case Instruction::ASSAULT:
        case Instruction::SWEEP:
        case Instruction::PATROL:
        case Instruction::RECON:
        case Instruction::ESCORT:
        case Instruction::DEFEND:
          return obj;

        default:
          break;
      }
    }
  }

  return nullptr;
}

void Element::ClearInstructions() { instructions.clear(); }

void Element::AddInstruction(const std::string& _instr) { instructions.emplace_back(_instr); }

std::string Element::GetInstruction(int index)
{
  if (instructions.empty())
    return {};

  index = std::max(index, 0);

  if (index >= instructions.size())
    index = index % instructions.size();

  return instructions.at(index);
}

void Element::ResumeAssignment()
{
  SetAssignment(nullptr);

  if (objectives.isEmpty())
    return;

  Instruction* objective = nullptr;

  for (int i = 0; i < objectives.size() && !objective; i++)
  {
    Instruction* instr = objectives[i];

    if (instr->Status() <= Instruction::ACTIVE)
    {
      switch (instr->Action())
      {
        case Instruction::INTERCEPT:
        case Instruction::STRIKE:
        case Instruction::ASSAULT:
          objective = instr;
          break;
      }
    }
  }

  if (objective)
  {
    Sim* sim = Sim::GetSim();

    ListIter<Element> iter = sim->GetElements();
    while (++iter)
    {
      Element* elem = iter.value();
      SimObject* tgt = objective->GetTarget();

      if (tgt && tgt->Type() == SimObject::SIM_SHIP && elem->Contains(static_cast<const Ship*>(tgt)))
      {
        SetAssignment(elem);
        return;
      }
    }
  }
}

void Element::HandleRadioMessage(RadioMessage* msg)
{
  if (!msg)
    return;

  static RadioHandler rh;

  // if this is a message from within the element,
  // then all ships should report in.  Otherwise,
  // just the leader will acknowledge the message.
  int full_report = ships.contains(msg->Sender());
  int reported = false;

  ListIter<Ship> s = ships;
  while (++s)
  {
    if (rh.ProcessMessage(msg, s.value()))
    {
      if (full_report)
      {
        if (s.value() != msg->Sender())
          rh.AcknowledgeMessage(msg, s.value());
      }

      else if (!reported)
      {
        rh.AcknowledgeMessage(msg, s.value());
        reported = true;
      }
    }
  }
}

bool Element::CanCommand(Element* e)
{
  while (e)
  {
    if (e->commander == this)
      return true;
    e = e->commander;
  }

  return false;
}

void Element::ExecFrame(double seconds)
{
  if (hold_time > 0)
  {
    hold_time -= seconds;
    return;
  }

  ListIter<Instruction> iter = flight_plan;
  while (++iter)
  {
    Instruction* instr = iter.value();

    if (instr->Status() == Instruction::COMPLETE && instr->HoldTime() > 0)
      instr->SetHoldTime(instr->HoldTime() - seconds);
  }
}

void Element::SetIFF(int iff)
{
  for (int i = 0; i < ships.size(); i++)
    ships[i]->SetIFF(iff);
}
