
#pragma once

#include "building.h"

#define TRUNKPORT_HEIGHTMAP_MAXSIZE 16


class TrunkPort : public Building
{
public:
    int m_targetLocationId;

    ShapeMarkerData *m_destination1;
    ShapeMarkerData *m_destination2;

    int     m_heightMapSize;
    LegacyVector3 *m_heightMap;
    float   m_openTimer;

public:
    TrunkPort();

    void Initialise         ( Building *_template );
    void SetDetail          ( int _detail );
    bool Advance            ();
    void Render             ( float predictionTime );
    void RenderAlphas       ( float predictionTime );

    bool PerformDepthSort   ( LegacyVector3 &_centerPos );

    void ReprogramComplete();

    void ListSoundEvents    ( LList<const char*> *_list );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );
};

