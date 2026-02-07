#ifndef Instruction_h
#define Instruction_h

#include "Geometry.h"
#include "RLoc.h"
#include "SimObject.h"

class Ship;

class Instruction : public SimObserver
{
  public:
    
    enum ACTION
    {
      VECTOR,
      LAUNCH,
      DOCK,
      RTB,
      DEFEND,
      ESCORT,
      PATROL,
      SWEEP,
      INTERCEPT,
      STRIKE,     // ground attack
      ASSAULT,    // starship attack
      RECON,

      //RECALL,
      //DEPLOY,

      NUM_ACTIONS
    };

    enum STATUS
    {
      PENDING,
      ACTIVE,
      SKIPPED,
      ABORTED,
      FAILED,
      COMPLETE,
      NUM_STATUS
    };

    enum FORMATION
    {
      DIAMOND,
      SPREAD,
      BOX,
      TRAIL,
      NUM_FORMATIONS
    };

    enum PRIORITY
    {
      PRIMARY = 1,
      SECONDARY,
      BONUS
    };

    Instruction(int action, std::string_view tgt = {});
    Instruction(std::string_view rgn, Point loc, int act = VECTOR);
    Instruction(SimRegion* rgn, Point loc, int act = VECTOR);
    Instruction(const Instruction& instr);
    ~Instruction() override;

    Instruction& operator =(const Instruction& n);

    // accessors:
    static std::string ActionName(int a);
    static std::string StatusName(int s);
    static std::string FormationName(int f);
    static std::string PriorityName(int p);

    std::string RegionName() const { return rgn_name; }
    SimRegion* Region() const { return region; }
    Point Location() const;
    RLoc& GetRLoc() { return rloc; }
    int Action() const { return action; }
    int Status() const { return status; }
    int Formation() const { return formation; }
    int Speed() const { return speed; }
    int EMCON() const { return emcon; }
    int WeaponsFree() const { return wep_free; }
    int Priority() const { return priority; }
    int Farcast() const { return farcast; }
    double HoldTime() const { return hold_time; }

    std::string TargetName() const { return tgt_name; }
    std::string TargetDesc() const { return tgt_desc; }
    SimObject* GetTarget();

    void Evaluate(Ship* s);
    std::string GetShortDescription() const;
    std::string GetDescription() const;

    // mutators:
    void SetRegion(SimRegion* r) { region = r; }
    void SetLocation(const Point& l);
    void SetAction(int s) { action = s; }
    void SetStatus(int s);
    void SetFormation(int s) { formation = s; }
    void SetSpeed(int s) { speed = s; }
    void SetEMCON(int e) { emcon = e; }
    void SetWeaponsFree(int f) { wep_free = f; }
    void SetPriority(int p) { priority = p; }
    void SetFarcast(int f) { farcast = f; }
    void SetHoldTime(double t) { hold_time = t; }

    void SetTarget(std::string_view n);
    void SetTarget(SimObject* _s);
    void SetTargetDesc(std::string_view d);
    void ClearTarget();

    bool Update(SimObject* s) override;
    std::string GetObserverName() const override;

  protected:
    std::string rgn_name;
    SimRegion* region;
    RLoc rloc;
    int action;
    int formation;
    int status;
    int speed;

    std::string tgt_name;
    std::string tgt_desc;
    SimObject* target;
    int emcon;
    int wep_free;
    int priority;
    int farcast;

    double hold_time;
};

#endif Instruction_h
