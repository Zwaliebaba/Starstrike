#include "pch.h"
#include "FlagRenderer.h"
#include "profiler.h"
#include "text_renderer.h"
#include "camera.h"
#include "opengl_directx.h"
#include "flag.h"
#include "GameApp.h"
#include "main.h"


void FlagRenderer::Render(Flag& _flag)
{
    START_PROFILE( g_context->m_profiler, "RenderFlag" );

    //
    // Advance the flag

    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int z = 0; z < FLAG_RESOLUTION; ++z )
        {
            float factor1 = g_advanceTime * 10 * (FLAG_RESOLUTION - x) / FLAG_RESOLUTION;
            if( x == 0 ) factor1 = 1.0f;
            float factor2 = 1.0f - factor1;
            LegacyVector3 targetPos;

            if( x == 0 )
            {
                targetPos = _flag.m_pos + _flag.m_up * _flag.m_size / 2.0f +
                                (float) x / (float) (FLAG_RESOLUTION-1) * _flag.m_front * _flag.m_size +
                                (float) z / (float) (FLAG_RESOLUTION-1) * _flag.m_up * _flag.m_size;
            }
            else
            {
                targetPos = _flag.m_flag[x-1][z] +
                                1.0f/(float) (FLAG_RESOLUTION-1) * _flag.m_front * _flag.m_size;
            }

            targetPos.x += 1 * cosf( g_gameTime + x * 1.5f ) * (float) x / (float) FLAG_RESOLUTION;
            targetPos.y += 3 * sinf( g_gameTime + x * 1.1f ) * (float) x / (float) FLAG_RESOLUTION;
            targetPos.z += 1 * cosf( g_gameTime + x * 1.3f ) * (float) x / (float) FLAG_RESOLUTION;

            _flag.m_flag[x][z] = _flag.m_flag[x][z] * factor2 + targetPos * factor1;

            if( x > 0 )
            {
                float distance = (_flag.m_flag[x-1][z] - _flag.m_flag[x][z]).Mag();
                if( distance > 10.0f )
                {
                    LegacyVector3 theVector = _flag.m_flag[x-1][z] - _flag.m_flag[x][z];
                    theVector.SetLength(10.0f);
                    _flag.m_flag[x][z] = _flag.m_flag[x-1][z] - theVector;
                }
            }
        }
    }

    //
    // Render the flag pole

    glColor4ub( 255, 255, 100, 255 );

    glDisable       ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D );

    LegacyVector3 right = _flag.m_up ^ ( g_context->m_camera->GetFront() );
    right.SetLength( 0.2f );

    glBegin( GL_QUADS );
        glVertex3fv( (_flag.m_pos-right).GetData() );
        glVertex3fv( (_flag.m_pos+right).GetData() );
        glVertex3fv( (_flag.m_pos+_flag.m_up*_flag.m_size*1.5f+right).GetData() );
        glVertex3fv( (_flag.m_pos+_flag.m_up*_flag.m_size*1.5f-right).GetData() );
    glEnd();

    //
    // Render the flag

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_BLEND );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, _flag.m_texId );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

    glColor4f( 1.0f, 1.0f, 1.0f, 0.8f );
    for( int x = 0; x < FLAG_RESOLUTION-1; ++x )
    {
        for( int y = 0; y < FLAG_RESOLUTION-1; ++y )
        {
            float texX = (float) x / (float) (FLAG_RESOLUTION-1);
            float texY = (float) y / (float) (FLAG_RESOLUTION-1);
            float texW = 1.0f / (FLAG_RESOLUTION-1);

            glBegin( GL_QUADS );
                glTexCoord2f(texX,texY);            glVertex3fv( _flag.m_flag[x][y].GetData() );
                glTexCoord2f(texX+texW,texY);       glVertex3fv( _flag.m_flag[x+1][y].GetData() );
                glTexCoord2f(texX+texW,texY+texW);  glVertex3fv( _flag.m_flag[x+1][y+1].GetData() );
                glTexCoord2f(texX,texY+texW);       glVertex3fv( _flag.m_flag[x][y+1].GetData() );
            glEnd();
        }
    }

    glDepthMask     ( true );
    glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

    END_PROFILE( g_context->m_profiler, "RenderFlag" );
}


void FlagRenderer::RenderText(Flag& _flag, int _posX, int _posY, char* _caption)
{
    if( _posX < 0 || _posY < 0 ||
        _posX >= FLAG_RESOLUTION ||
        _posY >= FLAG_RESOLUTION )
    {
        return;
    }

    LegacyVector3 pos = _flag.m_flag[_posX][_posY];
    LegacyVector3 up = (_flag.m_flag[_posX][_posY+1] - pos);
    LegacyVector3 right = (_flag.m_flag[_posX+1][_posY] - pos);

    pos += right * 0.5f;
    pos += up * 0.5f;

    LegacyVector3 front = (right ^ up).Normalise();
    up = (front ^ right).Normalise();

    g_gameFont.DrawText3D( pos + front * 0.1f, front, up, 4.0f, _caption );
    g_gameFont.DrawText3D( pos - front * 0.1f, -front, up, 4.0f, _caption );
}
