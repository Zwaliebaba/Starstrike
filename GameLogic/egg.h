
#pragma once

#include "entity.h"

#define EGG_DORMANTLIFE     120                             // total time alive while unfertilised


class Egg : public Entity
{
public:

    enum
    {
        StateDormant,
        StateFertilised,
        StateOpen
    };

    int     m_state;
    int     m_spiritId;
    float   m_timer;

public:
    Egg();

    void ChangeHealth   ( int amount );

    bool Advance        ( Unit *_unit );
    void Fertilise      ( int spiritId );
};

