#ifndef _included_unit_h
#define _included_unit_h

#include "slice_darray.h"
#include "LegacyVector3.h"

#include "entity.h"


class Unit
{
protected:
    LegacyVector3             m_wayPoint;

public:
	int					m_routeId;
	int					m_routeWayPointId;
    int                 m_teamId;
    int                 m_unitId;
    int                 m_troopType;
    SliceDArray         <Entity *> m_entities;

    LegacyVector3             m_centrePos;
    LegacyVector3             m_vel;
    float               m_radius;

    LegacyVector3             m_targetDir;

    float               m_attackAccumulator;                // Used to regulate fire rate

    enum
    {
        FormationRectangle,
        FormationAirstrike,
        NumFormations
    };

	LegacyVector3 GetWayPoint			();
	virtual void SetWayPoint	(LegacyVector3 const &_pos);
    LegacyVector3 GetFormationOffset  (int _formation, int _index);
    LegacyVector3 GetOffset           (int _formation, int _index);  // Takes into account formation AND obstructions

protected:
    LegacyVector3     m_accumulatedCentre;
    float       m_accumulatedRadiusSquared;
    int         m_numAccumulated;

public:
    Unit    (int troopType, int teamId, int unitId, int numEntities, LegacyVector3 const &_pos);
    virtual ~Unit();

    virtual void    Begin           ();
    virtual bool    Advance         ( int _slice );
    virtual void    Attack          ( LegacyVector3 pos, bool withGrenade );
    virtual void    AdvanceEntities ( int _slice );
    virtual void    Render          ( float _predictionTime );

    virtual bool    IsInView        ();

    Entity  *NewEntity              ( int *_index );
    int     AddEntity               ( Entity *_entity );
    void    RemoveEntity            ( int _index, float _posX, float _posZ );
    int     NumEntities             ();
    int     NumAliveEntities        ();                                 // Does not count entities still in the unit, but their m_dead=true
    void    UpdateEntityPosition    ( LegacyVector3 pos, float _radius );
    void    RecalculateOffsets      ();

	void	FollowRoute();

    virtual void DirectControl( TeamControls const& _teamControls );

	Entity *RayHit(LegacyVector3 const &_rayStart, LegacyVector3 const &_rayDir);
};


#endif
