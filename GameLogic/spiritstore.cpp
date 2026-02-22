#include "pch.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"

#include "math_utils.h"
#include "profiler.h"
#include "resource.h"

#include "app.h"
#include "renderer.h"

#include "spiritstore.h"


SpiritStore::SpiritStore()
:   m_sizeX(0.0f),
    m_sizeY(0.0f),
    m_sizeZ(0.0f)
{
}

void SpiritStore::Initialise ( int _initialCapacity, int _maxCapacity, LegacyVector3 _pos,
                                float _sizeX, float _sizeY, float _sizeZ )
{
    m_spirits.SetSize( _maxCapacity );
    m_spirits.SetStepSize( _maxCapacity / 2 );
    m_pos = _pos;
    m_sizeX = _sizeX;
    m_sizeY = _sizeY;
    m_sizeZ = _sizeZ;

    for( int i = 0; i < _initialCapacity; ++i )
    {
        Spirit s;
        s.m_teamId = 0;
        s.Begin();
        AddSpirit( &s );
    }
}

void SpiritStore::Advance()
{
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *s = m_spirits.GetPointer(i);
            s->Advance();
        }
    }
}

void SpiritStore::Render( float _predictionTime )
{
	START_PROFILE(g_app->m_profiler, "Spirit Store");

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);

    g_imRenderer->Color4f( 1.0f, 1.0f, 1.0f, 0.5f );

    g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/triangleOutline.bmp", true, false));

    g_imRenderer->Begin(PRIM_QUADS);
        g_imRenderer->Normal3f(0,1,0);
        g_imRenderer->TexCoord2f( 0.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 0.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);

        g_imRenderer->Normal3f(0,0,-1);
        g_imRenderer->TexCoord2f( 0.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        g_imRenderer->TexCoord2f( 0.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);

        g_imRenderer->Normal3f(0,0,1);
        g_imRenderer->TexCoord2f( 0.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 0.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);

        g_imRenderer->Normal3f(1,0,0);
        g_imRenderer->TexCoord2f( 0.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 0.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);

        g_imRenderer->Normal3f(-1,0,0);
        g_imRenderer->TexCoord2f( 0.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 0.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        g_imRenderer->TexCoord2f( 1.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        g_imRenderer->TexCoord2f( 0.0f, 1.0f );     g_imRenderer->Vertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
    g_imRenderer->End();


    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *s = m_spirits.GetPointer(i);
            s->Render( _predictionTime );
        }
    }

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);

	END_PROFILE(g_app->m_profiler, "Spirit Store");
}


int SpiritStore::NumSpirits ()
{
    return m_spirits.NumUsed();
}


void SpiritStore::AddSpirit ( Spirit *_spirit )
{
    Spirit *target = m_spirits.GetPointer();
    *target = *_spirit;
    target->m_pos = m_pos + LegacyVector3(syncsfrand(m_sizeX*1.5f),
                                    syncsfrand(m_sizeY*1.5f),
                                    syncsfrand(m_sizeZ*1.5f) );
    target->m_state = Spirit::StateInStore;
    target->m_numNearbyEggs = 0;
}

void SpiritStore::RemoveSpirits ( int _quantity )
{
    int numRemoved = 0;
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            m_spirits.MarkNotUsed(i);
            ++numRemoved;
            if( numRemoved == _quantity ) return;
        }
    }
}
