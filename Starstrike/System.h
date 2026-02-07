#ifndef System_h
#define System_h

#include "Geometry.h"
#include "List.h"
#include "Physical.h"

class Component;
class Ship;
class SystemDesign;

class System
{
  friend Component;

  public:
    
    enum CATEGORY
    {
      MISC_SYSTEM = 0,
      DRIVE       = 1,
      WEAPON,
      SHIELD,
      SENSOR,
      COMPUTER,
      POWER_SOURCE,
      FLIGHT_DECK,
      FARCASTER
    };

    enum STATUS
    {
      DESTROYED,
      CRITICAL,
      DEGRADED,
      NOMINAL,
      MAINT
    };

    enum POWER_FLAGS
    {
      POWER_WATTS    = 1,
      POWER_CRITICAL = 2
    };

    System(CATEGORY t, int s, std::string_view n, int maxv, double energy = 0, double capacity = 100, double sink_rate = 1);
    System(const System& s);
    virtual ~System();

    int operator==(const System& s) const { return this == &s; }

    CATEGORY Type() const { return m_type; }
    int Subtype() const { return subtype; }
    std::string Name() const { return m_name; }
    std::string Abbreviation() const { return abrv; }

    void SetName(std::string_view n) { m_name = n; }
    void SetAbbreviation(std::string_view a) { abrv = a; }
    void SetDesign(SystemDesign* d);

    virtual int Value() const { return static_cast<int>(max_value * availability * 100); }
    int MaxValue() const { return max_value * 100; }
    STATUS Status() const { return status; }
    double Availability() const { return availability * 100; }
    double Safety() const { return safety * 100; }
    double Stability() const { return stability * 100; }
    virtual void CalcStatus();
    virtual void Repair();

    double NetAvail() const { return net_avail; }
    void SetNetAvail(double d) { net_avail = static_cast<float>(d); }

    List<Component>& GetComponents() { return components; }

    virtual void ApplyDamage(double damage);
    virtual void ExecFrame(double seconds);
    virtual void ExecMaintFrame(double seconds);
    virtual void DoEMCON(int emcon);

    // PHYSICAL LOCATION (for inflicting system damage):
    virtual void Orient(const Physical* rep);
    virtual void Mount(Point loc, float radius, float hull_factor = 0.5f);
    virtual void Mount(const System& system);

    Point MountLocation() const { return mount_loc; }
    double Radius() const { return radius; }
    double HullProtection() const { return hull_factor; }

    // POWER UTILIZATION:
    bool IsPowerCritical() const { return (power_flags & POWER_CRITICAL) ? true : false; }
    bool UsesWatts() const { return (power_flags & POWER_WATTS) ? true : false; }

    virtual double GetRequest(double seconds) const;
    virtual void Distribute(double delivered_energy, double seconds);

    int GetSourceIndex() const { return source_index; }
    void SetSourceIndex(int i) { source_index = i; }

    virtual int Charge() const { return static_cast<int>(100 * energy / capacity); }

    bool IsPowerOn() const { return power_on; }
    virtual void PowerOn() { power_on = true; }
    virtual void PowerOff() { power_on = false; }

    // percentage, but stored as 0-1
    virtual double GetPowerLevel() const { return power_level * 100; }
    virtual void SetPowerLevel(double level);
    virtual void SetOverride(bool over);

    // for power drain damage:
    virtual void DrainPower(double to_level);

    void SetCapacity(double c) { capacity = static_cast<float>(c); }
    double GetCapacity() const { return capacity; }
    double GetEnergy() const { return energy; }
    double GetSinkRate() const { return sink_rate; }
    void SetEMCONPower(int emcon, int power_level);
    int GetEMCONPower(int emcon);

    int GetExplosionType() const { return explosion_type; }
    void SetExplosionType(int t) { explosion_type = t; }

    virtual const Ship* GetShip() const { return ship; }
    virtual void SetShip(Ship* s) { ship = s; }
    int GetID() const { return id; }
    void SetID(int n) { id = n; }

  protected:
    // AI information:
    CATEGORY m_type;
    Ship* ship;
    int id;
    int subtype;
    int max_value;

    // Displayable name:
    std::string m_name;
    std::string abrv;

    // System health status:
    STATUS status;
    float crit_level;
    float availability;
    float safety;
    float stability;
    float safety_overload;
    float net_avail;

    // Mounting:
    Point mount_loc;  // world space
    Point mount_rel;  // object space
    float radius;
    float hull_factor;

    // Power Sink:
    float energy;
    float capacity;
    float sink_rate;
    float power_level;
    int source_index;
    DWORD power_flags;
    bool power_on;
    BYTE emcon_power[3];
    BYTE emcon;

    int explosion_type;

    // Subcomponents:
    SystemDesign* m_design;
    List<Component> components;
};

#endif System_h
