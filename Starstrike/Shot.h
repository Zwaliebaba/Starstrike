#pragma once

#include "Color.h"
#include "Geometry.h"
#include "SimObject.h"

class Camera;
class Ship;
class Trail;
class System;
class WeaponDesign;
class Sprite;
class Sound;

class Shot : public SimObject, public SimObserver
{
  public:
    
    Shot(const Point& pos, const Camera& cam, WeaponDesign* design, const Ship* ship = nullptr);
    ~Shot() override;

    virtual void SeekTarget(SimObject* target, System* sub = nullptr);
    void ExecFrame(double factor) override;
    static void Initialize();
    static void Close();

    void Activate(Scene& scene) override;
    void Deactivate(Scene& scene) override;

    const Ship* Owner() const { return owner; }
    double Damage() const;
    int ShotType() const { return type; }
    virtual SimObject* GetTarget() const;

    virtual bool IsPrimary() const { return primary; }
    virtual bool IsDrone() const { return false; }
    virtual bool IsDecoy() const { return false; }
    virtual bool IsProbe() const { return false; }
    virtual bool IsMissile() const { return !primary; }
    virtual bool IsArmed() const { return armed; }
    virtual bool IsBeam() const { return beam; }
    virtual bool IsFlak() const;
    bool IsHostileTo(const SimObject* o) const override;

    bool HitTarget() const { return hit_target; }
    void SetHitTarget(bool h) { hit_target = h; }

    virtual bool IsTracking(Ship* tgt) const;
    virtual double PCS() const { return 0; }
    virtual double ACS() const { return 0; }
    virtual int GetIFF() const;
    virtual Color MarkerColor() const;

    const Point& Origin() const { return origin; }
    float Charge() const { return charge; }
    void SetCharge(float c);
    double Length() const;
    Graphic* GetTrail() const { return (Graphic*)trail; }
    void SetFuse(double seconds);

    void SetBeamPoints(const Point& from, const Point& to);
    virtual void Disarm();
    virtual void Destroy();

    const WeaponDesign* Design() const { return design; }
    std::string DesignName() const;
    int GetEta() const { return eta; }
    void SetEta(int t) { eta = t; }

    double AltitudeMSL() const;
    double AltitudeAGL() const;

    // SimObserver interface:
    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

    int operator ==(const Shot& s) const { return id == s.id; }

  protected:
    const Ship* owner;

    int type;
    float base_damage;
    float charge;
    float offset;
    float altitude_agl;
    short eta;
    BYTE iff_code;
    bool first_frame;
    bool primary;
    bool beam;
    bool armed;
    bool hit_target;

    Sprite* flash;   // muzzle flash
    Sprite* flare;   // drive flare
    Trail* trail;   // exhaust trail

    Sound* sound;
    WeaponDesign* design;

    // for beam weapons:
    Point origin;
};
