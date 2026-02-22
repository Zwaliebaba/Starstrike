#include "pch.h"

#include <float.h>

#include "2d_surface_map.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "math_utils.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "rgb_colour.h"
#include "texture_uv.h"
#include "LegacyVector3.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "texture_manager.h"

#include "app.h"
#include "camera.h"
#include "landscape_renderer.h"
#include "location.h"	// For SetupFog
#include "renderer.h"
#include "level_file.h"


#define MAIN_DISPLAY_LIST_NAME "LandscapeMain"
#define OVERLAY_DISPLAY_LIST_NAME "LandscapeOverlay"


//*****************************************************************************
// Protected Functions
//*****************************************************************************

void LandscapeRenderer::BuildVertArrayAndTriStrip(SurfaceMap2D <float> *_heightMap)
{
	m_highest = _heightMap->GetHighestValue();

	int const rows = _heightMap->GetNumColumns();
	int const cols = _heightMap->GetNumRows();

	m_verts.SetStepDouble();
	LandTriangleStrip *strip = new LandTriangleStrip();
	strip->m_firstVertIndex = 0;
	int degen = 0;
	LandVertex vertex1, vertex2;

	for (int z = 0; z < cols; ++z)
	{
		float fz = (float)z * _heightMap->m_cellSizeY;

		for (int x = 0; x < rows; ++x)
		{
			float heighta = _heightMap->GetData(x - 1, z    );
			float heightb = _heightMap->GetData(x - 1, z + 1);
			float height1 = _heightMap->GetData(x    , z    );
			float height2 = _heightMap->GetData(x    , z + 1);
			float height3 = _heightMap->GetData(x + 1, z    );
			float height4 = _heightMap->GetData(x + 1, z + 1);
			// Is this quad at least partly above water?
			if (heighta > 0.0f || heightb > 0.0f ||
				height1 > 0.0f || height2 > 0.0f ||
				height3 > 0.0f || height4 > 0.0f)
			{
				// Yes it is, add it to the strip
				float fx = (float)x * _heightMap->m_cellSizeX;
				vertex1.m_pos.Set(fx, height1, fz);
				vertex2.m_pos.Set(fx, height2, fz + _heightMap->m_cellSizeY);
				if (vertex1.m_pos.y < 0.3f) vertex1.m_pos.y = -10.0f;
				if (vertex2.m_pos.y < 0.3f) vertex2.m_pos.y = -10.0f;
				if(degen==1)
				{
					m_verts.PutData(vertex1);
					m_verts.PutData(vertex1);
				}
				degen = 2;
				m_verts.PutData(vertex1);
				m_verts.PutData(vertex2);
			}
			else
			{
				// No, quad is entirely below water, add degenerated joint.
				if(degen==2)
				{
					m_verts.PutData(vertex2);
					m_verts.PutData(vertex2);
					degen = 1;
				}
			}
		}
	}

	// end strip
	strip->m_numVerts = m_verts.NumUsed();
	m_strips.PutData(strip);
	m_numTriangles = strip->m_numVerts - 2;
}


void LandscapeRenderer::BuildNormArray()
{
	if (m_verts.Size() <= 0)
		return;

	int nextNormId = 0;

	// Go through all the strips...
	for (int i = 0; i < m_strips.Size(); ++i)
	{
		LandTriangleStrip *strip = m_strips[i];

		m_verts[nextNormId++].m_norm = g_upVector;
		m_verts[nextNormId++].m_norm = g_upVector;
		int const maxJ = strip->m_numVerts - 2;

		// For each vertex in strip
		for (int j = 0; j < maxJ; j += 2)
		{
			LegacyVector3 const &v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
			LegacyVector3 const &v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
			LegacyVector3 const &v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
			LegacyVector3 const &v4 = m_verts[strip->m_firstVertIndex + j + 3].m_pos;

			LegacyVector3 north(v1 - v2);
			LegacyVector3 northEast(v3 - v2);
			LegacyVector3 east(v4 - v2);

			LegacyVector3 norm1(northEast ^ north);
			norm1.Normalise();
			LegacyVector3 norm2(east ^ northEast);
			norm2.Normalise();

			m_verts[nextNormId++].m_norm = norm1;
			m_verts[nextNormId++].m_norm = norm2;

			int vertIndex = strip->m_firstVertIndex + j + 2;
			DarwiniaDebugAssert(nextNormId - 2 == vertIndex);
		}
	}

	int vertIndex = m_verts.NumUsed();
	DarwiniaDebugAssert(nextNormId == vertIndex);
}


void LandscapeRenderer::BuildUVArray(SurfaceMap2D <float> *_heightMap)
{
	if (m_verts.Size() <= 0)
		return;

	int nextUVId = 0;

    float factorX = 1.0f / _heightMap->m_cellSizeX;
    float factorZ = 1.0f / _heightMap->m_cellSizeY;

    for (int i = 0; i < m_strips.Size(); ++i)
	{
		LandTriangleStrip *strip = m_strips[i];

		int const numVerts = strip->m_numVerts;
		for (int j = 0; j < numVerts; ++j)
		{
			LegacyVector3 const &vert = m_verts[strip->m_firstVertIndex + j].m_pos;
			float u = vert.x * factorX;
			float v = vert.z * factorZ;
			m_verts[nextUVId++].m_uv = TextureUV(u, v);
		}
	}

	DarwiniaDebugAssert(nextUVId == m_verts.NumUsed());
}


// _gradient=1 means flat, _gradient=0 means vertical
// x and z are only passed in as a means to get predicatable noise
void LandscapeRenderer::GetLandscapeColour( float _height, float _gradient,
											unsigned int _x, unsigned int _y, RGBAColour *_colour )
{
	float heightAboveSea = _height;
    float u = powf(1.0f - _gradient, 0.4f);
	float v = 1.0f - heightAboveSea / m_highest;
    darwiniaSeedRandom(_x | _y + darwiniaRandom());
	if (heightAboveSea < 0.0f) heightAboveSea = 0.0f;
	v += sfrand(0.45f / (heightAboveSea + 2.0f));

    int x = u * m_landscapeColour->m_width;
    int y = v * m_landscapeColour->m_height;

    if( x < 0 ) x = 0;
    if( x > m_landscapeColour->m_width - 1) x = m_landscapeColour->m_width - 1;
    if( y < 0 ) y = 0;
    if( y > m_landscapeColour->m_height - 1) y = m_landscapeColour->m_height - 1;

    *_colour = m_landscapeColour->GetPixel(x, y);

    if( g_app->m_negativeRenderer ) _colour->a = 0;
}


void LandscapeRenderer::BuildColourArray()
{
	if (m_verts.Size() <= 0)
		return;

	int nextColId = 0;

	for (int i = 0; i < m_strips.Size(); ++i)
	{
		LandTriangleStrip *strip = m_strips[i];

		m_verts[nextColId++].m_col.Set(255,0,255);
		m_verts[nextColId++].m_col.Set(255,0,255);

		int const numVerts = strip->m_numVerts - 2;
		for (int j = 0; j < numVerts; ++j)
		{
			LegacyVector3 const &v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
			LegacyVector3 const &v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
			LegacyVector3 const &v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
			LegacyVector3 centre = (v1 + v2 + v3) * 0.33333f;
			LegacyVector3 const &norm = m_verts[strip->m_firstVertIndex + j + 2].m_norm;
			RGBAColour col;
			GetLandscapeColour(centre.y, norm.y,
							   (unsigned int)centre.x, (unsigned int)centre.z,
							   &col);

			//DebugTrace("%d[%d]: r:%02x g:%02x b:%02x a:%02x\n", i, j,  col.r, col.g, col.b, col.a);
			m_verts[nextColId++].m_col = col;
		}
	}

	DarwiniaDebugAssert(nextColId == m_verts.NumUsed());
}


//*****************************************************************************
// PublicFunctions
//*****************************************************************************

LandscapeRenderer::LandscapeRenderer(SurfaceMap2D <float> *_heightMap)
:	m_d3dVertexBuffer(nullptr),
	m_d3dVertexCount(0)
{
    char fullFilname[256];
    sprintf( fullFilname, "terrain/%s", g_app->m_location->m_levelFile->m_landscapeColourFilename );

    if( Location::ChristmasModEnabled() == 1 )
    {
        strcpy( fullFilname, "terrain/landscape_icecaps.bmp" );
    }

	// Read render mode from prefs file — always use D3D11 static VB now
	BinaryReader *reader = g_app->m_resource->GetBinaryReader(fullFilname);
	DarwiniaReleaseAssert(reader != NULL, "Failed to get resource %s", fullFilname);
	m_landscapeColour = new BitmapRGBA(reader, "bmp");
	delete reader;

	BuildRenderState(_heightMap);
}


LandscapeRenderer::~LandscapeRenderer()
{
	if (m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Release();
		m_d3dVertexBuffer = nullptr;
	}

	m_verts.Empty();
}

void LandscapeRenderer::BuildRenderState(SurfaceMap2D <float> *_heightMap)
{
	BuildVertArrayAndTriStrip(_heightMap);
	BuildNormArray();
	BuildColourArray();
	BuildUVArray(_heightMap);

	CreateD3DVertexBuffer();
}

void LandscapeRenderer::CreateD3DVertexBuffer()
{
	if (!g_renderDevice || m_verts.NumUsed() <= 0)
		return;

	int numVerts = m_verts.NumUsed();

	// Convert LandVertex array to ImRenderer-compatible vertex format
	struct LandImVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texcoord;
		DirectX::XMFLOAT3 normal;
	};

	LandImVertex* gpuVerts = new LandImVertex[numVerts];
	for (int i = 0; i < numVerts; ++i)
	{
		LandVertex& src = m_verts[i];
		LandImVertex& dst = gpuVerts[i];
		dst.pos      = { src.m_pos.x, src.m_pos.y, src.m_pos.z };
		dst.color    = { src.m_col.r / 255.0f, src.m_col.g / 255.0f, src.m_col.b / 255.0f, src.m_col.a / 255.0f };
		dst.texcoord = { src.m_uv.u, src.m_uv.v };
		dst.normal   = { src.m_norm.x, src.m_norm.y, src.m_norm.z };
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth     = numVerts * sizeof(LandImVertex);
	bd.Usage         = D3D11_USAGE_DEFAULT;
	bd.BindFlags     = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = gpuVerts;

	HRESULT hr = g_renderDevice->GetDevice()->CreateBuffer(&bd, &initData, &m_d3dVertexBuffer);
	delete[] gpuVerts;

	if (FAILED(hr))
	{
		m_d3dVertexBuffer = nullptr;
		m_d3dVertexCount = 0;
		return;
	}

	m_d3dVertexCount = numVerts;
}

void LandscapeRenderer::RenderMainSlow()
{
	if (!m_d3dVertexBuffer || !g_imRenderer)
		return;

	// Set blend state for negative renderer
	auto* ctx = g_renderDevice->GetContext();
	if (g_app->m_negativeRenderer)
		g_renderStates->SetBlendState(ctx, BLEND_SUBTRACTIVE_COLOR);

	g_imRenderer->UnbindTexture();

	// Upload full constant buffer (matrices, lighting, fog, etc.)
	g_imRenderer->UpdateConstantBuffer();
	ID3D11Buffer* cb = g_imRenderer->GetConstantBuffer();

	// Bind pipeline — same shaders as ImRenderer colored path
	UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2) + sizeof(DirectX::XMFLOAT3); // 48
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &m_d3dVertexBuffer, &stride, &offset);
	ctx->IASetInputLayout(g_imRenderer->GetInputLayout());
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ctx->VSSetShader(g_imRenderer->GetVertexShader(), nullptr, 0);
	ctx->VSSetConstantBuffers(0, 1, &cb);
	ctx->PSSetShader(g_imRenderer->GetColoredPixelShader(), nullptr, 0);
	ctx->PSSetConstantBuffers(0, 1, &cb);

	// Draw each strip
	for (int z = 0; z < m_strips.Size(); ++z)
	{
		LandTriangleStrip *strip = m_strips[z];
		ctx->Draw(strip->m_numVerts, strip->m_firstVertIndex);
	}

	if (g_app->m_negativeRenderer)
		g_renderStates->SetBlendState(ctx, BLEND_ALPHA);
}


void LandscapeRenderer::RenderOverlaySlow()
{
	if (!m_d3dVertexBuffer || !g_imRenderer)
		return;

	auto* ctx = g_renderDevice->GetContext();

	// Set overlay state: additive blend, depth readonly, textured
	if (!g_app->m_negativeRenderer)
		g_renderStates->SetBlendState(ctx, BLEND_ADDITIVE);
	else
		g_renderStates->SetBlendState(ctx, BLEND_SUBTRACTIVE_COLOR);

	g_renderStates->SetDepthState(ctx, DEPTH_ENABLED_READONLY);

	int outlineTextureId = g_app->m_resource->GetTexture("textures/triangleOutline.bmp", true, false);
	g_imRenderer->BindTexture(outlineTextureId);
	g_imRenderer->SetSampler(SAMPLER_LINEAR_WRAP);

	// Upload full constant buffer
	g_imRenderer->UpdateConstantBuffer();
	ID3D11Buffer* cb = g_imRenderer->GetConstantBuffer();

	// Bind pipeline — textured path
	UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2) + sizeof(DirectX::XMFLOAT3);
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &m_d3dVertexBuffer, &stride, &offset);
	ctx->IASetInputLayout(g_imRenderer->GetInputLayout());
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ctx->VSSetShader(g_imRenderer->GetVertexShader(), nullptr, 0);
	ctx->VSSetConstantBuffers(0, 1, &cb);
	ctx->PSSetConstantBuffers(0, 1, &cb);

	ID3D11ShaderResourceView* srv = g_textureManager->GetSRV(outlineTextureId);
	if (srv)
	{
		ctx->PSSetShader(g_imRenderer->GetTexturedPixelShader(), nullptr, 0);
		ctx->PSSetShaderResources(0, 1, &srv);
	}
	else
	{
		ctx->PSSetShader(g_imRenderer->GetColoredPixelShader(), nullptr, 0);
	}

	for (int z = 0; z < m_strips.Size(); ++z)
	{
		LandTriangleStrip *strip = m_strips[z];
		ctx->Draw(strip->m_numVerts, strip->m_firstVertIndex);
	}

	// Unbind SRV and restore state
	if (srv)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		ctx->PSSetShaderResources(0, 1, &nullSRV);
	}
	g_imRenderer->UnbindTexture();
	g_renderStates->SetDepthState(ctx, DEPTH_ENABLED_WRITE);
	g_renderStates->SetBlendState(ctx, BLEND_ALPHA);
}


void LandscapeRenderer::Render()
{
	if (m_verts.Size() <= 0)
		return;

	g_app->m_location->SetupFog();

	START_PROFILE(g_app->m_profiler, "Render Landscape Main");
	RenderMainSlow();
	END_PROFILE(g_app->m_profiler, "Render Landscape Main");

	int landscapeDetail = g_prefsManager->GetInt( "RenderLandscapeDetail", 1 );
	if( landscapeDetail < 4 )
	{
		START_PROFILE(g_app->m_profiler, "Render Landscape Overlay");
		RenderOverlaySlow();
		END_PROFILE(g_app->m_profiler, "Render Landscape Overlay");
	}
}
