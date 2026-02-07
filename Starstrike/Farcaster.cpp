#include "pch.h"
#include "Farcaster.h"
#include "Element.h"
#include "Explosion.h"
#include "Game.h"
#include "Instruction.h"
#include "QuantumDrive.h"
#include "Ship.h"
#include "Sim.h"

Farcaster::Farcaster(double cap, double rate)
  : System(FARCASTER, 0, "Farcaster", 1, static_cast<float>(cap), static_cast<float>(cap), static_cast<float>(rate)),
    ship(nullptr),
    dest(nullptr),
    jumpship(nullptr),
    cycle_time(10),
    active_state(QuantumDrive::ACTIVE_READY),
    warp_fov(1),
    no_dest(false)
{
  m_name = Game::GetText("sys.farcaster");
  abrv = Game::GetText("sys.farcaster.abrv");
}

Farcaster::Farcaster(const Farcaster& s)
  : System(s),
    ship(nullptr),
    dest(nullptr),
    jumpship(nullptr),
    start_rel(s.start_rel),
    end_rel(s.end_rel),
    cycle_time(s.cycle_time),
    active_state(QuantumDrive::ACTIVE_READY),
    warp_fov(1),
    no_dest(false)
{
  Mount(s);
  SetAbbreviation(s.Abbreviation());

  for (int i = 0; i < NUM_APPROACH_PTS; i++)
    approach_rel[i] = s.approach_rel[i];
}

Farcaster::~Farcaster() {}

void Farcaster::ExecFrame(double seconds)
{
  System::ExecFrame(seconds);

  if (ship && !no_dest)
  {
    if (!dest)
    {
      Element* elem = ship->GetElement();

      if (elem->NumObjectives())
      {
        Sim* sim = Sim::GetSim();
        Instruction* obj = elem->GetObjective(0);

        if (obj)
          dest = sim->FindShip(obj->TargetName());
      }

      if (!dest)
        no_dest = true;
    }
    else
    {
      if (dest->IsDying() || dest->IsDead())
      {
        dest = nullptr;
        no_dest = true;
      }
    }
  }

  // if no destination, show red nav lights:
  if (no_dest)
    energy = 0.0f;

  if (active_state == QuantumDrive::ACTIVE_READY && energy >= capacity && ship && ship->GetRegion() && dest && dest->
    GetRegion())
  {
    SimRegion* rgn = ship->GetRegion();
    SimRegion* dst = dest->GetRegion();
    ListIter<Ship> s_iter = rgn->Ships();

    jumpship = nullptr;

    while (++s_iter)
    {
      Ship* s = s_iter.value();

      if (s == ship || s->IsStatic() || s->WarpFactor() > 1)
        continue;

      Point delta = s->Location() - ship->Location();

      // activate:
      if (delta.length() < 1000)
      {
        active_state = QuantumDrive::ACTIVE_PREWARP;
        jumpship = s;
        Observe(jumpship);
        break;
      }
    }
  }

  if (active_state == QuantumDrive::ACTIVE_READY)
    return;

  if (ship)
  {
    bool warping = false;

    if (active_state == QuantumDrive::ACTIVE_PREWARP)
    {
      if (warp_fov < 5000)
        warp_fov *= 1.5;
      else
        Jump();

      warping = true;
    }

    else if (active_state == QuantumDrive::ACTIVE_POSTWARP)
    {
      if (warp_fov > 1)
        warp_fov *= 0.75;
      else
      {
        warp_fov = 1;
        active_state = QuantumDrive::ACTIVE_READY;
      }

      warping = true;
    }

    if (jumpship)
    {
      if (warping)
      {
        jumpship->SetWarp(warp_fov);

        SimRegion* r = ship->GetRegion();
        ListIter<Ship> neighbor = r->Ships();

        while (++neighbor)
        {
          if (neighbor->IsDropship())
          {
            Ship* s = neighbor.value();
            Point delta = s->Location() - ship->Location();

            if (delta.length() < 5e3)
              s->SetWarp(warp_fov);
          }
        }
      }
      else
      {
        warp_fov = 1;
        jumpship->SetWarp(warp_fov);
      }
    }
  }
}

void Farcaster::Jump()
{
  Sim* sim = Sim::GetSim();
  SimRegion* rgn = ship->GetRegion();
  SimRegion* dst = dest->GetRegion();

  sim->CreateExplosion(jumpship->Location(), Point(0, 0, 0), Explosion::QUANTUM_FLASH, 1.0f, 0, rgn);
  sim->RequestHyperJump(jumpship, dst, dest->Location().OtherHand(), 0, ship, dest);

  energy = 0.0f;

  Farcaster* f = dest->GetFarcaster();
  if (f)
    f->Arrive(jumpship);

  active_state = QuantumDrive::ACTIVE_READY;
  warp_fov = 1;
  jumpship = nullptr;
}

void Farcaster::Arrive(Ship* s)
{
  energy = 0.0f;

  active_state = QuantumDrive::ACTIVE_POSTWARP;
  warp_fov = 5000;
  jumpship = s;

  if (jumpship && jumpship->Velocity().length() < 500)
    jumpship->SetVelocity(jumpship->Heading() * 500.0f);
}

void Farcaster::SetApproachPoint(int i, Point loc)
{
  if (i >= 0 && i < NUM_APPROACH_PTS)
    approach_rel[i] = loc;
}

void Farcaster::SetStartPoint(Point loc) { start_rel = loc; }

void Farcaster::SetEndPoint(Point loc) { end_rel = loc; }

void Farcaster::SetCycleTime(double t) { cycle_time = t; }

void Farcaster::Orient(const Physical* rep)
{
  System::Orient(rep);

  Matrix orientation = rep->Cam().Orientation();
  Point loc = rep->Location();

  start_point = (start_rel * orientation) + loc;
  end_point = (end_rel * orientation) + loc;

  for (int i = 0; i < NUM_APPROACH_PTS; i++)
    approach_point[i] = (approach_rel[i] * orientation) + loc;
}

bool Farcaster::Update(SimObject* obj)
{
  if (obj == jumpship)
  {
    jumpship->SetWarp(1);

    SimRegion* r = ship->GetRegion();
    ListIter<Ship> neighbor = r->Ships();

    while (++neighbor)
    {
      if (neighbor->IsDropship())
      {
        Ship* s = neighbor.value();
        Point delta = s->Location() - ship->Location();

        if (delta.length() < 5e3)
          s->SetWarp(1);
      }
    }

    jumpship = nullptr;
  }

  return SimObserver::Update(obj);
}

std::string Farcaster::GetObserverName() const { return Name(); }
