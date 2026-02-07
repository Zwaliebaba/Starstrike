#include "pch.h"
#include "Computer.h"
#include "Game.h"

Computer::Computer(int comp_type, std::string_view comp_name)
  : System(COMPUTER, comp_type, comp_name, 1, 1, 1, 1)
{
  SetAbbreviation(Game::GetText("sys.computer.abrv"));
  power_flags = POWER_WATTS | POWER_CRITICAL;

  if (subtype == FLIGHT)
    crit_level = -1.0f;
}

Computer::Computer(const Computer& c)
  : System(c)
{
  System::Mount(c);
  SetAbbreviation(c.Abbreviation());
  power_flags = POWER_WATTS | POWER_CRITICAL;

  if (subtype == FLIGHT)
    crit_level = -1.0f;
}

Computer::~Computer() {}

void Computer::ApplyDamage(double damage) { System::ApplyDamage(damage); }

void Computer::ExecFrame(double seconds)
{
  energy = 0.0f;
  System::ExecFrame(seconds);
}

void Computer::Distribute(double delivered_energy, double seconds)
{
  if (IsPowerOn())
  {
    // convert Joules to Watts:
    energy = static_cast<float>(delivered_energy / seconds);

    // brown out:
    if (energy < capacity * 0.75f)
      power_on = false;

      // spike:
    else if (energy > capacity * 1.5f)
    {
      power_on = false;
      ApplyDamage(50);
    }
  }
}
