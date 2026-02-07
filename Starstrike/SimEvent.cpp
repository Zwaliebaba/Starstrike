#include "pch.h"
#include "SimEvent.h"
#include "Game.h"
#include "Sim.h"

List<ShipStats> g_records;

SimEvent::SimEvent(int e, std::string_view t, std::string_view i)
  : event(e),
    count(0)
{
  if (Sim* sim = Sim::GetSim())
    time = static_cast<int>(sim->MissionClock());
  else
    time = static_cast<int>(Game::GameTime() / 1000);

  SetTarget(t);
  SetInfo(i);
}

void SimEvent::SetTime(int t) { time = t; }

void SimEvent::SetTarget(std::string_view t)
{
  if (!t.empty())
    target = t;
}

void SimEvent::SetInfo(std::string_view i)
{
  if (!i.empty())
    info = i;
}

void SimEvent::SetCount(int c) { count = c; }

std::string SimEvent::GetEventDesc() const
{
  switch (event)
  {
    case LAUNCH:
      return Game::GetText("sim.event.Launch");
    case DOCK:
      return Game::GetText("sim.event.Dock");
    case LAND:
      return Game::GetText("sim.event.Land");
    case EJECT:
      return Game::GetText("sim.event.Eject");
    case CRASH:
      return Game::GetText("sim.event.Crash");
    case COLLIDE:
      return Game::GetText("sim.event.Collision With");
    case DESTROYED:
      return Game::GetText("sim.event.Destroyed By");
    case MAKE_ORBIT:
      return Game::GetText("sim.event.Make Orbit");
    case BREAK_ORBIT:
      return Game::GetText("sim.event.Break Orbit");
    case QUANTUM_JUMP:
      return Game::GetText("sim.event.Quantum Jump");
    case LAUNCH_SHIP:
      return Game::GetText("sim.event.Launch Ship");
    case RECOVER_SHIP:
      return Game::GetText("sim.event.Recover Ship");
    case FIRE_GUNS:
      return Game::GetText("sim.event.Fire Guns");
    case FIRE_MISSILE:
      return Game::GetText("sim.event.Fire Missile");
    case DROP_DECOY:
      return Game::GetText("sim.event.Drop Decoy");
    case GUNS_KILL:
      return Game::GetText("sim.event.Guns Kill");
    case MISSILE_KILL:
      return Game::GetText("sim.event.Missile Kill");
    case LAUNCH_PROBE:
      return Game::GetText("sim.event.Launch Probe");
    case SCAN_TARGET:
      return Game::GetText("sim.event.Scan Target");
    default:
      return Game::GetText("sim.event.no event");
  }
}

ShipStats::ShipStats(std::string_view n, int i)
  : name(n),
    combat_group(nullptr),
    combat_unit(nullptr),
    player(false),
    elem_index(-1),
    ship_class(0),
    iff(i),
    kill1(0),
    kill2(0),
    lost(0),
    coll(0),
    gun_shots(0),
    gun_hits(0),
    missile_shots(0),
    missile_hits(0),
    points(0),
    cmd_points(0)
{
  if (!n.empty())
    name = Game::GetText("[unknown]");
}

ShipStats::~ShipStats() { events.destroy(); }

void ShipStats::SetType(std::string_view t)
{
  if (!t.empty())
    type = t;
}

void ShipStats::SetRole(std::string_view r)
{
  if (!r.empty())
    role = r;
}

void ShipStats::SetRegion(std::string_view r)
{
  if (!r.empty())
    region = r;
}

void ShipStats::SetCombatGroup(CombatGroup* g) { combat_group = g; }

void ShipStats::SetCombatUnit(CombatUnit* u) { combat_unit = u; }

void ShipStats::SetElementIndex(int n) { elem_index = n; }

void ShipStats::SetPlayer(bool p) { player = p; }

void ShipStats::Summarize()
{
  kill1 = 0;
  kill2 = 0;
  lost = 0;
  coll = 0;

  ListIter<SimEvent> iter = events;
  while (++iter)
  {
    SimEvent* event = iter.value();
    int code = event->GetEvent();

    if (code == SimEvent::GUNS_KILL)
      kill1++;

    else if (code == SimEvent::MISSILE_KILL)
      kill2++;

    else if (code == SimEvent::DESTROYED)
      lost++;

    else if (code == SimEvent::CRASH)
      coll++;

    else if (code == SimEvent::COLLIDE)
      coll++;
  }
}

SimEvent* ShipStats::AddEvent(SimEvent* e)
{
  events.append(e);
  return e;
}

SimEvent* ShipStats::AddEvent(int event, std::string_view tgt, std::string_view info)
{
  auto e = NEW SimEvent(event, tgt, info);
  events.append(e);
  return e;
}

bool ShipStats::HasEvent(int event)
{
  for (int i = 0; i < events.size(); i++)
  {
    if (events[i]->GetEvent() == event)
      return true;
  }

  return false;
}

void ShipStats::Initialize() { g_records.destroy(); }
void ShipStats::Close() { g_records.destroy(); }

int ShipStats::NumStats() { return g_records.size(); }

ShipStats* ShipStats::GetStats(int i)
{
  if (i >= 0 && i < g_records.size())
    return g_records.at(i);

  return nullptr;
}

ShipStats* ShipStats::Find(std::string_view name)
{
  if (!name.empty())
  {
    ListIter<ShipStats> iter = g_records;
    while (++iter)
    {
      ShipStats* stats = iter.value();
      if (stats->GetName() == name)
        return stats;
    }

    auto stats = NEW ShipStats(name);
    g_records.append(stats);
    return stats;
  }

  return nullptr;
}
