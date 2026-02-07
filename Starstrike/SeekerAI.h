#ifndef SeekerAI_h
#define SeekerAI_h

#include "SimObject.h"
#include "SteerAI.h"

class Ship;
class Shot;

class SeekerAI : public SteerAI
{
  public:
    SeekerAI(SimObject* s);
    ~SeekerAI() override;

    int Type() const override { return 1001; }
    int Subframe() const override { return true; }

    void ExecFrame(double seconds) override;
    void FindObjective() override;
    void SetTarget(SimObject* targ, System* sub = nullptr) override;
    virtual bool Overshot();

    virtual void SetPursuit(int p) { pursuit = p; }
    virtual int GetPursuit() const { return pursuit; }

    virtual void SetDelay(double d) { delay = d; }
    virtual double GetDelay() const { return delay; }

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

  protected:
    // behaviors:
    virtual Steer AvoidCollision();
    virtual Steer SeekTarget();

    // accumulate behaviors:
    void Navigator() override;

    virtual void CheckDecoys(double distance);

    Ship* orig_target;
    Shot* shot;
    int pursuit;    // type of pursuit curve
    // 1: pure pursuit
    // 2: lead pursuit

    double delay;      // don't start seeking until then
    bool overshot;
};

#endif SeekerAI_h
