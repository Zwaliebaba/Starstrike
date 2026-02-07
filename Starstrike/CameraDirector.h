#pragma once

#include "Camera.h"
#include "SimObject.h"
#include "List.h"

class HUDView;
class Orbital;
class Ship;
class Sim;

class CameraDirector : public SimObserver
{
public:
  enum CAM_MODE
  {
    MODE_NONE,

    MODE_COCKPIT,
    MODE_CHASE,
    MODE_TARGET,
    MODE_THREAT,
    MODE_ORBIT,
    MODE_VIRTUAL,
    MODE_TRANSLATE,
    MODE_ZOOM,
    MODE_DOCKING,
    MODE_DROP,

    MODE_LAST
  };

  // CONSTRUCTORS:
  CameraDirector();
  virtual ~CameraDirector();

  int operator==(const CameraDirector &that) const { return this == &that; }

  static CameraDirector *GetInstance();

  // CAMERA:
  static int GetCameraMode();
  static void SetCameraMode(int m, double trans_time = 1);
  static std::string GetModeName();

  static double GetRangeLimit();
  static void SetRangeLimit(double r);
  static void SetRangeLimits(double min, double max);

  virtual void Reset();
  virtual void SetMode(int m, double trans_time = 1);
  virtual int GetMode() const { return requested_mode > MODE_NONE ? requested_mode : mode; }
  virtual Camera *GetCamera() { return &camera; }
  virtual Ship *GetShip() { return ship; }
  virtual void SetShip(Ship *s);

  virtual void VirtualHead(double az, double el);
  virtual void VirtualHeadOffset(double x, double y, double z);
  virtual void VirtualAzimuth(double delta);
  virtual void VirtualElevation(double delta);
  virtual void ExternalAzimuth(double delta);
  virtual void ExternalElevation(double delta);
  virtual void ExternalRange(double delta);
  virtual void SetOrbitPoint(double az, double el, double range);
  virtual void SetOrbitRates(double az_rate, double el_rate, double r_rate);

  static bool IsViewCentered();

  virtual void CycleViewObject();

  virtual Orbital *GetViewOrbital() const { return external_body; }
  virtual Ship *GetViewObject() const { return external_ship; }

  virtual void SetViewOrbital(Orbital *orb);
  virtual void SetViewObject(Ship *obj, bool quick = false);
  virtual void SetViewObjectGroup(ListIter<Ship> group, bool quick = false);
  virtual void ClearGroup();

  // BEHAVIORS:
  virtual void ExecFrame(double s);
  virtual void Transition(double s);

  // SimObserver:
  virtual bool Update(SimObject *obj);
  std::string GetObserverName() const override { return "CameraDirector"; }

protected:
  virtual void Cockpit(double s);
  virtual void Chase(double s);
  virtual void Target(double s);
  virtual void Threat(double s);
  virtual void Virtual(double s);
  virtual void Orbit(double s);
  virtual void Docking(double s);
  virtual void Drop(double s);

  int mode;
  int requested_mode;
  int old_mode;
  Camera camera;

  Ship *ship = {};

  SimRegion *region = {};
  Orbital *external_body = {};
  Ship *external_ship = {};
  List<Ship> external_group;
  Point external_point;

  Point base_loc;

  double virt_az;
  double virt_el;
  double virt_x;
  double virt_y;
  double virt_z;
  double azimuth;
  double elevation;
  double az_rate;
  double el_rate;
  double range_rate;
  double range;
  double range_min;
  double range_max;
  double base_range;
  double transition;

  Sim *sim = {};
  HUDView *hud;

  static CameraDirector *instance;
};
