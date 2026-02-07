#ifndef CombatGroup_h
#define CombatGroup_h

#include "Geometry.h"
#include "List.h"

class CombatAssignment;
class Campaign;
class Combatant;
class CombatUnit;
class CombatZone;

class CombatGroup
{
  public:
    
    enum GROUP_TYPE
    {
      FORCE = 1,           // Commander In Chief

      WING,                // Air Force
      INTERCEPT_SQUADRON,  // a2a fighter
      FIGHTER_SQUADRON,    // multi-role fighter
      ATTACK_SQUADRON,     // strike / attack
      LCA_SQUADRON,        // landing craft

      FLEET,               // Navy
      DESTROYER_SQUADRON,  // destroyer
      BATTLE_GROUP,        // heavy cruiser(s)
      CARRIER_GROUP,       // fleet carrier

      BATTALION,           // Army
      MINEFIELD,
      BATTERY,
      MISSILE,
      STATION,             // orbital station
      STARBASE,            // planet-side base

      C3I,                 // Command, Control, Communications, Intelligence
      COMM_RELAY,
      EARLY_WARNING,
      FWD_CONTROL_CTR,
      ECM,
      SUPPORT,
      COURIER,
      MEDICAL,
      SUPPLY,
      REPAIR,
      CIVILIAN,            // root for civilian groups

      WAR_PRODUCTION,
      FACTORY,
      REFINERY,
      RESOURCE,
      INFRASTRUCTURE,
      TRANSPORT,
      NETWORK,
      HABITAT,
      STORAGE,
      NON_COM,             // other civilian traffic
      FREIGHT,
      PASSENGER,
      PRIVATE
    };

    CombatGroup(int t, int n, std::string_view s, int i, int e, CombatGroup* p = nullptr);
    ~CombatGroup();

    // comparison operators are used to sort combat groups into a priority list
    // in DESCENDING order, so the sense of the comparison is backwards from
    // usual...
    int operator <(const CombatGroup& g) const { return value > g.value; }
    int operator <=(const CombatGroup& g) const { return value >= g.value; }
    int operator ==(const CombatGroup& g) const { return this == &g; }

    // operations:
    static CombatGroup* LoadOrderOfBattle(std::string_view fname, int iff, Combatant* combatant);
    static void SaveOrderOfBattle(std::string_view fname, CombatGroup* force);
    static void MergeOrderOfBattle(BYTE* block, std::string_view fname, int iff, Combatant* combatant, Campaign* campaign);

    void AddComponent(CombatGroup* g);
    CombatGroup* FindGroup(int t, int n = -1);
    CombatGroup* Clone(bool deep = true);

    // accessors and mutators:
    const char* GetDescription() const;
    const char* GetShortDescription() const;

    void SetCombatant(Combatant* c) { combatant = c; }

    Combatant* GetCombatant() { return combatant; }
    CombatGroup* GetParent() { return parent; }
    List<CombatGroup>& GetComponents() { return components; }
    List<CombatGroup>& GetLiveComponents() { return live_comp; }
    List<CombatUnit>& GetUnits() { return units; }
    CombatUnit* GetRandomUnit();
    CombatUnit* GetFirstUnit();
    CombatUnit* GetNextUnit();
    CombatUnit* FindUnit(std::string_view name);
    CombatGroup* FindCarrier();

    std::string Name() const { return name; }
    int Type() const { return type; }
    int CountUnits() const;
    int IntelLevel() const { return enemy_intel; }
    int GetID() const { return id; }
    int GetIFF() const { return iff; }
    Point Location() const { return location; }
    void MoveTo(const Point& loc);
    std::string GetRegion() const { return region; }
    void SetRegion(std::string rgn) { region = rgn; }
    void AssignRegion(std::string rgn);
    int Value() const { return value; }
    int Sorties() const { return sorties; }
    void SetSorties(int n) { sorties = n; }
    int Kills() const { return kills; }
    void SetKills(int n) { kills = n; }
    int Points() const { return points; }
    void SetPoints(int n) { points = n; }
    int UnitIndex() const { return unit_index; }

    double GetNextJumpTime() const;

    double GetPlanValue() const { return plan_value; }
    void SetPlanValue(double v) { plan_value = v; }

    bool IsAssignable() const;
    bool IsTargetable() const;
    bool IsDefensible() const;
    bool IsStrikeTarget() const;
    bool IsMovable() const;
    bool IsFighterGroup() const;
    bool IsStarshipGroup() const;
    bool IsReserve() const;

    // these two methods return zero terminated arrays of
    // integers identifying the preferred assets for attack
    // or defense in priority order:
    static const int* PreferredAttacker(int type);
    static const int* PreferredDefender(int type);

    bool IsExpanded() const { return expanded; }
    void SetExpanded(bool e) { expanded = e; }

    std::string GetAssignedSystem() const { return assigned_system; }
    void SetAssignedSystem(std::string_view s);
    CombatZone* GetCurrentZone() const { return current_zone; }
    void SetCurrentZone(CombatZone* z) { current_zone = z; }
    CombatZone* GetAssignedZone() const { return assigned_zone; }
    void SetAssignedZone(CombatZone* z);
    void ClearUnlockedZones();
    bool IsZoneLocked() const { return assigned_zone && zone_lock; }
    void SetZoneLock(bool lock = true);
    bool IsSystemLocked() const { return assigned_system.length() > 0; }

    std::string GetStrategicDirection() const { return strategic_direction; }
    void SetStrategicDirection(std::string dir) { strategic_direction = dir; }

    void SetIntelLevel(int n);
    int CalcValue();

    List<CombatAssignment>& GetAssignments() { return assignments; }
    void ClearAssignments();

    static int TypeFromName(std::string_view name);
    static std::string_view NameFromType(int type);

  private:
    const char* GetOrdinal() const;

    // attributes:
    int type;
    int id;
    std::string name;
    int iff;
    int enemy_intel;

    double plan_value; // scratch pad for plan modules

    List<CombatUnit> units;
    List<CombatGroup> components;
    List<CombatGroup> live_comp;
    Combatant* combatant;
    CombatGroup* parent;
    std::string region;
    Point location;
    int value;
    int unit_index;

    int sorties;
    int kills;
    int points;

    bool expanded;   // for tree control

    std::string assigned_system;
    CombatZone* current_zone;
    CombatZone* assigned_zone;
    bool zone_lock;
    List<CombatAssignment> assignments;

    std::string strategic_direction;
};

#endif CombatGroup_h
