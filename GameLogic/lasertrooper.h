
#pragma once

#include "entity.h"


class LaserTrooper : public Entity
{
public:
    LegacyVector3 m_targetPos;
    LegacyVector3 m_unitTargetPos;

    float   m_victoryDance;

public:

    void Begin      ();

    bool Advance    ( Unit *_unit );

    void AdvanceVictoryDance();
};

