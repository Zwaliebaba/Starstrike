#ifndef CombatZone_h
#define CombatZone_h

#include "List.h"

class CombatGroup;
class CombatUnit;
class ZoneForce;

class CombatZone
{
  public:
    CombatZone();
    ~CombatZone();

    int operator ==(const CombatZone& g) const { return this == &g; }

    std::string Name() const { return name; }
    std::string System() const { return system; }
    void AddGroup(CombatGroup* g);
    void RemoveGroup(CombatGroup* g);
    bool HasGroup(CombatGroup* g);
    void AddRegion(std::string_view rgn);
    bool HasRegion(std::string_view rgn);
    std::vector<std::string>& GetRegions() { return regions; }
    List<ZoneForce>& GetForces() { return forces; }

    ZoneForce* FindForce(int iff);
    ZoneForce* MakeForce(int iff);

    void Clear();

    static List<CombatZone>& Load(const char* filename);

  private:
    // attributes:
    std::string name;
    std::string system;
    std::vector<std::string> regions;
    List<ZoneForce> forces;
};

class ZoneForce
{
  public:
    ZoneForce(int i);

    int GetIFF() { return iff; }
    List<CombatGroup>& GetGroups() { return groups; }
    List<CombatGroup>& GetTargetList() { return target_list; }
    List<CombatGroup>& GetDefendList() { return defend_list; }

    void AddGroup(CombatGroup* g);
    void RemoveGroup(CombatGroup* g);
    bool HasGroup(CombatGroup* g);

    int GetNeed(int group_type) const;
    void SetNeed(int group_type, int needed);
    void AddNeed(int group_type, int needed);

  private:
    // attributes:
    int iff;
    List<CombatGroup> groups;
    List<CombatGroup> defend_list;
    List<CombatGroup> target_list;
    int need[8];
};

#endif CombatZone_h
