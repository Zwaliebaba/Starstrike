#ifndef INCLUDED_LANDSCAPE_RENDERER_H
#define INCLUDED_LANDSCAPE_RENDERER_H


#include "2d_surface_map.h"
#include "fast_darray.h"
#include "rgb_colour.h"
#include "texture_uv.h"
#include "LegacyVector3.h"

#include <d3d11.h>

class BitmapRGBA;


class LandVertex
{
public:
	LegacyVector3		m_pos;
	LegacyVector3		m_norm;
	RGBAColour	m_col;
	TextureUV	m_uv;
};


//*****************************************************************************
// Class LandTriangleStrip
//*****************************************************************************

class LandTriangleStrip
{
public:
	int				m_firstVertIndex;
	int				m_numVerts;

	LandTriangleStrip(): m_firstVertIndex(-1), m_numVerts(-2) {}
};


//*****************************************************************************
// Class LandscapeRenderer
//*****************************************************************************

class LandscapeRenderer
{
protected:
	BitmapRGBA      *m_landscapeColour;
	float			m_highest;

	FastDArray		<LandVertex> m_verts;

	ID3D11Buffer*	m_d3dVertexBuffer;
	int				m_d3dVertexCount;

	FastDArray		<LandTriangleStrip *> m_strips;

	void BuildVertArrayAndTriStrip(SurfaceMap2D <float> *_heightMap);
	void BuildNormArray();
	void BuildUVArray(SurfaceMap2D <float> *_heightMap);
	void GetLandscapeColour(float _height, float _gradient,
							unsigned int _x, unsigned int _y, RGBAColour *_colour);
	void BuildColourArray();
	void CreateD3DVertexBuffer();

public:
	int				m_numTriangles;

	LandscapeRenderer(SurfaceMap2D <float> *_heightMap);
	~LandscapeRenderer();

	void BuildRenderState(SurfaceMap2D <float> *_heightMap);

	void RenderMainSlow();
	void RenderOverlaySlow();
	void Render();
};


#endif
