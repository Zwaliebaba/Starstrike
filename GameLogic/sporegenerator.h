
#ifndef _included_sporegenerator_h
#define _included_sporegenerator_h

#include "entity.h"

#define SPOREGENERATOR_NUMTAILS 4


class SporeGenerator : public Entity
{
public:
    float       m_retargetTimer;
    float       m_eggTimer;
    LegacyVector3     m_targetPos;
    int         m_spiritId;

protected:
    bool    SearchForRandomPos      ();
    bool    SearchForSpirits        ();

    bool    AdvanceToTargetPosition ();
    bool    AdvanceIdle             ();
    bool    AdvanceEggLaying        ();
    bool    AdvancePanic            ();

    void    RenderTail( LegacyVector3 const &_from, LegacyVector3 const &_to, float _size );

protected:
    ShapeMarker     *m_eggMarker;
    ShapeMarker     *m_tail[SPOREGENERATOR_NUMTAILS];

    enum
    {
        StateIdle,
        StateEggLaying,
        StatePanic
    };
    int m_state;

public:
    SporeGenerator();

    void Begin          ();
    bool Advance        ( Unit *_unit );
    void ChangeHealth   ( int _amount );

    bool IsInView           ();
    void Render             ( float _predictionTime );
    bool RenderPixelEffect  ( float _predictionTime );

    void ListSoundEvents    ( LList<char *> *_list );
};



#endif
