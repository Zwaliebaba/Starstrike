#pragma once

#include "List.h"


class Sim;
class SimRegion;
class SimObject;
class SimObserver;
class SimHyper;
class CombatGroup;
class CombatUnit;

class SimEvent
{
  public:
    
    enum EVENT
    {
      LAUNCH = 1,
      DOCK,
      LAND,
      EJECT,
      CRASH,
      COLLIDE,
      DESTROYED,
      MAKE_ORBIT,
      BREAK_ORBIT,
      QUANTUM_JUMP,
      LAUNCH_SHIP,
      RECOVER_SHIP,
      FIRE_GUNS,
      FIRE_MISSILE,
      DROP_DECOY,
      GUNS_KILL,
      MISSILE_KILL,
      LAUNCH_PROBE,
      SCAN_TARGET
    };

    SimEvent(int event, std::string_view tgt = {}, std::string_view info = {});
    ~SimEvent() = default;

    int GetEvent() const { return event; }
    int GetTime() const { return time; }
    std::string GetEventDesc() const;
    std::string GetTarget() const { return target; }
    std::string GetInfo() const { return info; }
    int GetCount() const { return count; }

    void SetTarget(std::string_view tgt);
    void SetInfo(std::string_view info);
    void SetCount(int count);
    void SetTime(int time);

  private:
    int event;
    int time;
    std::string target;
    std::string info;
    int count;
};

class ShipStats
{
  public:
    
    ShipStats(std::string_view name, int iff = 0);
    ~ShipStats();

    static void Initialize();
    static void Close();
    static ShipStats* Find(std::string_view name);
    static int NumStats();
    static ShipStats* GetStats(int i);

    void Summarize();

    std::string GetName() const { return name; }
    std::string GetType() const { return type; }
    std::string GetRole() const { return role; }
    std::string GetRegion() const { return region; }
    CombatGroup* GetCombatGroup() const { return combat_group; }
    CombatUnit* GetCombatUnit() const { return combat_unit; }
    int GetElementIndex() const { return elem_index; }
    int GetShipClass() const { return ship_class; }
    int GetIFF() const { return iff; }
    int GetGunKills() const { return kill1; }
    int GetMissileKills() const { return kill2; }
    int GetDeaths() const { return lost; }
    int GetColls() const { return coll; }
    int GetPoints() const { return points; }
    int GetCommandPoints() const { return cmd_points; }

    int GetGunShots() const { return gun_shots; }
    int GetGunHits() const { return gun_hits; }
    int GetMissileShots() const { return missile_shots; }
    int GetMissileHits() const { return missile_hits; }

    bool IsPlayer() const { return player; }

    List<SimEvent>& GetEvents() { return events; }
    SimEvent* AddEvent(SimEvent* e);
    SimEvent* AddEvent(int event, std::string_view tgt = {}, std::string_view info = {});
    bool HasEvent(int event);

    void SetShipClass(int c) { ship_class = c; }
    void SetIFF(int i) { iff = i; }
    void SetType(std::string_view t);
    void SetRole(std::string_view r);
    void SetRegion(std::string_view r);
    void SetCombatGroup(CombatGroup* g);
    void SetCombatUnit(CombatUnit* u);
    void SetElementIndex(int n);
    void SetPlayer(bool p);

    void AddGunShot() { gun_shots++; }
    void AddGunHit() { gun_hits++; }
    void AddMissileShot() { missile_shots++; }
    void AddMissileHit() { missile_hits++; }
    void AddPoints(int p) { points += p; }
    void AddCommandPoints(int p) { cmd_points += p; }

  private:
    std::string name;
    std::string type;
    std::string role;
    std::string region;
    CombatGroup* combat_group;
    CombatUnit* combat_unit;
    bool player;
    int elem_index;
    int ship_class;
    int iff;
    int kill1;
    int kill2;
    int lost;
    int coll;

    int gun_shots;
    int gun_hits;

    int missile_shots;
    int missile_hits;

    int points;
    int cmd_points;

    List<SimEvent> events;
};
