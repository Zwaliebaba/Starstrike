#include "pch.h"

#include "3d_sprite.h"

#include "app.h"
#include "camera.h"
#include "renderer.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"


void Render3DSprite(LegacyVector3 const &_pos, float _width, float _height, int _textureId)
{
	LegacyVector3 camUp = g_app->m_camera->GetUp();
	LegacyVector3 camRight = (camUp ^ g_app->m_camera->GetFront()) * (_width * 0.5f);
	camUp *= _height;

	LegacyVector3 bottomLeft(_pos - camRight);
	LegacyVector3 bottomRight(_pos + camRight);
	LegacyVector3 topLeft(bottomLeft + camUp);
	LegacyVector3 topRight(bottomRight + camUp);

	g_imRenderer->BindTexture(_textureId);
	g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
	g_imRenderer->Begin(PRIM_QUADS);
		g_imRenderer->TexCoord2f(1, 1);
		g_imRenderer->Vertex3fv(topLeft.GetData());
		g_imRenderer->TexCoord2f(0, 1);
		g_imRenderer->Vertex3fv(topRight.GetData());
		g_imRenderer->TexCoord2f(0, 0);
		g_imRenderer->Vertex3fv(bottomRight.GetData());
		g_imRenderer->TexCoord2f(1, 0);
		g_imRenderer->Vertex3fv(bottomLeft.GetData());
	g_imRenderer->End();
	g_imRenderer->UnbindTexture();
	g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
}
