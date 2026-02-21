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

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    glEnable        ( GL_BLEND );
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
    glDepthMask     ( false );
    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
    glDisable       ( GL_CULL_FACE );

    g_imRenderer->Color4f( 1.0f, 1.0f, 1.0f, 0.5f );
    glColor4f       ( 1.0f, 1.0f, 1.0f, 0.5f );

    glEnable        (GL_TEXTURE_2D);
    g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/triangleOutline.bmp", true, false));
    glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/triangleOutline.bmp", true, false));
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

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

    glBegin(GL_QUADS);
        glNormal3f(0,1,0);
        glTexCoord2f( 0.0f, 0.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0f, 0.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0f, 1.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 0.0f, 1.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);

        glNormal3f(0,0,-1);
        glTexCoord2f( 0.0f, 0.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0f, 0.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0f, 1.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 0.0f, 1.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);

        glNormal3f(0,0,1);
        glTexCoord2f( 0.0f, 0.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0f, 0.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0f, 1.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 0.0f, 1.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);

        glNormal3f(1,0,0);
        glTexCoord2f( 0.0f, 0.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0f, 0.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0f, 1.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 0.0f, 1.0f );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);

        glNormal3f(-1,0,0);
        glTexCoord2f( 0.0f, 0.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0f, 0.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0f, 1.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 0.0f, 1.0f );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
    glEnd();

    glDisable       ( GL_TEXTURE_2D );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *s = m_spirits.GetPointer(i);
            s->Render( _predictionTime );
        }
    }

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
    glDisable       ( GL_BLEND );
    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
    glEnable        ( GL_CULL_FACE );
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
    glDepthMask     ( true );

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
