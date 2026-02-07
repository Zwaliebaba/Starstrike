#pragma once

#include "Geometry.h"
#include "SimObject.h"

class Solid;
class Model;
class Shot;

class Debris : public SimObject
{
  public:
    Debris(Model* model, const Vec3& pos, const Vec3& vel, double mass);

    void SetLife(int seconds) { life = seconds; }
    virtual int HitBy(Shot* shot, Point& impact);

    void ExecFrame(double seconds) override;
    virtual double AltitudeAGL() const;
};
