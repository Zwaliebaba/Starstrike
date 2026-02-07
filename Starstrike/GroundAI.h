#ifndef GroundAI_h
#define GroundAI_h

#include "Director.h"
#include "SimObject.h"

class Ship;
class System;
class CarrierAI;

class GroundAI : public Director, public SimObserver
{
  public:
    GroundAI(SimObject* self);
    ~GroundAI() override;

    void ExecFrame(double seconds) override;
    virtual void SetTarget(SimObject* targ, System* sub = nullptr);
    virtual SimObject* GetTarget() const { return target; }
    virtual System* GetSubTarget() const { return subtarget; }
    int Type() const override;

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

  protected:
    virtual void SelectTarget();

    Ship* ship;
    SimObject* target;
    System* subtarget;
    double exec_time;
    CarrierAI* carrier_ai;
};

#endif GroundAI_h
