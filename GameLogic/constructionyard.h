
#pragma once

#include "building.h"


// ****************************************************************************
// Class ConstructionYard
// ****************************************************************************

#define YARD_NUMPRIMITIVES 9
#define YARD_NUMRUNGSPIKES 6

class ConstructionYard : public Building
{
    friend class ConstructionYardBuildingRenderer;

protected:
    ShapeStatic     *m_rung;
    ShapeStatic     *m_primitive;
    ShapeMarkerData *m_primitives[YARD_NUMPRIMITIVES];
    ShapeMarkerData *m_rungSpikes[YARD_NUMRUNGSPIKES];

    int     m_numPrimitives;
    int     m_numSurges;
    int     m_numTanksProduced;
    float   m_fractionPopulated;
    float   m_timer;

    float   m_alpha;

    bool    IsPopulationLocked();                       // Are there too many tanks already

public:
    ConstructionYard();

    bool Advance();

    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    Matrix34 GetRungMatrix1();
    Matrix34 GetRungMatrix2();

    bool AddPrimitive();
    void AddPowerSurge();
};



// ****************************************************************************
// Class DisplayScreen
// ****************************************************************************

#define DISPLAYSCREEN_NUMRAYS 3

class DisplayScreen : public Building
{
    friend class DisplayScreenBuildingRenderer;

protected:
    ShapeStatic       *m_armour;
    ShapeMarkerData *m_rays[DISPLAYSCREEN_NUMRAYS];

public:
    DisplayScreen();

    void RenderAlphas       ( float _predictionTime );

};

