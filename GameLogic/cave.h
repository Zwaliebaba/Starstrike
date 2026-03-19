#pragma once

#include "building.h"

class ShapeMarkerData;


class Cave : public Building
{
protected:
    int             m_troopType;
    int             m_unitId;
    float           m_spawnTimer;
    bool            m_dead;

    ShapeMarkerData     *m_spawnPoint;

public:
    Cave();

    bool Advance();
    void Damage( float _damage );
};

