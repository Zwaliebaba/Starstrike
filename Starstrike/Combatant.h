#ifndef Combatant_h
#define Combatant_h

#include "List.h"

class CombatGroup;
class Mission;

class Combatant
{
  public:
    
    Combatant(std::string_view com_name, std::string_view file_name, int iff);
    Combatant(std::string_view com_name, CombatGroup* force);
    ~Combatant();

    // operations:

    // accessors:
    std::string Name() const { return name; }
    int GetIFF() const { return iff; }
    int Score() const { return score; }
    std::string GetDescription() const { return name; }
    CombatGroup* GetForce() { return force; }
    CombatGroup* FindGroup(int type, int id = -1);
    List<CombatGroup>& GetTargetList() { return target_list; }
    List<CombatGroup>& GetDefendList() { return defend_list; }
    List<Mission>& GetMissionList() { return mission_list; }

    void AddMission(const Mission* m);
    void SetScore(int points) { score = points; }
    void AddScore(int points) { score += points; }

    double GetTargetStratFactor(int type);
    void SetTargetStratFactor(int type, double f);

  private:
    std::string name;
    int iff;
    int score;

    CombatGroup* force;
    List<CombatGroup> target_list;
    List<CombatGroup> defend_list;
    List<Mission> mission_list;

    double target_factor[8];
};

#endif Combatant_h
