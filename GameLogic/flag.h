
#ifndef _included_flag_h
#define _included_flag_h

#include "LegacyVector3.h"

#define FLAG_RESOLUTION 6


class Flag
{
protected:
    LegacyVector3 m_flag[FLAG_RESOLUTION][FLAG_RESOLUTION];

    int     m_texId;
    LegacyVector3 m_pos;
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    float   m_size;

public:
    Flag();

    void Initialise         ();

    void SetTexture         ( int _textureId );
    void SetPosition        ( LegacyVector3 const &_pos );
    void SetOrientation     ( LegacyVector3 const &_front, LegacyVector3 const &_up );
    void SetSize            ( float _size );

    void Render();
    void RenderText         ( int _posX, int _posY, char *_caption );
};


#endif