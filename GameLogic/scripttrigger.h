
#pragma once

#include "building.h"

#define SCRIPTRIGGER_RUNALWAYS              999
#define SCRIPTRIGGER_RUNNEVER               998
#define SCRIPTRIGGER_RUNCAMENTER            997
#define SCRIPTRIGGER_RUNCAMVIEW             996


class ScriptTrigger : public Building
{
public:
    char    m_scriptFilename[256];
    float   m_range;
    int     m_entityType;
    int     m_linkId;

protected:
    int     m_triggered;
    float   m_timer;

public:
    ScriptTrigger();

    void Initialise     ( Building *_template );
    bool Advance        ();

    void Trigger();

    bool DoesSphereHit          (LegacyVector3 const &_pos, float _radius);
    bool DoesShapeHit           (ShapeStatic *_shape, Matrix34 _transform);
    bool DoesRayHit             (LegacyVector3 const &_rayStart, LegacyVector3 const &_rayDir,
                                 float _rayLen=1e10, LegacyVector3 *_pos=NULL, LegacyVector3 *_norm=NULL);

    int  GetBuildingLink();
    void SetBuildingLink( int _buildingId );

    void Read       ( TextReader *_in, bool _dynamic );
    void Write      ( FileWriter *_out );
};

