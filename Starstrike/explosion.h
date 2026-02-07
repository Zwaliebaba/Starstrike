#ifndef Explosion_h
#define Explosion_h

#include "Geometry.h"
#include "SimObject.h"

class Solid;
class Particles;
class System;

class Explosion : public SimObject, public SimObserver
{
  public:
    
    enum Type
    {
      SHIELD_FLASH    = 1,
      HULL_FLASH      = 2,
      BEAM_FLASH      = 3,
      SHOT_BLAST      = 4,
      HULL_BURST      = 5,
      HULL_FIRE       = 6,
      PLASMA_LEAK     = 7,
      SMOKE_TRAIL     = 8,
      SMALL_FIRE      = 9,
      SMALL_EXPLOSION = 10,
      LARGE_EXPLOSION = 11,
      LARGE_BURST     = 12,
      NUKE_EXPLOSION  = 13,
      QUANTUM_FLASH   = 14,
      HYPER_FLASH     = 15
    };

    Explosion(int type, const Vec3& pos, const Vec3& vel, float exp_scale, float part_scale, SimRegion* rgn = nullptr,
              SimObject* source = nullptr);
    ~Explosion() override;

    static void Initialize();
    static void Close();

    void ExecFrame(double seconds) override;
    Particles* GetParticles() { return particles; }

    void Activate(Scene& scene) override;
    void Deactivate(Scene& scene) override;

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

  protected:
    int type;
    Particles* particles;

    float scale;
    float scale1;
    float scale2;

    SimObject* source;
    Point mount_rel;
};

#endif Explosion_h
