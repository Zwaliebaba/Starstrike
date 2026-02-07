#pragma once

#include "Weapon.h"

class WeaponGroup
{
  public:
    WeaponGroup(std::string_view name);
    ~WeaponGroup();

    void ExecFrame(double factor);

    // identification:
    std::string Name() const { return name; }
    std::string Abbreviation() const { return abrv; }
    void SetName(std::string_view n);
    void SetAbbreviation(std::string_view a);

    bool IsPrimary() const;
    bool IsDrone() const;
    bool IsDecoy() const;
    bool IsProbe() const;
    bool IsMissile() const;
    bool IsBeam() const;
    int Value() const;

    // weapon list:
    void AddWeapon(Weapon* w);
    int NumWeapons() const;
    List<Weapon>& GetWeapons();
    bool Contains(const Weapon* w) const;

    // weapon selection:
    void SelectWeapon(int n);
    void CycleWeapon();
    Weapon* GetWeapon(int n) const;
    Weapon* GetSelected() const;

    // operations:
    bool GetTrigger() const { return trigger; }
    void SetTrigger(bool t = true) { trigger = t; }
    int Ammo() const { return ammo; }
    float Mass() const { return mass; }
    float Resistance() const { return resist; }
    void CheckAmmo();

    void SetTarget(SimObject* t, System* sub = nullptr);
    SimObject* GetTarget() const;
    System* GetSubTarget() const;
    void DropTarget();
    void SetFiringOrders(int o);
    int GetFiringOrders() const { return orders; }
    void SetControlMode(int m);
    int GetControlMode() const { return control; }
    void SetSweep(int s);
    int GetSweep() const { return sweep; }
    int Status() const;

    WeaponDesign* GetDesign() const;
    bool CanTarget(DWORD tgt_class) const;

    void PowerOn();
    void PowerOff();

  protected:
    // Displayable name:
    std::string name;
    std::string abrv;

    List<Weapon> m_weapons;

    int selected;
    bool trigger;
    int ammo;

    int orders;
    int control;
    int sweep;

    float mass;
    float resist;
};
