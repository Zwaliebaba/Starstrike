#ifndef CombatUnit_h
#define CombatUnit_h

#include "Color.h"
#include "Geometry.h"
#include "List.h"

class CombatGroup;
class ShipDesign;

class CombatUnit
{
  public:
    CombatUnit(std::string_view n, std::string_view reg, int t, std::string_view dname, int number, int i);
    CombatUnit(const CombatUnit& unit);

    int operator ==(const CombatUnit& u) const { return this == &u; }

    const char* GetDescription() const;

    int GetValue() const;
    int GetSingleValue() const;
    bool CanDefend(CombatUnit* unit) const;
    bool CanLaunch() const;
    double PowerVersus(CombatUnit* tgt) const;
    int AssignMission();
    void CompleteMission();

    double MaxRange() const;
    double MaxEffectiveRange() const;
    double OptimumRange() const;

    void Engage(CombatUnit* tgt);
    void Disengage();

    // accessors and mutators:
    std::string Name() const { return name; }
    std::string Registry() const { return regnum; }
    std::string DesignName() const { return design_name; }
    std::string Skin() const { return skin; }
    void SetSkin(std::string_view s) { skin = s; }
    int Type() const { return type; }
    int Count() const { return count; }
    int LiveCount() const { return count - dead_count; }
    int DeadCount() const { return dead_count; }
    void SetDeadCount(int n) { dead_count = n; }
    int Kill(int n);
    int Available() const { return available; }
    int GetIFF() const { return iff; }
    bool IsLeader() const { return leader; }
    void SetLeader(bool l) { leader = l; }
    Point Location() const { return location; }
    void MoveTo(const Point& loc);
    std::string GetRegion() const { return region; }
    void SetRegion(std::string_view rgn) { region = rgn; }
    CombatGroup* GetCombatGroup() const { return group; }
    void SetCombatGroup(CombatGroup* g) { group = g; }

    Color MarkerColor() const;
    bool IsGroundUnit() const;
    bool IsStarship() const;
    bool IsDropship() const;
    bool IsStatic() const;

    CombatUnit* GetCarrier() const { return carrier; }
    void SetCarrier(CombatUnit* c) { carrier = c; }

    const ShipDesign* GetDesign();
    int GetShipClass() const;

    List<CombatUnit>& GetAttackers() { return attackers; }

    double GetPlanValue() const { return plan_value; }
    void SetPlanValue(int v) { plan_value = v; }

    double GetSustainedDamage() const { return sustained_damage; }
    void SetSustainedDamage(double d) { sustained_damage = d; }

    double GetHeading() const { return heading; }
    void SetHeading(double d) { heading = d; }

    double GetNextJumpTime() const { return jump_time; }

  private:
    std::string name;
    std::string regnum;
    std::string design_name;
    std::string skin;
    int type;
    const ShipDesign* design;
    int count;
    int dead_count;
    int available;
    int iff;
    bool leader;
    std::string region;
    Point location;
    double plan_value; // scratch pad for plan modules
    double launch_time;
    double jump_time;
    double sustained_damage;
    double heading;

    CombatUnit* carrier;
    List<CombatUnit> attackers;
    CombatUnit* target;
    CombatGroup* group;
};

#endif CombatUnit_h
