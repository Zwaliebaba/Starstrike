#include "pch.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "math_utils.h"
#include "matrix34.h"
#include "LegacyVector3.h"
#include "debug_render.h"

#include "app.h" // DELETEME
#include "camera.h" // DELETEME

#include "3d_sierpinski_gasket.h"


Sierpinski3D::Sierpinski3D(unsigned int _numPoints)
:	m_numPoints(_numPoints)
{
	m_points = new LegacyVector3[_numPoints];

	float size = 20;
	float x1=    0, y1=    0, z1= size;
	float x2= size, y2= size, z2=-size;
	float x3= size, y3=-size, z3=-size;
	float x4=-size, y4= size, z4=-size;
	float x5=-size, y5=-size, z5=-size;
	float x = x1, y = y1, z = z1;

	for(unsigned int i = 0; i < m_numPoints; ++i)
	{
		switch( darwiniaRandom()%5 )
		{
			case 0:
				x = ( x+x1 ) / 2;
				y = ( y+y1 ) / 2;
				z = ( z+z1 ) / 2;
				break;
			case 1:
				x = ( x+x2 ) / 2;
				y = ( y+y2 ) / 2;
				z = ( z+z2 ) / 2;
				break;
			case 2:
				x = ( x+x3 ) / 2;
				y = ( y+y3 ) / 2;
				z = ( z+z3 ) / 2;
				break;
			case 3:
				x = ( x+x4 ) / 2;
				y = ( y+y4 ) / 2;
				z = ( z+z4 ) / 2;
				break;
			case 4:
				x = ( x+x5 ) / 2;
				y = ( y+y5 ) / 2;
				z = ( z+z5 ) / 2;
				break;
		}
		m_points[i].x = x;
		m_points[i].y = y;
		m_points[i].z = z;
	}
}


Sierpinski3D::~Sierpinski3D()
{
	delete [] m_points;
	m_numPoints = 0;
}


void Sierpinski3D::Render(float scale)
{
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
	//glDepthMask(false);
	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);
    g_imRenderer->UnbindTexture();
    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
	g_imRenderer->Scalef(scale, scale, scale);

	float alpha = 128.0f * scale;
	g_imRenderer->Color4ub(alpha*0.4f, alpha*0.7f, alpha, 128);

	g_imRenderer->Begin(PRIM_POINTS);
		for (unsigned int i = 0; i < m_numPoints; ++i)
		{
			g_imRenderer->Vertex3fv(m_points[i].GetData());
		}
	g_imRenderer->End();

		for (unsigned int i = 0; i < m_numPoints; ++i)
		{
		}

	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
}
