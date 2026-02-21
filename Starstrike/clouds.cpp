#include "pch.h"

#include <math.h>

#include "resource.h"
#include "debug_utils.h"
#include "preferences.h"
#include "profiler.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "texture_manager.h"

#include "app.h"
#include "globals.h"
#include "clouds.h"
#include "location.h"


Clouds::Clouds()
{
    m_offset.Set(0.0f,0.0f,0.0f);
    m_vel.Set(0.04f,0.0f,0.0f);
}


void Clouds::Advance()
{
    m_vel.Set(0.03f,0.0f,-0.01f);
    m_offset += m_vel * SERVER_ADVANCE_PERIOD;
}


void Clouds::Render(float _predictionTime)
{
    START_PROFILE( g_app->m_profiler, "RenderSky" );
	RenderSky();
    END_PROFILE( g_app->m_profiler, "RenderSky" );

    START_PROFILE( g_app->m_profiler, "RenderBlobby" );
    RenderBlobby(_predictionTime);
    END_PROFILE( g_app->m_profiler, "RenderBlobby" );

    START_PROFILE( g_app->m_profiler, "RenderFlat" );
    RenderFlat(_predictionTime);
    END_PROFILE( g_app->m_profiler, "RenderFlat" );
}


// Renders a textured quad by spliting it up into a regular grid of smaller quads.
// This is needed to prevent fog artifacts on Radeon cards.
void Clouds::RenderQuad(float posNorth, float posSouth, float posEast, float posWest, float height,
						float texNorth, float texSouth, float texEast, float texWest)
{
	int const steps = 4;

    float sizeX = posWest - posEast;
    float sizeZ = posSouth - posNorth;
	float texSizeX = texWest - texEast;
	float texSizeZ = texSouth - texNorth;
	float posStepX = sizeX / (float)steps;
	float posStepZ = sizeZ / (float)steps;
	float texStepX = texSizeX / (float)steps;
	float texStepZ = texSizeZ / (float)steps;

    g_imRenderer->Begin(PRIM_QUADS);
        for (int j = 0; j < steps; ++j)
        {
            float pz = posNorth + j * posStepZ;
            float tz = texNorth + j * texStepZ;

            for (int i = 0; i < steps; ++i)
            {
                float px = posEast + i * posStepX;
                float tx = texEast + i * texStepX;

                g_imRenderer->TexCoord2f(tx + texStepX, tz);
                g_imRenderer->Vertex3f(px + posStepX, height, pz);

                g_imRenderer->TexCoord2f(tx + texStepX, tz + texStepZ);
                g_imRenderer->Vertex3f(px + posStepX, height, pz + posStepZ);

                g_imRenderer->TexCoord2f(tx, tz + texStepZ);
                g_imRenderer->Vertex3f(px, height, pz + posStepZ);

                g_imRenderer->TexCoord2f(tx, tz);
                g_imRenderer->Vertex3f(px, height, pz);
            }
        }
    g_imRenderer->End();

		for (int j = 0; j < steps; ++j)
		{
			float pz = posNorth + j * posStepZ;
			float tz = texNorth + j * texStepZ;

			for (int i = 0; i < steps; ++i)
			{
				float px = posEast + i * posStepX;
				float tx = texEast + i * texStepX;


			}
		}
}


void Clouds::RenderFlat( float _predictionTime )
{
	float r         = 8.0f;
	float height    = 1200.0f;
	float xStart    = -1000.0f*r;
	float xEnd      = 1000.0f + 1000.0f*r;
	float zStart    = -1000.0f*r;
	float zEnd      = 1000.0f + 1000.0f*r;
	float detail    = 9.0f;

	int cloudDetail = g_prefsManager->GetInt( "RenderCloudDetail", 1 );

	LegacyVector3 offset  = m_offset + m_vel * _predictionTime;

	int texId = g_app->m_resource->GetTexture("textures/clouds.bmp");
	g_imRenderer->BindTexture(texId);
	g_imRenderer->SetSampler(SAMPLER_NEAREST_WRAP);
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);


	float fogColor  [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	g_imRenderer->Color4f( 0.7f, 0.7f, 0.9f, 0.3f );

	if( cloudDetail == 3 )
	{
		g_imRenderer->Color4f( 0.7f, 0.7f, 0.9f, 0.5f );
	}

    RenderQuad(zStart, zEnd, xStart, xEnd, height,
			   offset.z, offset.z + detail, offset.x, offset.x + detail);

    detail          *= 0.5f;
    height          -= 200.0f;

    if( cloudDetail == 1 )
    {
	    RenderQuad(zStart, zEnd, xStart, xEnd, height,
			       offset.x/2, offset.x/2 + detail, offset.x/2, offset.x/2 + detail);
    }

    g_app->m_location->SetupFog();

    g_imRenderer->UnbindTexture();
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);


}

void Clouds::RenderBlobby( float _predictionTime )
{
    float r         = 8.0f;
    float height    = 1200.0f;
    float xStart    = -1000.0f*r;
    float xEnd      = 1000.0f + 1000.0f*r;
    float zStart    = -1000.0f*r;
    float zEnd      = 1000.0f + 1000.0f*r;
    float detail    = 9.0f;

	LegacyVector3 offset  = m_offset + m_vel * _predictionTime;

	int cloudDetail = g_prefsManager->GetInt( "RenderCloudDetail", 1 );

	int texId = g_app->m_resource->GetTexture("textures/clouds.bmp", false);
	g_imRenderer->BindTexture(texId);
	g_imRenderer->SetSampler(SAMPLER_LINEAR_WRAP);
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);


	float fogColor  [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	g_imRenderer->Color4f( 0.7f, 0.7f, 0.9f, 0.6f );

    if( cloudDetail == 1 || cloudDetail == 2 )
    {
        RenderQuad		(zStart, zEnd, xStart, xEnd, height,
	    				 offset.z, detail + offset.z, offset.x, detail + offset.x);
    }

    detail          *= 0.5f;
    height          -= 200.0f;

    RenderQuad		(zStart, zEnd, xStart, xEnd, height,
					 offset.x/2, detail + offset.x/2, offset.x/2, detail + offset.x/2);

    detail          *= 0.4f;
    height          -= 200.0f;

    if( cloudDetail == 1 )
    {
	    RenderQuad		(zStart, zEnd, xStart, xEnd, height,
					     offset.x, detail + offset.x, offset.z, detail + offset.z);
    }

    g_app->m_location->SetupFog();

    g_imRenderer->UnbindTexture();
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);

}


// *** RenderSky
void Clouds::RenderSky()
{
	float r = 3.0f;
	float height = 1200.0f;
	float gridSize = 80.0f;
	float lineWidth = 8.0f;

	float xStart = -2000.0f*r;
	float xEnd = 2000.0f + 2000.0f*r;
	float zStart = -2000.0f*r;
	float zEnd = 2000.0f + 2000.0f*r;

	float fogColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	int texId = g_app->m_resource->GetTexture("textures/laser.bmp");
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
	g_imRenderer->BindTexture(texId);
	g_imRenderer->Color4f(0.5, 0.5, 1.0, 0.3);


	g_imRenderer->Begin( PRIM_QUADS );
	for( int x = xStart; x < xEnd; x += gridSize )
	{
		g_imRenderer->TexCoord2i(0,0);      g_imRenderer->Vertex3f( x-lineWidth, height, zStart );
		g_imRenderer->TexCoord2i(0,1);      g_imRenderer->Vertex3f( x+lineWidth, height, zStart );
		g_imRenderer->TexCoord2i(1,1);      g_imRenderer->Vertex3f( x+lineWidth, height, zEnd );
		g_imRenderer->TexCoord2i(1,0);      g_imRenderer->Vertex3f( x-lineWidth, height, zEnd );

		g_imRenderer->TexCoord2i(1,0);      g_imRenderer->Vertex3f( xEnd, height, x-lineWidth );
		g_imRenderer->TexCoord2i(1,1);      g_imRenderer->Vertex3f( xEnd, height, x+lineWidth );
		g_imRenderer->TexCoord2i(0,1);      g_imRenderer->Vertex3f( xStart, height, x+lineWidth );
		g_imRenderer->TexCoord2i(0,0);      g_imRenderer->Vertex3f( xStart, height, x-lineWidth );
	}
	g_imRenderer->End();

	for( int x = xStart; x < xEnd; x += gridSize )
	{

	}

	g_imRenderer->UnbindTexture();
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);

	g_app->m_location->SetupFog();
}
