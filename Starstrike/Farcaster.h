#ifndef Farcaster_h
#define Farcaster_h

#include "Geometry.h"
#include "SimObject.h"
#include "System.h"

class ShipDesign;
class Farcaster;

class Farcaster : public System, public SimObserver
{
  public:
    Farcaster(double capacity, double sink_rate);
    Farcaster(const Farcaster& rhs);
    ~Farcaster() override;

    enum CONSTANTS { NUM_APPROACH_PTS = 4 };

    void ExecFrame(double seconds) override;
    void SetShip(Ship* s) override { ship = s; }
    void SetDest(Ship* d) { dest = d; }

    Point ApproachPoint(int i) const { return approach_point[i]; }
    Point StartPoint() const { return start_point; }
    Point EndPoint() const { return end_point; }

    virtual void SetApproachPoint(int i, Point loc);
    virtual void SetStartPoint(Point loc);
    virtual void SetEndPoint(Point loc);
    virtual void SetCycleTime(double time);

    void Orient(const Physical* rep) override;

    // SimObserver:
    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

    // accessors:
    [[nodiscard]] const Ship* GetShip() const override { return ship; }
    [[nodiscard]] const Ship* GetDest() const { return dest; }

    int ActiveState() const { return active_state; }
    double WarpFactor() const { return warp_fov; }

  protected:
    virtual void Jump();
    virtual void Arrive(Ship* s);

    Ship* ship;
    Ship* dest;
    Ship* jumpship;

    Point start_rel;
    Point end_rel;
    Point approach_rel[NUM_APPROACH_PTS];

    Point start_point;
    Point end_point;
    Point approach_point[NUM_APPROACH_PTS];

    double cycle_time;
    int active_state;
    double warp_fov;

    bool no_dest;
};

#endif Farcaster_h
