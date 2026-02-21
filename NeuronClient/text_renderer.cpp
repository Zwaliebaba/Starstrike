#include "pch.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "binary_stream_readers.h"
#include "bitmap.h"
#include "debug_utils.h"
#include "filesys_utils.h"
#include "resource.h"
#include "LegacyVector3.h"
#include "window_manager.h"

#include "app.h"
#include "camera.h"
#include "text_renderer.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "texture_manager.h"


TextRenderer g_gameFont;
TextRenderer g_editorFont;


// Horizontal size as a proportion of vertical size
#define HORIZONTAL_SIZE		0.6f

#define TEX_MARGIN			0.003f
#define TEX_STRETCH			(1.0f - (26.0f * TEX_MARGIN))

#define TEX_WIDTH			((1.0f / 16.0f) * TEX_STRETCH * 0.9f)
#define TEX_HEIGHT			((1.0f / 14.0f) * TEX_STRETCH)


// ****************************************************************************
//  Class TextRenderer
// ****************************************************************************

void TextRenderer::Initialise(char const *_filename)
{
	m_filename = strdup(_filename);
	BuildOpenGlState();
    m_renderShadow = false;
    m_renderOutline = false;
}


void TextRenderer::BuildOpenGlState()
{
	BinaryReader *reader = g_app->m_resource->GetBinaryReader(m_filename);
	char const *extension = GetExtensionPart(m_filename);
	BitmapRGBA bmp(reader, extension);
	delete reader;

	m_bitmapWidth = bmp.m_width * 2;
	m_bitmapHeight = bmp.m_height * 2;

	BitmapRGBA scaledUp(m_bitmapWidth, m_bitmapHeight);
	scaledUp.Blit(0, 0, bmp.m_width, bmp.m_height, &bmp, 0, 0, scaledUp.m_width, scaledUp.m_height, false);
	m_textureID = scaledUp.ConvertToTexture();
}


void TextRenderer::BeginText2D()
{
	// D3D11: save current matrices, set ortho projection for 2D text
	if (g_imRenderer)
	{
		m_savedProjMatrix = g_imRenderer->GetProjectionMatrix();
		m_savedViewMatrix = g_imRenderer->GetViewMatrix();
		m_savedWorldMatrix = g_imRenderer->GetWorldMatrix();

		int v[4] = { 0, 0, g_renderDevice->GetBackBufferWidth(), g_renderDevice->GetBackBufferHeight() };
		float left   = v[0] - 0.325f;
		float right  = v[0] + v[2] - 0.325f;
		float bottom = v[1] + v[3] - 0.325f;
		float top    = v[1] - 0.325f;
		g_imRenderer->SetProjectionMatrix(DirectX::XMMatrixOrthographicOffCenterRH(left, right, bottom, top, -1.0f, 1.0f));
		g_imRenderer->SetViewMatrix(DirectX::XMMatrixIdentity());
		g_imRenderer->SetWorldMatrix(DirectX::XMMatrixIdentity());
	}
}


void TextRenderer::EndText2D()
{


	// D3D11: restore saved matrices
	if (g_imRenderer)
	{
		g_imRenderer->SetProjectionMatrix(m_savedProjMatrix);
		g_imRenderer->SetViewMatrix(m_savedViewMatrix);
		g_imRenderer->SetWorldMatrix(m_savedWorldMatrix);
	}
}


float TextRenderer::GetTexCoordX( unsigned char theChar )
{
	float const charWidth = 1.0f / 16.0f; // In OpenGL texture UV space
    float xPos = (theChar % 16);
    float texX = xPos * charWidth + TEX_MARGIN + 0.002f;
    return texX;
}


float TextRenderer::GetTexCoordY( unsigned char theChar )
{
	float const charHeight = 1.0f / 14.0f;
    float yPos = (theChar >> 4) - 2;
    float texY = yPos * charHeight; // In OpenGL texture UV space
    texY = 1.0f - texY - charHeight + TEX_MARGIN + 0.001f;
    return texY;
}


void TextRenderer::DrawText2DUp( float _x, float _y, float _size, char const *_text, ... )
{
	float horiSize = _size * HORIZONTAL_SIZE;

	if( m_renderShadow )    { g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR); }
	else                    { g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE); }

	g_imRenderer->BindTexture(m_textureID);


	unsigned numChars = strlen(_text);
	for( unsigned int i = 0; i < numChars; ++i )
	{
		unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			g_imRenderer->Begin( PRIM_QUADS );
				g_imRenderer->TexCoord2f(texX, texY + TEX_HEIGHT);		        g_imRenderer->Vertex2f( _x,				_y + horiSize);
				g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	g_imRenderer->Vertex2f( _x,	            _y );
				g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY);				g_imRenderer->Vertex2f( _x + _size,	    _y );
				g_imRenderer->TexCoord2f(texX, texY);							g_imRenderer->Vertex2f( _x + _size,     _y + horiSize);
			g_imRenderer->End();

		}

		_y -= horiSize;
	}

	g_imRenderer->UnbindTexture();
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
}


void TextRenderer::DrawText2DDown( float _x, float _y, float _size, char const *_text, ... )
{
	float horiSize = _size * HORIZONTAL_SIZE;

	if( m_renderShadow ||
		m_renderOutline )   { g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR); }
	else                    { g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE); }

	g_imRenderer->BindTexture(m_textureID);


	unsigned numChars = strlen(_text);
	for( unsigned int i = 0; i < numChars; ++i )
	{
		unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			if( m_renderOutline )
			{
				for( int x = -1; x <= 1; ++x )
				{
					for( int y = -1; y <= 1; ++y )
					{
						if( x != 0 || y != 0 )
						{
							g_imRenderer->Begin( PRIM_QUADS );
								g_imRenderer->TexCoord2f(texX, texY + TEX_HEIGHT);		        g_imRenderer->Vertex2f( _x+x + _size,     _y+y);
								g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	g_imRenderer->Vertex2f( _x+x + _size,	    _y+y+horiSize );
								g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY);				g_imRenderer->Vertex2f( _x+x,	            _y+y+horiSize );
								g_imRenderer->TexCoord2f(texX, texY);							g_imRenderer->Vertex2f( _x+x,             _y+y);
							g_imRenderer->End();

						}
					}
				}
			}
			else
			{
				g_imRenderer->Begin( PRIM_QUADS );
					g_imRenderer->TexCoord2f(texX, texY + TEX_HEIGHT);		        g_imRenderer->Vertex2f( _x + _size,     _y);
					g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	g_imRenderer->Vertex2f( _x + _size,	    _y+horiSize );
					g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY);				g_imRenderer->Vertex2f( _x,	            _y+horiSize );
					g_imRenderer->TexCoord2f(texX, texY);							g_imRenderer->Vertex2f( _x,             _y);
				g_imRenderer->End();

			}
		}

		_y += horiSize;
	}

	g_imRenderer->UnbindTexture();
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
}


void TextRenderer::SetRenderShadow ( bool _renderShadow )
{
    m_renderShadow = _renderShadow;
}


void TextRenderer::SetRenderOutline( bool _renderOutline )
{
    m_renderOutline = _renderOutline;
}


void TextRenderer::DrawText2DSimple(float _x, float _y, float _size, char const *_text )
{
	// Compatibility wank - needed to achieve the same behaviour the old code had
	_y -= 7.0f;
	_x -= 3.0f;

	float horiSize = _size * HORIZONTAL_SIZE;

	if( m_renderShadow ||
		m_renderOutline )   { g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR); }
	else                    { g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE); }

	g_imRenderer->BindTexture(m_textureID);


	unsigned numChars = strlen(_text);
	for( unsigned int i = 0; i < numChars; ++i )
	{
		unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			if( m_renderOutline )
			{
				for( int x = -1; x <= 1; ++x )
				{
					for( int y = -1; y <= 1; ++y )
					{
						if( x != 0 || y != 0 )
						{
							g_imRenderer->Begin( PRIM_QUADS );
								g_imRenderer->TexCoord2f(texX, texY + TEX_HEIGHT);		        g_imRenderer->Vertex2f( _x+x,				_y+y );
								g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	g_imRenderer->Vertex2f( _x+x + horiSize,	_y+y );
								g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY);				g_imRenderer->Vertex2f( _x+x + horiSize,	_y+y + _size );
								g_imRenderer->TexCoord2f(texX, texY);							g_imRenderer->Vertex2f( _x+x,				_y+y + _size );
							g_imRenderer->End();

						}
					}
				}
			}
			else
			{
				g_imRenderer->Begin( PRIM_QUADS );
					g_imRenderer->TexCoord2f(texX, texY + TEX_HEIGHT);		        g_imRenderer->Vertex2f( _x,				_y );
					g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	g_imRenderer->Vertex2f( _x + horiSize,	_y );
					g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY);				g_imRenderer->Vertex2f( _x + horiSize,	_y + _size );
					g_imRenderer->TexCoord2f(texX, texY);							g_imRenderer->Vertex2f( _x,				_y + _size );
				g_imRenderer->End();

			}
		}

		_x += horiSize;
	}

	g_imRenderer->UnbindTexture();
	g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
	//glDisable       ( GL_BLEND );                             // Not here, Blending is enabled during Eclipse render
}

// Draw the text, justified depending on the _xJustification parameter.
//		_xJustification < 0		Right justified text
//	    _xJustification == 0	Centred text
//		_xJustification > 0		Left justified text
void TextRenderer::DrawText2DJustified( float _x, float _y, float _size, int _xJustification, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	if (_xJustification > 0)
		DrawText2DSimple( _x, _y, _size, buf );			// Left Justification
	else {
		float width = GetTextWidth( strlen(buf), _size );

		if (_xJustification < 0)
			DrawText2DSimple( _x - width, _y, _size, buf );	// Right Justification
		else
			DrawText2DSimple( _x - width/2, _y, _size, buf );	// Centre
	}
}


void TextRenderer::DrawText2D( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);
    DrawText2DSimple( _x, _y, _size, buf );
}


void TextRenderer::DrawText2DRight( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    float width = GetTextWidth( strlen(buf), _size );
    DrawText2DSimple( _x - width, _y, _size, buf );
}


void TextRenderer::DrawText2DCentre( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    float width = GetTextWidth( strlen(buf), _size );
    DrawText2DSimple( _x - width/2, _y, _size, buf );
}

namespace {
	// RAII helper that saves and restores D3D11 render states around text drawing
	class SaveFontDrawAttributes {
	public:
		SaveFontDrawAttributes()
		{
			m_savedBlend = g_renderStates->GetCurrentBlendState();
			m_savedDepth = g_renderStates->GetCurrentDepthState();
			m_savedRaster = g_renderStates->GetCurrentRasterState();
		}

		~SaveFontDrawAttributes()
		{
			auto* ctx = g_renderDevice->GetContext();
			g_renderStates->SetBlendState(ctx, m_savedBlend);
			g_renderStates->SetDepthState(ctx, m_savedDepth);
			g_renderStates->SetRasterState(ctx, m_savedRaster);
		}

	private:
		BlendStateId m_savedBlend;
		DepthStateId m_savedDepth;
		RasterStateId m_savedRaster;
	};
}

void TextRenderer::DrawText3DSimple( LegacyVector3 const &_pos, float _size, char const *_text )
{
	// Store render states
	SaveFontDrawAttributes saveAttribs;

	Camera *cam = g_app->m_camera;
	LegacyVector3 pos(_pos);
	LegacyVector3 vertSize = cam->GetUp() * _size;
	LegacyVector3 horiSize = -cam->GetRight() * _size * HORIZONTAL_SIZE;
	unsigned int numChars = strlen(_text);
	pos += vertSize * 0.5f;


	//
	// Render the text

	g_renderStates->SetBlendState(g_renderDevice->GetContext(), m_renderShadow ? BLEND_SUBTRACTIVE_COLOR : BLEND_ADDITIVE);
	g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
	g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);
	g_imRenderer->BindTexture(m_textureID);

	for( unsigned int i = 0; i < numChars; ++i )
	{
		unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			g_imRenderer->Begin( PRIM_QUADS );
				g_imRenderer->TexCoord2f(texX, texY + TEX_HEIGHT);				g_imRenderer->Vertex3fv( (pos).GetData() );
				g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	g_imRenderer->Vertex3fv( (pos + horiSize).GetData() );
				g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY);				g_imRenderer->Vertex3fv( (pos + horiSize - vertSize).GetData() );
				g_imRenderer->TexCoord2f(texX, texY);							g_imRenderer->Vertex3fv( (pos - vertSize).GetData() );
			g_imRenderer->End();

		}

		pos += horiSize;
	}

	g_imRenderer->UnbindTexture();
}


void TextRenderer::DrawText3D( LegacyVector3 const &_pos, float _size, char const *_text, ... )
{
	// Convert the variable argument list into a single string
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	DrawText3DSimple(_pos, _size, buf);
}


void TextRenderer::DrawText3DCentre( LegacyVector3 const &_pos, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	LegacyVector3 pos = _pos;
	float amount =  HORIZONTAL_SIZE * (float)strlen(buf) * 0.5f * _size;
	pos += g_app->m_camera->GetRight() * amount;

	DrawText3DSimple(pos, _size, buf);
}


void TextRenderer::DrawText3DRight( LegacyVector3 const &_pos, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	LegacyVector3 pos = _pos;
	float amount =  HORIZONTAL_SIZE * (float)strlen(buf) * _size;
	pos += g_app->m_camera->GetRight() * amount;

	DrawText3DSimple(pos, _size, buf);
}


void TextRenderer::DrawText3D( LegacyVector3 const &_pos, LegacyVector3 const &_front, LegacyVector3 const &_up,
                               float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	SaveFontDrawAttributes saveAttribs;

	Camera *cam = g_app->m_camera;

	LegacyVector3 pos = _pos;
	LegacyVector3 vertSize = _up * _size;
	LegacyVector3 horiSize = ( _up ^ _front ) * _size * HORIZONTAL_SIZE;
	unsigned int numChars = strlen(buf);
	pos -= horiSize * numChars * 0.5f;
	pos += vertSize * 0.5f;


	//
	// Render the text

	g_renderStates->SetBlendState(g_renderDevice->GetContext(), m_renderShadow ? BLEND_SUBTRACTIVE_COLOR : BLEND_ADDITIVE);
	g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
	g_imRenderer->BindTexture(m_textureID);

	//glDisable		(GL_DEPTH_TEST);
	//glColor3ub		(255,255,255);

	for( unsigned int i = 0; i < numChars; ++i )
	{
		unsigned char thisChar = buf[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			g_imRenderer->Begin( PRIM_QUADS );
				g_imRenderer->TexCoord2f(texX, texY + TEX_HEIGHT);				g_imRenderer->Vertex3fv( (pos).GetData() );
				g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	g_imRenderer->Vertex3fv( (pos + horiSize).GetData() );
				g_imRenderer->TexCoord2f(texX + TEX_WIDTH, texY);				g_imRenderer->Vertex3fv( (pos + horiSize - vertSize).GetData() );
				g_imRenderer->TexCoord2f(texX, texY);							g_imRenderer->Vertex3fv( (pos - vertSize).GetData() );
			g_imRenderer->End();

		}

		pos += horiSize;
	}

	g_imRenderer->UnbindTexture();
}


float TextRenderer::GetTextWidth( unsigned int _numChars, float _size )
{
    return _numChars * _size * HORIZONTAL_SIZE;
}
