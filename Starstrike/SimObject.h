#pragma once

#include "List.h"
#include "Physical.h"

class Sim;
class SimRegion;
class SimObserver;
class Scene;

class SimObject : public Physical
{
  friend class SimRegion;

  public:
    
    enum TYPES
    {
      SIM_SHIP = 100,
      SIM_SHOT,
      SIM_DRONE,
      SIM_EXPLOSION,
      SIM_DEBRIS,
      SIM_ASTEROID
    };

    SimObject()
      : region(nullptr),
        objid(0),
        active(false),
        notifying(false) {}

    SimObject(const char* n, int t = 0)
      : Physical(n, t),
        region(nullptr),
        objid(0),
        active(false),
        notifying(false) {}

    ~SimObject() override;

    virtual SimRegion* GetRegion() const { return region; }
    virtual void SetRegion(SimRegion* rgn) { region = rgn; }

    virtual void Notify();
    virtual void Register(SimObserver* obs);
    virtual void Unregister(SimObserver* obs);

    virtual void Activate(Scene& scene);
    virtual void Deactivate(Scene& scene);

    virtual DWORD GetObjID() const { return objid; }
    virtual void SetObjID(DWORD _id) { objid = _id; }

    virtual bool IsHostileTo([[maybe_unused]] const SimObject* o) const { return false; }

  protected:
    SimRegion* region = {};
    List<SimObserver> observers;
    DWORD objid;
    bool active;
    bool notifying;
};

class SimObserver
{
  public:
    
    virtual ~SimObserver();

    int operator ==(const SimObserver& o) const { return this == &o; }

    virtual bool Update(SimObject* obj);
    virtual std::string GetObserverName() const;

    virtual void Observe(SimObject* obj);
    virtual void Ignore(SimObject* obj);

  protected:
    List<SimObject> observe_list;
};
