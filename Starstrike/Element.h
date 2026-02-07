#pragma once

#include "List.h"
#include "SimObject.h"

class Ship;
class Instruction;
class RadioMessage;
class CombatGroup;
class CombatUnit;

class Element : public SimObserver
{
  public:
    // CONSTRUCTORS:
    Element(std::string_view call_sign, int iff, int type = 0 /*PATROL*/);
    ~Element() override;

    int operator ==(const Element& e) const { return id == e.id; }

    // GENERAL ACCESSORS:
    int Identity() const { return id; }
    int Type() const { return type; }
    std::string Name() const { return name; }
    void SetName(const char* s) { name = s; }
    virtual int GetIFF() const { return iff; }
    int Player() const { return player; }
    void SetPlayer(int p) { player = p; }
    DWORD GetLaunchTime() const { return launch_time; }
    void SetLaunchTime(DWORD t);
    int IntelLevel() const { return intel; }
    void SetIntelLevel(int i) { intel = i; }

    // ELEMENT COMPONENTS:
    int NumShips() const { return ships.size(); }
    int AddShip(Ship*, int index = -1);
    void DelShip(Ship*);
    Ship* GetShip(int index);
    int GetShipClass();
    int FindIndex(const Ship* s);
    bool Contains(const Ship* s);
    bool IsActive() const;
    bool IsFinished() const;
    bool IsNetObserver() const;
    bool IsSquadron() const;
    bool IsStatic() const;
    bool IsHostileTo(const Ship* s) const;
    bool IsHostileTo(int iff_code) const;
    bool IsObjectiveTargetOf(const Ship* s) const;
    bool IsRogue() const { return rogue; }
    bool IsPlayable() const { return playable; }
    int* Loadout() { return load; }

    void SetRogue(bool r) { rogue = r; }
    void SetPlayable(bool p) { playable = p; }
    void SetLoadout(int* l);
    virtual void SetIFF(int iff);

    virtual void ExecFrame(double seconds);
    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

    // OBJECTIVES:
    void ClearObjectives();
    void AddObjective(Instruction* obj);
    Instruction* GetObjective(int index);
    Instruction* GetTargetObjective();
    int NumObjectives() const { return objectives.size(); }

    void ClearInstructions();
    void AddInstruction(const std::string& _instr);
    std::string GetInstruction(int index);
    int NumInstructions() const { return instructions.size(); }

    // ORDERS AND NAVIGATION:
    double GetHoldTime();
    void SetHoldTime(double t);
    bool GetZoneLock();
    void SetZoneLock(bool z);
    void AddNavPoint(Instruction* pt, Instruction* afterPoint = nullptr, bool send = true);
    void DelNavPoint(Instruction* pt, bool send = true);
    void ClearFlightPlan(bool send = true);
    Instruction* GetNextNavPoint();
    int GetNavIndex(const Instruction* n);
    List<Instruction>& GetFlightPlan();
    int FlightPlanLength();
    virtual void HandleRadioMessage(RadioMessage* msg);

    // CHAIN OF COMMAND:
    Element* GetCommander() const { return commander; }
    void SetCommander(Element* e) { commander = e; }
    Element* GetAssignment() const { return assignment; }
    void SetAssignment(Element* e) { assignment = e; }
    void ResumeAssignment();
    bool CanCommand(Element* e);
    Ship* GetCarrier() const { return carrier; }
    void SetCarrier(Ship* c) { carrier = c; }
    int GetCommandAILevel() const { return command_ai; }
    void SetCommandAILevel(int n) { command_ai = n; }
    std::string GetSquadron() const { return squadron; }
    void SetSquadron(std::string_view s) { squadron = s; }

    // DYNAMIC CAMPAIGN:
    CombatGroup* GetCombatGroup() const { return combat_group; }
    void SetCombatGroup(CombatGroup* g) { combat_group = g; }
    CombatUnit* GetCombatUnit() const { return combat_unit; }
    void SetCombatUnit(CombatUnit* u) { combat_unit = u; }

    // SQUADRON STUFF:
    int GetCount() const { return count; }
    void SetCount(int n) { count = n; }

  protected:
    int id;
    int iff;
    int type;
    int player;
    int command_ai;
    int respawns;
    int intel;
    std::string name;

    // squadron elements only:
    int count;

    List<Ship> ships;
    std::vector<std::string> ship_names;
    std::vector<std::string> instructions;
    List<Instruction> objectives;
    List<Instruction> flight_plan;

    Element* commander;
    Element* assignment;
    Ship* carrier;
    std::string squadron;

    CombatGroup* combat_group;
    CombatUnit* combat_unit;
    DWORD launch_time;
    double hold_time;

    bool rogue;
    bool playable;
    bool zone_lock;
    int load[16];
};
