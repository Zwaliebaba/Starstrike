#pragma once

#include "Debris.h"
#include "Geometry.h"

class Asteroid : public Debris
{
  public:
    Asteroid(int type, const Vec3& pos, double mass);

    static void Initialize();
    static void Close();
};
