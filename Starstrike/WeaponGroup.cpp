#include "pch.h"
#include "WeaponGroup.h"
#include "Ship.h"

WeaponGroup::WeaponGroup(std::string_view n)
  : name(n),
    selected(0),
    trigger(false),
    ammo(0),
    orders(Weapon::MANUAL),
    control(Weapon::SINGLE_FIRE),
    sweep(Weapon::SWEEP_TIGHT),
    mass(0.0f),
    resist(0.0f) {}

WeaponGroup::~WeaponGroup() { m_weapons.destroy(); }

void WeaponGroup::SetName(std::string_view n) { name = n; }

void WeaponGroup::SetAbbreviation(std::string_view a) { abrv = a; }

bool WeaponGroup::IsPrimary() const
{
  if (m_weapons.size() > 0)
    return m_weapons[0]->IsPrimary();

  return false;
}

bool WeaponGroup::IsDrone() const
{
  if (m_weapons.size() > 0)
    return m_weapons[0]->IsDrone();

  return false;
}

bool WeaponGroup::IsDecoy() const
{
  if (m_weapons.size() > 0)
    return m_weapons[0]->IsDecoy();

  return false;
}

bool WeaponGroup::IsProbe() const
{
  if (m_weapons.size() > 0)
    return m_weapons[0]->IsProbe();

  return false;
}

bool WeaponGroup::IsMissile() const
{
  if (m_weapons.size() > 0)
    return m_weapons[0]->IsMissile();

  return false;
}

bool WeaponGroup::IsBeam() const
{
  if (m_weapons.size() > 0)
    return m_weapons[0]->IsBeam();

  return false;
}

void WeaponGroup::AddWeapon(Weapon* w) { m_weapons.append(w); }

int WeaponGroup::NumWeapons() const { return m_weapons.size(); }

List<Weapon>& WeaponGroup::GetWeapons() { return m_weapons; }

bool WeaponGroup::Contains(const Weapon* w) const { return m_weapons.contains(w); }

void WeaponGroup::SelectWeapon(int n)
{
  if (n >= 0 && n < m_weapons.size())
    selected = n;
}

void WeaponGroup::CycleWeapon()
{
  selected++;

  if (selected >= m_weapons.size())
    selected = 0;
}

Weapon* WeaponGroup::GetWeapon(int n) const
{
  if (n >= 0 && n < m_weapons.size())
    return m_weapons[n];

  return nullptr;
}

Weapon* WeaponGroup::GetSelected() const { return m_weapons[selected]; }

bool WeaponGroup::CanTarget(DWORD tgt_class) const
{
  if (selected >= 0 && selected < m_weapons.size())
    return m_weapons[selected]->CanTarget(tgt_class);

  return false;
}

void WeaponGroup::ExecFrame(double seconds)
{
  ammo = 0;
  mass = 0.0f;
  resist = 0.0f;

  ListIter<Weapon> iter = m_weapons;
  while (++iter)
  {
    Weapon* w = iter.value();
    w->ExecFrame(seconds);

    ammo += w->Ammo();
    mass += w->Mass();
    resist += w->Resistance();
  }
}

void WeaponGroup::CheckAmmo()
{
  ammo = 0;
  mass = 0.0f;
  resist = 0.0f;

  ListIter<Weapon> iter = m_weapons;
  while (++iter)
  {
    Weapon* w = iter.value();

    ammo += w->Ammo();
    mass += w->Mass();
    resist += w->Resistance();
  }
}

void WeaponGroup::SetTarget(SimObject* target, System* subtarget)
{
  ListIter<Weapon> w = m_weapons;
  while (++w)
    w->SetTarget(target, subtarget);
}

SimObject* WeaponGroup::GetTarget() const
{
  SimObject* target = nullptr;

  if (m_weapons.size())
    target = m_weapons[0]->GetTarget();

  return target;
}

System* WeaponGroup::GetSubTarget() const
{
  System* subtarget = nullptr;

  if (m_weapons.size())
    subtarget = m_weapons[0]->GetSubTarget();

  return subtarget;
}

void WeaponGroup::DropTarget()
{
  ListIter<Weapon> w = m_weapons;
  while (++w)
    w->SetTarget(nullptr, nullptr);
}

WeaponDesign* WeaponGroup::GetDesign() const
{
  if (selected >= 0 && selected < m_weapons.size())
    return (WeaponDesign*)m_weapons[selected]->Design();

  return nullptr;
}

int WeaponGroup::Status() const
{
  int status = System::NOMINAL;
  int critical = true;

  ListIter<Weapon> iter = (List<Weapon>&)m_weapons; // cast-away const
  while (++iter)
  {
    Weapon* w = iter.value();

    if (w->Status() < System::NOMINAL)
      status = System::DEGRADED;

    if (w->Status() > System::CRITICAL)
      critical = false;
  }

  if (critical)
    return System::CRITICAL;

  return status;
}

void WeaponGroup::SetFiringOrders(int o)
{
  orders = o;

  ListIter<Weapon> w = m_weapons;
  while (++w)
    w->SetFiringOrders(orders);
}

void WeaponGroup::SetControlMode(int m)
{
  control = m;

  ListIter<Weapon> w = m_weapons;
  while (++w)
    w->SetControlMode(control);
}

void WeaponGroup::SetSweep(int s)
{
  sweep = s;

  ListIter<Weapon> w = m_weapons;
  while (++w)
    w->SetSweep(sweep);
}

void WeaponGroup::PowerOff()
{
  ListIter<Weapon> w = m_weapons;
  while (++w)
    w->PowerOff();
}

void WeaponGroup::PowerOn()
{
  ListIter<Weapon> w = m_weapons;
  while (++w)
    w->PowerOn();
}

int WeaponGroup::Value() const
{
  int result = 0;

  for (int i = 0; i < m_weapons.size(); i++)
  {
    const Weapon* w = m_weapons[i];
    result += w->Value();
  }

  return result;
}
