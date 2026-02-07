#ifndef QuantumDrive_h
#define QuantumDrive_h

#include "Geometry.h"
#include "System.h"

class Ship;
class SimRegion;

class QuantumDrive : public System
{
  public:
    enum SUBTYPE
    {
      QUANTUM,
      HYPER
    };

    QuantumDrive(SUBTYPE s, double capacity, double sink_rate);
    QuantumDrive(const QuantumDrive& rhs);
    ~QuantumDrive() override;

    enum ACTIVE_STATES
    {
      ACTIVE_READY,
      ACTIVE_COUNTDOWN,
      ACTIVE_PREWARP,
      ACTIVE_POSTWARP
    };

    void SetDestination(SimRegion* rgn, const Point& loc);
    bool Engage(bool immediate = false);
    int ActiveState() const { return active_state; }
    double WarpFactor() const { return warp_fov; }
    double JumpTime() const { return jump_time; }
    void PowerOff() override;

    void ExecFrame(double seconds) override;

    void SetShip(Ship* s) override { ship = s; }
    Ship* GetShip() const override { return ship; }

    double GetCountdown() const { return countdown; }
    void SetCountdown(double d) { countdown = d; }

  protected:
    void Jump();
    void AbortJump();

    int active_state = {};

    Ship* ship = {};
    double warp_fov = {};
    double jump_time = {};
    double countdown = {};

    SimRegion* dst_rgn = {};
    Point dst_loc = {};
};

#endif // QuantumDrive_h
