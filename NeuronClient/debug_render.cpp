#include "pch.h"

#include <math.h>
#include <stdarg.h>

#include "math_utils.h"
#include "sphere_renderer.h"
#include "text_renderer.h"
#include "debug_utils.h"

#include "app.h"
#include "camera.h"
#include "debug_render.h"
#include "renderer.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"


#ifdef DEBUG_RENDER_ENABLED


void RenderSquare2d(float x, float y, float size, RGBAColour const &_col)
{
	int v[4];
	glGetIntegerv(GL_VIEWPORT, &v[0]);
	float left = v[0];
	float right = v[0] + v[2];
	float bottom = v[1] + v[3];
	float top = v[1];

	// D3D11 path
	auto savedProj = g_imRenderer->GetProjectionMatrix();
	auto savedView = g_imRenderer->GetViewMatrix();
	auto savedWorld = g_imRenderer->GetWorldMatrix();
	g_imRenderer->SetProjectionMatrix(DirectX::XMMatrixOrthographicOffCenterRH(left, right, bottom, top, -1.0f, 1.0f));
	g_imRenderer->SetViewMatrix(DirectX::XMMatrixIdentity());
	g_imRenderer->SetWorldMatrix(DirectX::XMMatrixIdentity());

	g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);
	if (_col.a != 255)
		g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

	g_imRenderer->Color4ubv(_col.GetData());
	g_imRenderer->Begin(PRIM_QUADS);
		g_imRenderer->Vertex2f(x - size, y - size);
		g_imRenderer->Vertex2f(x + size, y - size);
		g_imRenderer->Vertex2f(x + size, y + size);
		g_imRenderer->Vertex2f(x - size, y + size);
	g_imRenderer->End();

	g_imRenderer->SetProjectionMatrix(savedProj);
	g_imRenderer->SetViewMatrix(savedView);
	g_imRenderer->SetWorldMatrix(savedWorld);

	// OpenGL path
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(left, right, bottom, top);
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	if (_col.a != 255)
		glEnable(GL_BLEND);

	glColor4ubv(_col.GetData());
	glBegin(GL_QUADS);
		glVertex2f(x - size, y - size);
		glVertex2f(x + size, y - size);
		glVertex2f(x + size, y + size);
		glVertex2f(x - size, y + size);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}


void RenderCube(LegacyVector3 const &_centre, float _sizeX, float _sizeY, float _sizeZ, RGBAColour const &_col)
{
	float halfX = _sizeX * 0.5f;
	float halfY = _sizeY * 0.5f;
	float halfZ = _sizeZ * 0.5f;

	if (_col.a != 255)
		g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

	g_imRenderer->Color4ubv(_col.GetData());
	g_imRenderer->Begin(PRIM_LINE_LOOP);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z - halfZ);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z - halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z - halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z - halfZ);
	g_imRenderer->End();
	g_imRenderer->Begin(PRIM_LINE_LOOP);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z + halfZ);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z + halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z + halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z + halfZ);
	g_imRenderer->End();
	g_imRenderer->Begin(PRIM_LINES);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z - halfZ);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z + halfZ);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z - halfZ);
		g_imRenderer->Vertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z + halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z - halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z + halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z - halfZ);
		g_imRenderer->Vertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z + halfZ);
	g_imRenderer->End();

	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);

	// OpenGL path
	if (_col.a != 255)
		glEnable(GL_BLEND);
	glColor4ubv(_col.GetData());
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z - halfZ);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z - halfZ);
	glEnd();
	glBegin(GL_LINE_LOOP);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z + halfZ);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z + halfZ);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z + halfZ);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z + halfZ);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z - halfZ);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z + halfZ);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z + halfZ);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z + halfZ);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z + halfZ);
	glEnd();
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}


void RenderSphereRings(LegacyVector3 const &_centre, float _radius, RGBAColour const &_col)
{
	g_imRenderer->Color3ubv(_col.GetData());
	glColor3ubv(_col.GetData());

	int const _segs = 20;
	float thetaIncrement = M_PI * 2.0f / (float)_segs;

	static float cx[_segs] = {-1};
	static float sx[_segs];

	int i;

	if (cx[0] == -1)
	{
		for (i = 0; i < _segs; i++)
		{
			float theta = (float)i * thetaIncrement;
			sx[i] = sinf(theta);
			cx[i] = cosf(theta);
		}
	}

	glLineWidth(2.0f);
	g_imRenderer->Begin(PRIM_LINE_LOOP);
	for (i = 0; i < _segs; i++)
	{
		LegacyVector3 pos = _centre;
		pos.x += sx[i] * _radius;
		pos.z += cx[i] * _radius;
		g_imRenderer->Vertex3fv(pos.GetData());
	}
	g_imRenderer->End();
	g_imRenderer->Begin(PRIM_LINE_LOOP);
	for (i = 0; i < _segs; i++)
	{
		LegacyVector3 pos = _centre;
		pos.y += sx[i] * _radius;
		pos.z += cx[i] * _radius;
		g_imRenderer->Vertex3fv(pos.GetData());
	}
	g_imRenderer->End();
	g_imRenderer->Begin(PRIM_LINE_LOOP);
	for (i = 0; i < _segs; i++)
	{
		LegacyVector3 pos = _centre;
		pos.x += sx[i] * _radius;
		pos.y += cx[i] * _radius;
		g_imRenderer->Vertex3fv(pos.GetData());
	}
	g_imRenderer->End();

	glBegin(GL_LINE_LOOP);
		for (i = 0; i < _segs; i++)
		{
			LegacyVector3 pos = _centre;
			pos.x += sx[i] * _radius;
			pos.z += cx[i] * _radius;
			glVertex3fv(pos.GetData());
		}
	glEnd();
	glBegin(GL_LINE_LOOP);
		for (i = 0; i < _segs; i++)
		{
			LegacyVector3 pos = _centre;
			pos.y += sx[i] * _radius;
			pos.z += cx[i] * _radius;
			glVertex3fv(pos.GetData());
		}
	glEnd();
	glBegin(GL_LINE_LOOP);
		for (i = 0; i < _segs; i++)
		{
			LegacyVector3 pos = _centre;
			pos.x += sx[i] * _radius;
			pos.y += cx[i] * _radius;
			glVertex3fv(pos.GetData());
		}
	glEnd();
}


void RenderSphere(LegacyVector3 const &_centre, float _radius, RGBAColour const &_col)
{
	static Sphere aSphere;

	g_imRenderer->Color3ubv(_col.GetData());
	glColor3ubv(_col.GetData());
	aSphere.Render(_centre, _radius);
}


void RenderVerticalCylinder(LegacyVector3 const &_centreBase, LegacyVector3 const &_verticalAxis,
							float _height, float _radius, RGBAColour const &_col)
{
	LegacyVector3 axis1 = _verticalAxis;
	axis1.Normalise();
	LegacyVector3 axis2;
	LegacyVector3 axis3;

	if (axis1.x > 0.5f)			axis2.Set(0, 1, 0);
	else						axis2.Set(1, 0, 0);

	axis3 = axis1 ^ axis2;
	axis2 = axis1 ^ axis3;

	g_imRenderer->Color3ubv(_col.GetData());
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(1.0f);
	glColor3ubv(_col.GetData());

	int const numEdges = 16;

	// D3D11 path
	g_imRenderer->Begin(PRIM_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			LegacyVector3 pos = _centreBase;
			float theta = M_PI * 2.0f * (float)i / (float)numEdges;
			pos += axis2 * sinf(theta) * _radius;
			pos += axis3 * cosf(theta) * _radius;
			g_imRenderer->Vertex3fv(pos.GetData());
		}
	g_imRenderer->End();
	g_imRenderer->Begin(PRIM_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			LegacyVector3 pos = _centreBase + axis1 * _height;
			float theta = M_PI * 2.0f * (float)i / (float)numEdges;
			pos += axis2 * sinf(theta) * _radius;
			pos += axis3 * cosf(theta) * _radius;
			g_imRenderer->Vertex3fv(pos.GetData());
		}
	g_imRenderer->End();
	g_imRenderer->Begin(PRIM_LINES);
		for (int i = 0; i < numEdges; ++i)
		{
			LegacyVector3 pos = _centreBase;
			float theta = M_PI * 2.0f * (float)i / (float)numEdges;
			pos += axis2 * sinf(theta) * _radius;
			pos += axis3 * cosf(theta) * _radius;
			g_imRenderer->Vertex3fv(pos.GetData());
			pos += axis1 * _height;
			g_imRenderer->Vertex3fv(pos.GetData());
		}
	g_imRenderer->End();

	// OpenGL path
	glBegin(GL_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			LegacyVector3 pos = _centreBase;
			float theta = M_PI * 2.0f * (float)i / (float)numEdges;
			pos += axis2 * sinf(theta) * _radius;
			pos += axis3 * cosf(theta) * _radius;
			glVertex3fv(pos.GetData());
		}
	glEnd();
	glBegin(GL_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			LegacyVector3 pos = _centreBase + axis1 * _height;
			float theta = M_PI * 2.0f * (float)i / (float)numEdges;
			pos += axis2 * sinf(theta) * _radius;
			pos += axis3 * cosf(theta) * _radius;
			glVertex3fv(pos.GetData());
		}
	glEnd();
	glBegin(GL_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			LegacyVector3 pos = _centreBase;
			float theta = M_PI * 2.0f * (float)i / (float)numEdges;
			pos += axis2 * sinf(theta) * _radius;
			pos += axis3 * cosf(theta) * _radius;
			glVertex3fv(pos.GetData());
			pos += axis1 * _height;
			glVertex3fv(pos.GetData());
		}
	glEnd();

	glEnable(GL_LIGHTING);
}


void RenderArrow(LegacyVector3 const &start, LegacyVector3 const &end, float width, RGBAColour const &_col/* =RGBAColour */)
{
	Camera *cam = g_app->m_camera;
	LegacyVector3 midPoint = (start + end) * 0.5f;
	LegacyVector3 midPointToCamera = cam->GetPos() - midPoint;
	float midPointToCameraDist = midPointToCamera.Mag();
	LegacyVector3 invDir = start - end;
	LegacyVector3 sidewaysDir = ((cam->GetPos() - end) ^ invDir).Normalise();
	float arrowLen = invDir.Mag();
	invDir.SetLength(arrowLen * 0.2f);

	if (_col.a != 255)
		g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

	g_imRenderer->Color3ubv(_col.GetData());
	LegacyVector3 p1 = end + invDir + sidewaysDir * arrowLen * 0.1f;
	LegacyVector3 p2 = end + invDir - sidewaysDir * arrowLen * 0.1f;
	g_imRenderer->Begin(PRIM_LINES);
		g_imRenderer->Vertex3fv(start.GetDataConst());
		g_imRenderer->Vertex3fv(end.GetDataConst());
		g_imRenderer->Vertex3fv(p1.GetDataConst());
		g_imRenderer->Vertex3fv(end.GetDataConst());
		g_imRenderer->Vertex3fv(p2.GetDataConst());
		g_imRenderer->Vertex3fv(end.GetDataConst());
	g_imRenderer->End();

	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);

	// OpenGL path
	if (_col.a != 255)
		glEnable(GL_BLEND);

	glLineWidth(width / midPointToCameraDist * arrowLen);
	glColor3ubv(_col.GetData());

	glBegin(GL_LINES);
		glVertex3fv(start.GetDataConst());
		glVertex3fv(end.GetDataConst());
		glVertex3fv(p1.GetDataConst());
		glVertex3fv(end.GetDataConst());
		glVertex3fv(p2.GetDataConst());
		glVertex3fv(end.GetDataConst());
	glEnd();

	glDisable( GL_LINE_SMOOTH );
	glDisable( GL_BLEND );
}


void RenderPointMarker(LegacyVector3 const &point, char const *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);

	LegacyVector3 end(point + LegacyVector3(20,20,20));
	RenderArrow(end, point, 2.0f);
	g_editorFont.DrawText3DCentre(end, 3.0f, buf);
}


void PrintMatrix( const char *_name, GLenum _whichMatrix )
{
	GLdouble matrix[16];

	glGetDoublev( _whichMatrix, matrix );

	DebugTrace( "\tMatrix: %s\n", _name );
	for (int row = 0; row < 4; row++) {
		DebugTrace("\t\t");
		for (int col = 0; col < 4; col++)
			DebugTrace("% 13.1f ", matrix[col * 4 + row]);
		DebugTrace("\n");
	}
	DebugTrace("\n");
}

void PrintMatrices( const char *_title )
{
	static int numTimes = 0;
	numTimes++;
	if (numTimes > 10)
		return;

	DebugTrace(
		"OpenGL:   "
		"%s\n", _title);
	PrintMatrix( "Model View", GL_MODELVIEW_MATRIX );
	PrintMatrix( "Projection", GL_PROJECTION_MATRIX );

}

#endif // ifdef DEBUG_RENDER_ENABLED
