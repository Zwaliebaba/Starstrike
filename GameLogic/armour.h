
#pragma once

#include "entity.h"
#include "flag.h"

class ShapeMarkerData;
class ShapeStatic;

#define ARMOUR_UNLOADPERIOD   0.1f


class Armour: public Entity
{
protected:
    ShapeMarkerData     *m_markerEntrance;
    ShapeMarkerData     *m_markerFlag;
    LegacyVector3         m_up;
    LegacyVector3         m_conversionPoint;
    float           m_speed;

    enum
    {
        StateIdle,
        StateUnloading,
        StateLoading,
    };
    int             m_numPassengers;
    float           m_previousUnloadTimer;
    float           m_newOrdersTimer;

    Flag            m_flag;
    Flag            m_deployFlag;

public:
    LegacyVector3         m_wayPoint;
    int             m_state;

public:
    Armour();
    ~Armour();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    void Render             ( float _predictionTime );

    void ChangeHealth       ( int _amount );

    void SetOrders          ( LegacyVector3 const &_orders );
    void SetWayPoint        ( LegacyVector3 const &_wayPoint );
    void SetConversionPoint ( LegacyVector3 const &_conversionPoint );
    void AdvanceToTargetPos ();
    void DetectCollisions   ();
    void ConvertToGunTurret ();

    void SetDirectOrders    (); // orders while in direct control mode - removes gun turret mode

    bool IsLoading          ();
    bool IsUnloading        ();
    void ToggleLoading      ();
    void AddPassenger       ();
    void RemovePassenger    ();

    int Capacity();

    void ListSoundEvents    ( LList<const char*> *_list );

    void GetEntrance( LegacyVector3 &_exitPos, LegacyVector3 &_exitDir );
};

