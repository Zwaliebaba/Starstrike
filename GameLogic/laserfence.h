#pragma once

#include <stdio.h>

#include "building.h"


class ShapeStatic;
class ShapeFragmentData;
class TextReader;

#define LASERFENCE_RAISESPEED       0.3f


class LaserFence : public Building
{
    friend class LaserFenceBuildingRenderer;
protected:
    float           m_status;                       // 0=down, 1=up
    int             m_nextLaserFenceId;
    float           m_sparkTimer;

    bool            m_radiusSet;

    ShapeMarkerData     *m_marker1;
    ShapeMarkerData     *m_marker2;

	bool			m_nextToggled;		// set to true when the fence has enabled/disabled the next fence in the line, to prevent constant enable calling

public:
    enum
    {
        ModeDisabled,
        ModeEnabling,
        ModeEnabled,
        ModeDisabling,
		ModeNeverOn
    };
    int             m_mode;
    float           m_scale;

public:
    LaserFence();

    void Initialise( Building *_template );
    void SetDetail ( int _detail );

    bool Advance        ();
    void Render         ( float predictionTime );
    void RenderAlphas   ( float predictionTime );
    void RenderLights   ();

    bool PerformDepthSort   ( LegacyVector3 &_centerPos );
    bool IsInView           ();

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );

    void Enable     ();
    void Disable    ();
    void Toggle     ();
	bool IsEnabled  ();

    void Spark      ();
    void Electrocute( LegacyVector3 const &_pos );

    int  GetBuildingLink();
    void SetBuildingLink( int _buildingId );

    float GetFenceFullHeight    ();

    bool DoesSphereHit          (LegacyVector3 const &_pos, float _radius);
    bool DoesRayHit             (LegacyVector3 const &_rayStart, LegacyVector3 const &_rayDir,
                                 float _rayLen=1e10, LegacyVector3 *_pos=NULL, LegacyVector3 *_norm=NULL);
    bool DoesShapeHit           (ShapeStatic *_shape, Matrix34 _transform);

    LegacyVector3 GetTopPosition();
};

