#include "pch.h"
#include "flag.h"
#include "GameAppSim.h"
#include "main.h"

Flag::Flag()
{
}


void Flag::Initialise()
{
    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int z = 0; z < FLAG_RESOLUTION; ++z )
        {
            LegacyVector3 targetPos = m_pos +
                                (float) x / (float) FLAG_RESOLUTION * m_front +
                                (float) z / (float) FLAG_RESOLUTION * m_up;

            m_flag[x][z] = targetPos;
        }
    }
}


void Flag::SetTexture( int _textureId )
{
    m_texId = _textureId;
}


void Flag::SetPosition( LegacyVector3 const &_pos )
{
    m_pos = _pos;
}


void Flag::SetOrientation( LegacyVector3 const &_front, LegacyVector3 const &_up )
{
    m_front = _front;
    m_up = _up;
}


void Flag::SetSize( float _size )
{
    m_size = _size;
}

