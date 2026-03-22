
#pragma once

#include "building.h"


class SafeArea : public Building
{
public:
    float       m_size;
    int         m_entitiesRequired;
    int         m_entityTypeRequired;

    float       m_recountTimer;
    int         m_entitiesCounted;

public:
    SafeArea();

    void Initialise ( Building *_template );
    bool Advance    ();

    bool DoesSphereHit          (LegacyVector3 const &_pos, float _radius);
    bool DoesShapeHit           (ShapeStatic *_shape, Matrix34 _transform);
    bool DoesRayHit             (LegacyVector3 const &_rayStart, LegacyVector3 const &_rayDir,
                                 float _rayLen=1e10, LegacyVector3 *_pos=NULL, LegacyVector3 *_norm=NULL);

    const char* GetObjectiveCounter();

    void Read       ( TextReader *_in, bool _dynamic );
    void Write      ( FileWriter *_out );
};

