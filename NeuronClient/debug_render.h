#ifndef _included_debug_render_h
#define _included_debug_render_h

#ifdef DEBUG_RENDER_ENABLED

#include "rgb_colour.h"
#include "LegacyVector3.h"

void RenderSquare2d(float x, float y, float size, RGBAColour const &_col=RGBAColour(255,255,255));

void RenderCube(LegacyVector3 const &_centre, float _sizeX, float _sizeY, float _sizeZ, RGBAColour const &_col=RGBAColour(255,255,255));
void RenderSphereRings(LegacyVector3 const &_centre, float _radius, RGBAColour const &_col=RGBAColour(255,255,255));
void RenderSphere(LegacyVector3 const &_centre, float _radius, RGBAColour const &_col=RGBAColour(255,255,255));

void RenderVerticalCylinder(LegacyVector3 const &_centreBase, LegacyVector3 const &_verticalAxis,
							float _height, float _radius,
							RGBAColour const &_col=RGBAColour(255,255,255));

void RenderArrow(LegacyVector3 const &start, LegacyVector3 const &end, float width, RGBAColour const &_col=RGBAColour(255,255,255));
void RenderPointMarker(LegacyVector3 const &point, char const *text, ...);

void PrintMatrix( const char *_name, GLenum _whichMatrix );
void PrintMatrices( const char *_title );

#endif // DEBUG_RENDER_ENABLED

#endif
