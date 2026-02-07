#ifndef MouseController_h
#define MouseController_h

#include "MotionController.h"

class MouseController : public MotionController
{
  public:
    
    MouseController();
    ~MouseController() override;

    // setup
    void MapKeys(KeyMapEntry* mapping, int nkeys) override;

    // sample the physical device
    void Acquire() override;

    // translations
    double X() override { return 0; }
    double Y() override { return 0; }
    double Z() override { return 0; }

    // rotations
    double Pitch() override
    {
      if (active)
        return p;
      return 0;
    }

    double Roll() override
    {
      if (active)
        return r;
      return 0;
    }

    double Yaw() override
    {
      if (active)
        return w;
      return 0;
    }

    int Center() override { return 0; }

    // throttle
    double Throttle() override
    {
      if (active)
        return t;
      return 0;
    }

    void SetThrottle(double _throttle) override { t = _throttle; }

    // actions
    int Action(int n) override { return action[n]; }
    int ActionMap(int n) override;

    // actively sampling?
    virtual bool Active() { return active; }
    virtual void SetActive(bool a) { active = a; }

    static MouseController* GetInstance();

  protected:
    double p, r, w, dx, dy, t;
    int action[MaxActions];
    int map[32];
    bool active;
    int active_key;
};

#endif MouseController_h
