#ifndef Component_h
#define Component_h



class ComponentDesign
{
  public:
    
    ComponentDesign();
    ~ComponentDesign();
    int operator ==(const ComponentDesign& rhs) const { return (name == rhs.name); }

    // identification:
    std::string name;
    std::string abrv;

    float repair_time;
    float replace_time;
    int spares;
    DWORD affects;
};

class System;

class Component
{
  public:
    
    enum STATUS
    {
      DESTROYED,
      CRITICAL,
      DEGRADED,
      NOMINAL,
      REPLACE,
      REPAIR
    };

    enum DAMAGE
    {
      DAMAGE_EFFICIENCY = 0x01,
      DAMAGE_SAFETY     = 0x02,
      DAMAGE_STABILITY  = 0x04
    };

    Component(ComponentDesign* d, System* s);
    Component(const Component& c);
    virtual ~Component();

    std::string Name() const { return design->name; }
    std::string Abbreviation() const { return design->abrv; }
    float RepairTime() const { return design->repair_time; }
    float ReplaceTime() const { return design->replace_time; }

    bool DamageEfficiency() const { return (design->affects & DAMAGE_EFFICIENCY) ? true : false; }
    bool DamageSafety() const { return (design->affects & DAMAGE_SAFETY) ? true : false; }
    bool DamageStability() const { return (design->affects & DAMAGE_STABILITY) ? true : false; }

    STATUS Status() const { return status; }
    float Availability() const;
    float TimeRemaining() const;
    int SpareCount() const;
    bool IsJerried() const;
    int NumJerried() const;

    void SetSystem(System* s) { system = s; }
    System* GetSystem() const { return system; }

    virtual void ApplyDamage(double damage);
    virtual void ExecMaintFrame(double seconds);
    virtual void Repair();
    virtual void Replace();

  protected:
    ComponentDesign* design;

    // Component health status:
    STATUS status;
    float availability;
    float time_remaining;
    int spares;
    int jerried;
    System* system;
};

#endif Component_h
