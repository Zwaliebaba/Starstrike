
#ifndef _included_safearea_h
#define _included_safearea_h

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
    void Render     ( float predictionTime );

    bool DoesSphereHit          (LegacyVector3 const &_pos, float _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (LegacyVector3 const &_rayStart, LegacyVector3 const &_rayDir,
                                 float _rayLen=1e10, LegacyVector3 *_pos=NULL, LegacyVector3 *_norm=NULL);

    char *GetObjectiveCounter();

    void Read       ( TextReader *_in, bool _dynamic );
    void Write      ( FileWriter *_out );
};


#endif
