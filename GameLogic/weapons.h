#pragma once

#include "entity.h"
#include "worldobject.h"

class Shape;

// ****************************************************************************
//  Class ThrowableWeapon
// ****************************************************************************

class ThrowableWeapon : public WorldObject
{
  protected:
    Shape* m_shape;
    float m_birthTime;
    float m_force;

    LegacyVector3 m_front;
    LegacyVector3 m_up;

    int m_numFlashes;

    RGBAColour m_colour;

    void TriggerSoundEvent(const char* _event);

  public:
    ThrowableWeapon(int _type, const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);

    void Initialise();
    bool Advance() override;
    void Render(float _predictionTime) override;

    static float GetMaxForce(int _researchLevel);
    static float GetApproxMaxRange(float _maxForce);
};

// ****************************************************************************
//  Class Grenade
// ****************************************************************************

class Grenade : public ThrowableWeapon
{
  public:
    float m_life;
    float m_power;

    Grenade(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);
    bool Advance() override;
};

// ****************************************************************************
//  Class AirStrikeMarker
// ****************************************************************************

class AirStrikeMarker : public ThrowableWeapon
{
  public:
    WorldObjectId m_airstrikeUnit;

    AirStrikeMarker(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);
    bool Advance() override;
};

// ****************************************************************************
//  Class ControllerGrenade
// ****************************************************************************

class ControllerGrenade : public ThrowableWeapon
{
  public:
    ControllerGrenade(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);
    bool Advance() override;
};

// ****************************************************************************
//  Class Rocket
// ****************************************************************************

class Rocket : public WorldObject
{
  public:
    unsigned char m_fromTeamId;

    Shape* m_shape;
    float m_timer;

    LegacyVector3 m_target;

    Rocket() {}
    Rocket(LegacyVector3 _startPos, LegacyVector3 _targetPos);

    void Initialise();
    bool Advance() override;
    void Render(float predictionTime) override;
};

// ****************************************************************************
//  Class Laser
// ****************************************************************************

class Laser : public WorldObject
{
  public:
    unsigned char m_fromTeamId;

  protected:
    float m_life;
    bool m_harmless; // becomes true after hitting someone
    bool m_bounced;

  public:
    Laser() {}

    void Initialise(float _lifeTime);
    bool Advance() override;
    void Render(float predictionTime) override;
};

// ****************************************************************************
//  Class TurretShell
// ****************************************************************************

class TurretShell : public WorldObject
{
  protected:
    float m_life;

  public:
    TurretShell(float _life);

    bool Advance() override;
    void Render(float predictionTime) override;
};

// ****************************************************************************
//  Class Shockwave
// ****************************************************************************

class Shockwave : public WorldObject
{
  public:
    Shape* m_shape;
    int m_teamId;
    float m_size;
    float m_life;

    Shockwave(int _teamId, float _size);

    bool Advance() override;
    void Render(float predictionTime) override;
};

// ****************************************************************************
//  Class MuzzleFlash
// ****************************************************************************

class MuzzleFlash : public WorldObject
{
  public:
    LegacyVector3 m_front;
    float m_size;
    float m_life;

    MuzzleFlash();
    MuzzleFlash(const LegacyVector3& _pos, const LegacyVector3& _front, float _size, float _life);

    bool Advance() override;
    void Render(float _predictionTime) override;
};

// ****************************************************************************
//  Class Missile
// ****************************************************************************

class Missile : public WorldObject
{
  protected:
    float m_life;
    LList<LegacyVector3> m_history;
    Shape* m_shape;
    ShapeMarker* m_booster;
    MuzzleFlash m_fire;

  public:
    WorldObjectId m_tankId; // Who fired me
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    LegacyVector3 m_target;

    Missile();

    bool Advance() override;
    bool AdvanceToTargetPosition(const LegacyVector3& _pos);
    void Explode();
    void Render(float _predictionTime) override;
};
