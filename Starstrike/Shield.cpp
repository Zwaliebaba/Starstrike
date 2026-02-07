#include "pch.h"
#include "Shield.h"
#include "Game.h"
#include "Shot.h"
#include "WeaponDesign.h"

static const char* shield_name[] = {"sys.shield.none", "sys.shield.deflector", "sys.shield.grav", "sys.shield.hyper"};

static int shield_value[] = {0, 2, 2, 3};

Shield::Shield(SUBTYPE shield_type)
  : System(SHIELD, shield_type, "shield", shield_value[shield_type], 100, 0),
    shield_capacitor(false),
    shield_bubble(false),
    shield_curve(0.05f),
    shield_cutoff(0.0f),
    deflection_cost(1.0f)
{
  m_name = Game::GetText(shield_name[shield_type]);
  abrv = Game::GetText("sys.shield.abrv");

  power_flags = POWER_WATTS | POWER_CRITICAL;
  energy = 0.0f;
  power_level = 0.0f;
  shield_level = 0.0f;

  switch (shield_type)
  {
    default: case DEFLECTOR:
      capacity = sink_rate = 2.0e3f;
      shield_factor = 0.05f;
      break;

    case GRAV_SHIELD:
      capacity = sink_rate = 7.0e3f;
      shield_factor = 0.01f;
      break;

    case HYPER_SHIELD:
      capacity = sink_rate = 10.0e3f;
      shield_factor = 0.003f;
      break;
  }

  emcon_power[0] = 0;
  emcon_power[1] = 0;
  emcon_power[2] = 100;
}

Shield::Shield(const Shield& s)
  : System(s),
    shield_capacitor(s.shield_capacitor),
    shield_bubble(s.shield_bubble),
    shield_factor(s.shield_factor),
    shield_curve(s.shield_curve),
    shield_cutoff(s.shield_cutoff),
    requested_power_level(0.0f),
    deflection_cost(s.deflection_cost)
{
  power_flags = s.power_flags;
  energy = 0.0f;
  power_level = 0.0f;
  shield_level = 0.0f;

  Mount(s);
}

Shield::~Shield() {}

void Shield::SetShieldCapacitor(bool c)
{
  shield_capacitor = c;

  if (shield_capacitor)
  {
    power_flags = POWER_CRITICAL;
    shield_curve = 0.05f;
  }
  else
  {
    power_flags = POWER_WATTS | POWER_CRITICAL;
    shield_curve = 0.25f;
  }
}

void Shield::ExecFrame(double seconds)
{
  System::ExecFrame(seconds);

  if (power_level < requested_power_level)
  {
    power_level += static_cast<float>(seconds * 0.10);     // ten seconds to charge up

    if (power_level > requested_power_level)
      power_level = requested_power_level;
  }
  else if (power_level > requested_power_level)
  {
    power_level -= static_cast<float>(seconds * 0.20);     // five seconds to power down

    if (power_level < requested_power_level)
      power_level = requested_power_level;
  }

  if (power_level < 0.01 && !shield_capacitor)
  {
    shield_level = 0.0f;
    energy = 0.0f;
  }
}

void Shield::Distribute(double delivered_energy, double seconds)
{
  System::Distribute(delivered_energy, seconds);

  if (shield_capacitor)
  {
    if (shield_cutoff > 0 && shield_cutoff < 0.999)
    {
      float cutoff = shield_cutoff * capacity;

      if (energy > cutoff)
        shield_level = (energy - cutoff) / (capacity - cutoff);
      else
        shield_level = 0.0f;
    }

    else
      shield_level = energy / capacity;
  }
  else
  {
    shield_level = energy / sink_rate;
    energy = 0.0f;
  }

  if (shield_level < 0)
    shield_level = 0;
}

double Shield::DeflectDamage(Shot* shot, double damage)
{
  double filter = 1;
  double penetration = 5;
  double leak = 0;

  if (shot)
    penetration = shot->Design()->penetration;

  filter = 1 - shield_factor * penetration;

  if (filter < 0)
    filter = 0;

  else if (filter > 1)
    filter = 1;

  if (shield_capacitor)
  {
    if (shield_cutoff > 0 && shield_level < 1e-6)
    {
      leak = damage;
      energy -= static_cast<float>(damage * deflection_cost);
    }

    else
    {
      leak = damage * (1 - pow(shield_level, shield_curve) * filter * availability);

      double deflected = damage - leak;
      energy -= static_cast<float>(deflected) * deflection_cost;
    }
  }
  else
    leak = damage * (1 - pow(shield_level, shield_curve) * filter * availability);

  return leak;
}

void Shield::SetPowerLevel(double level)
{
  if (level > 100)
    level = 100;
  else if (level < 0)
    level = 0;

  level /= 100;

  if (requested_power_level != level)
  {
    // if the system is on emergency override power,
    // do not let the EMCON system use this method
    // to drop it back to normal power:
    if (power_level > 1 && level == 1)
    {
      requested_power_level = power_level;
      return;
    }

    requested_power_level = static_cast<float>(level);
  }
}

void Shield::SetNetShieldLevel(int level)
{
  if (level > 100)
    level = 100;
  else
    if (level < 0)
      level = 0;

  requested_power_level = static_cast<float>(level / 100.0);
  power_level = requested_power_level;
}

void Shield::DoEMCON(int index)
{
  int e = GetEMCONPower(index);

  if (power_level * 100 > e || emcon != index)
  {
    if (e == 0)
      PowerOff();
    else if (emcon != index)
    {
      PowerOn();

      if (power_level * 100 > e)
        SetPowerLevel(e);
    }
  }

  emcon = index;
}
