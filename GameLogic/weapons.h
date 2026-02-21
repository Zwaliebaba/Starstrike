#ifndef _included_weapons_h
#define _included_weapons_h


#include "entity.h"
#include "worldobject.h"

class Shape;


// ****************************************************************************
//  Class ThrowableWeapon
// ****************************************************************************

class ThrowableWeapon : public WorldObject
{
protected:
    Shape   *m_shape;
    float   m_birthTime;
    float   m_force;

    LegacyVector3 m_front;
    LegacyVector3 m_up;

    int     m_numFlashes;

    RGBAColour m_colour;

    void TriggerSoundEvent( char *_event );

public:
    ThrowableWeapon( int _type, LegacyVector3 const &_startPos, LegacyVector3 const &_front, float _force );

    void Initialise ();
    bool Advance    ();
    void Render     ( float _predictionTime );

	static float GetMaxForce( int _researchLevel );
	static float GetApproxMaxRange( float _maxForce );
};


// ****************************************************************************
//  Class Grenade
// ****************************************************************************

class Grenade : public ThrowableWeapon
{
public:
    float   m_life;
    float   m_power;

public:
    Grenade( LegacyVector3 const &_startPos, LegacyVector3 const &_front, float _force );
    bool Advance    ();
};


// ****************************************************************************
//  Class AirStrikeMarker
// ****************************************************************************

class AirStrikeMarker : public ThrowableWeapon
{
public:
    WorldObjectId    m_airstrikeUnit;

public:
    AirStrikeMarker( LegacyVector3 const &_startPos, LegacyVector3 const &_front, float _force );
    bool Advance    ();
};


// ****************************************************************************
//  Class ControllerGrenade
// ****************************************************************************

class ControllerGrenade : public ThrowableWeapon
{
public:
    ControllerGrenade( LegacyVector3 const &_startPos, LegacyVector3 const &_front, float _force );
    bool Advance    ();
};


// ****************************************************************************
//  Class Rocket
// ****************************************************************************

class Rocket : public WorldObject
{
public:
    unsigned char m_fromTeamId;

    Shape   *m_shape;
    float    m_timer;

public:
    LegacyVector3 m_target;

    Rocket() {}
    Rocket(LegacyVector3 _startPos, LegacyVector3 _targetPos);

    void Initialise ();
    bool Advance    ();
    void Render     ( float predictionTime );
};



// ****************************************************************************
//  Class Laser
// ****************************************************************************

class Laser : public WorldObject
{
public:
    unsigned char m_fromTeamId;

protected:
    float   m_life;
    bool    m_harmless;                 // becomes true after hitting someone
    bool    m_bounced;

public:
    Laser() {}

    void Initialise(float _lifeTime);
    bool Advance();
    void Render( float predictionTime );
};



// ****************************************************************************
//  Class TurretShell
// ****************************************************************************

class TurretShell : public WorldObject
{
protected:
    float   m_life;

public:
    TurretShell(float _life);

    bool Advance();
    void Render( float predictionTime );
};


// ****************************************************************************
//  Class Shockwave
// ****************************************************************************

class Shockwave : public WorldObject
{
public:
    Shape   *m_shape;
    int     m_teamId;
    float   m_size;
    float   m_life;

public:
    Shockwave( int _teamId, float _size );

    bool Advance();
    void Render( float predictionTime );
};



// ****************************************************************************
//  Class MuzzleFlash
// ****************************************************************************

class MuzzleFlash : public WorldObject
{
public:
    LegacyVector3 m_front;
    float   m_size;
    float   m_life;

public:
    MuzzleFlash();
    MuzzleFlash( LegacyVector3 const &_pos, LegacyVector3 const &_front, float _size, float _life );

    bool Advance();
    void Render( float _predictionTime );
};



// ****************************************************************************
//  Class Missile
// ****************************************************************************

class Missile : public WorldObject
{
protected:
    float           m_life;
    LList           <LegacyVector3> m_history;
    Shape           *m_shape;
    ShapeMarker     *m_booster;
    MuzzleFlash     m_fire;

public:
    WorldObjectId   m_tankId;                   // Who fired me
    LegacyVector3         m_front;
    LegacyVector3         m_up;
    LegacyVector3         m_target;

public:
    Missile();

    bool Advance();
    bool AdvanceToTargetPosition( LegacyVector3 const &_pos );
    void Explode();
    void Render( float _predictionTime );
};

#endif
