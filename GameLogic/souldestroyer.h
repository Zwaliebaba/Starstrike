
#ifndef _included_souldestroyer_h
#define _included_souldestroyer_h

#include "entity.h"

#define SOULDESTROYER_MINSEARCHRANGE       200.0f
#define SOULDESTROYER_MAXSEARCHRANGE       300.0f
#define SOULDESTROYER_DAMAGERANGE          25.0f
#define SOULDESTROYER_MAXSPIRITS           50

class Shape;



class SoulDestroyer : public Entity
{
protected:
    LegacyVector3         m_targetPos;
    LegacyVector3         m_up;
    WorldObjectId   m_targetEntity;
    LList<LegacyVector3>  m_positionHistory;
    FastDArray      <float> m_spirits;

    float           m_retargetTimer;
    float           m_panic;

    static Shape        *s_shapeHead;
    static Shape        *s_shapeTail;
    static ShapeMarker  *s_tailMarker;

    LegacyVector3      m_spiritPosition[SOULDESTROYER_MAXSPIRITS];

protected:
    bool        SearchForRandomPosition();
    bool        SearchForTargetEnemy();
    bool        SearchForRetreatPosition();

    bool        AdvanceToTargetPosition();
    void        RecordHistoryPosition();
    bool        GetTrailPosition( LegacyVector3 &_pos, LegacyVector3 &_vel );

    void RenderShapes               ( float _predictionTime );
    void RenderShapesForPixelEffect ( float _predictionTime );
    void RenderSpirit               ( LegacyVector3 const &_pos, float _alpha );
    bool RenderPixelEffect          ( float _predictionTime );

    void Panic( float _time );

public:
    SoulDestroyer();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    void ChangeHealth       ( int _amount );
    void Render             ( float _predictionTime );

    void Attack             ( LegacyVector3 const &_pos );

    void ListSoundEvents    ( LList<char *> *_list );

	void SetWaypoint( LegacyVector3 const _waypoint );
};




class Zombie : public WorldObject
{
public:
    LegacyVector3     m_front;
    LegacyVector3     m_up;
    float       m_life;

    LegacyVector3     m_hover;
    float       m_positionOffset;                       // Used to make them float around a bit
    float       m_xaxisRate;
    float       m_yaxisRate;
    float       m_zaxisRate;

public:
    Zombie();

    bool Advance();
    void Render( float _predictionTime );
};

#endif
