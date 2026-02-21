#include "pch.h"

#include <math.h>
#include <float.h>

#include "2d_array.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "debug_utils.h"
#include "hi_res_time.h"
#include "math_utils.h"
#include "profiler.h"
#include "preferences.h"
#include "render_utils.h"
#include "resource.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "texture_manager.h"

#include "app.h"
#include "camera.h"
#include "main.h"
#include "renderer.h"
#include "water.h"
#include "location.h"
#include "level_file.h"

#define LIGHTMAP_TEXTURE_NAME "water_lightmap"

float const waveBrightnessScale = 4.0f;
float const shoreBrighteningFactor = 250.0f;
float const shoreNoiseFactor = 0.25f;


// ****************************************************************************
// Class Water
// ****************************************************************************

Water::Water()
:	m_waterDepths(NULL),
	m_shoreNoise(NULL),
	m_waterDepthMap(NULL),
	m_waveTableX(NULL),
	m_waveTableZ(NULL),
	m_renderWaterEffect(0),
	m_colourTable(NULL)
{
    if( !g_app->m_editing )
    {
        Landscape *land = &g_app->m_location->m_landscape;

	    GenerateLightMap();

	    int detail = g_prefsManager->GetInt( "RenderWaterDetail" );

        if (detail > 0)
	    {
            float worldSize = max( g_app->m_location->m_landscape.GetWorldSizeX(),
                                 g_app->m_location->m_landscape.GetWorldSizeZ() );
            worldSize /= 100.0f;

            m_cellSize = (float)detail * worldSize;

		    int alpha = ( g_app->m_negativeRenderer ? 0 : 255 );

		    // Load colour information from a bitmap
		    {
                char fullFilename[256];
                sprintf( fullFilename, "terrain/%s", g_app->m_location->m_levelFile->m_wavesColourFilename );

                if( Location::ChristmasModEnabled() == 1 )
                {
                    strcpy( fullFilename, "terrain/waves_earth.bmp" );
                }

			    BinaryReader *in = g_app->m_resource->GetBinaryReader(fullFilename);
			    BitmapRGBA bmp(in, "bmp");
			    m_colourTable = new RGBAColour[bmp.m_width];
			    m_numColours = bmp.m_width;
				for( int x = 0; x < bmp.m_width; ++x )
				{
					m_colourTable[x] = bmp.GetPixel( x, 1 );
					m_colourTable[x].a = alpha;
				}
			    delete in;
		    }

		    BuildTriangleStrips();

		    m_waveTableSizeX = (2.0f * land->GetWorldSizeX()) / m_cellSize + 2;
		    m_waveTableSizeZ = (2.0f * land->GetWorldSizeZ()) / m_cellSize + 2;
		    m_waveTableX = new float[m_waveTableSizeX];
		    m_waveTableZ = new float[m_waveTableSizeZ];

		    BuildOpenGlState();
	    }
    }
}


Water::~Water()
{

	m_renderVerts.Empty();
	delete [] m_waterDepths;	m_waterDepths = NULL;
	delete [] m_shoreNoise;		m_shoreNoise = NULL;
	delete [] m_colourTable;	m_colourTable = NULL;
	delete [] m_waveTableX;		m_waveTableX = NULL;
	delete [] m_waveTableZ;		m_waveTableZ = NULL;
}

void Water::GenerateLightMap()
{
    double startTime = GetHighResTime();

    #define MASK_SIZE 128

	float landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
	float scaleFactorX = 2.0f * landSizeX / (float)MASK_SIZE;
	float scaleFactorZ = 2.0f * landSizeZ / (float)MASK_SIZE;
	int low = MASK_SIZE / 4;
	int high = (MASK_SIZE / 4) * 3;
	float offX = low * scaleFactorX;
	float offZ = low * scaleFactorZ;


    //
    // Generate basic mask image data

	Array2D <float> landData;
	landData.Initialise(MASK_SIZE, MASK_SIZE, 0.0f);
	landData.SetAll(0.0f);

    for (int z = low; z < high; ++z )
    {
	    for (int x = low; x < high; ++x )
        {
			float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(
									(0.0f + (float)x) * scaleFactorX - offX,
									(0.0f + (float)z) * scaleFactorZ - offZ);
            if( landHeight > 0.0f )
            {
                landData.PutData(x, z, 1.0f);
            }
        }
    }


	//
	// Horizontal blur

	int const blurSize = 11;
	int const halfBlurSize = 5;
	float m[blurSize] = { 0.2, 0.3, 0.4, 0.5, 0.8, 1.0, 0.8, 0.5, 0.4, 0.3, 0.2 };
	for (int i = 0; i < blurSize; ++i)
	{
		m[i] /= 5.4f;
	}

	Array2D <float> tempData;
	tempData.Initialise(MASK_SIZE, MASK_SIZE, 0.0f);
	tempData.SetAll(0.0f);
	for (int z = 0; z < MASK_SIZE; ++z)
	{
		for (int x = 0; x < MASK_SIZE; ++x)
		{
			float val = landData.GetData(x, z);
			if (NearlyEquals(val, 0.0f)) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				tempData.AddToData(x + i - halfBlurSize, z, val * m[i]);
			}
		}
	}


	//
	// Vertical blur

	landData.SetAll(0.0f);
	for (int z = 0; z < MASK_SIZE; ++z)
	{
		for (int x = 0; x < MASK_SIZE; ++x)
		{
			float val = tempData.GetData(x, z);
			if (NearlyEquals(val, 0.0f)) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				landData.AddToData(x, z + i - halfBlurSize, val * m[i]);
			}
		}
	}


    //
    // Generate finished image and upload to openGL

    BitmapRGBA finalImage( MASK_SIZE, MASK_SIZE );
    for (int x = 0; x < MASK_SIZE; ++x)
    {
        for (int z = 0; z < MASK_SIZE; ++z)
        {
            float grayVal = (float)landData.GetData(x, z) * 855.0f;
			if (grayVal > 255.0f) grayVal = 255.0f;
            finalImage.PutPixel( x, MASK_SIZE - z - 1, RGBAColour(grayVal, grayVal, grayVal) );
        }
    }

    if( g_app->m_resource->GetBitmap(LIGHTMAP_TEXTURE_NAME) != NULL )
    {
        g_app->m_resource->DeleteBitmap(LIGHTMAP_TEXTURE_NAME);
	}

    if( g_app->m_resource->DoesTextureExist(LIGHTMAP_TEXTURE_NAME) )
	{
        g_app->m_resource->DeleteTexture(LIGHTMAP_TEXTURE_NAME);
    }

    g_app->m_resource->AddBitmap(LIGHTMAP_TEXTURE_NAME, finalImage);


	//
	// Create the water depth map

	float depthMapCellSize = (landSizeX * 2.0f) / (float)finalImage.m_height;
	m_waterDepthMap = new SurfaceMap2D <float> (
							landSizeX * 2.0f, landSizeZ * 2.0f,
							-landSizeX/2.0f, -landSizeZ/2.0f,
							depthMapCellSize, depthMapCellSize, 1.0f);
	if (!g_app->m_editing)
	{
		for (int z = 0; z < finalImage.m_height; ++z)
		{
			for (int x = 0; x < finalImage.m_width; ++x)
			{
				RGBAColour pixel = finalImage.GetPixel(x, z);
				float depth = (float)pixel.g / 255.0f;
				depth = 1.0f - depth;
				m_waterDepthMap->PutData(x, finalImage.m_height - z - 1, depth);
			}
		}
	}


	//
	// Take a low res copy of the water depth map that the flat water renderer
	// can efficiently use to determine where under water polys are needed

	int const flatWaterRatio = 4;	// One flat water quad is the same size as 8x8 dynamic water quads
	scaleFactorX *= flatWaterRatio;
	scaleFactorZ *= flatWaterRatio;
	m_flatWaterTiles = new Array2D<bool> (m_waterDepthMap->GetNumColumns() / flatWaterRatio,
										  m_waterDepthMap->GetNumRows() / flatWaterRatio,
										  false);

	for (int z = 0; z < m_flatWaterTiles->GetNumRows(); ++z)
	{
		for (int x = 0; x < m_flatWaterTiles->GetNumColumns(); ++x)
		{
			int currentVal = 0;
			for (int dz = 0; dz < flatWaterRatio; ++dz)
			{
				for (int dx = 0; dx < flatWaterRatio; ++dx)
				{
					float depth = m_waterDepthMap->GetData(x * flatWaterRatio + dx,
														   z * flatWaterRatio + dz);
					if (depth >= 1.0f)		// Very deep
						++currentVal;
				}
			}

			int const topScore = flatWaterRatio * flatWaterRatio;
			if (currentVal == topScore)
				m_flatWaterTiles->PutData(x, m_flatWaterTiles->GetNumRows() - z - 1, false);
			else
				m_flatWaterTiles->PutData(x, m_flatWaterTiles->GetNumRows() - z - 1, true);
		}
	}


    double totalTime = GetHighResTime() - startTime;
    DebugTrace( "Water lightmap generation took %dms\n", int(totalTime * 1000) );
}


void Water::BuildOpenGlState()
{
}


bool Water::IsVertNeeded(float x, float z)
{
	float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(x, z);
	if (landHeight > 4.0f)
	{
		return false;
	}

	float waterDepth = m_waterDepthMap->GetValue(x, z);
	if (waterDepth > 0.999f)
	{
		return false;
	}

	return true;
}


void Water::BuildTriangleStrips()
{
	m_renderVerts.SetStepDouble();
	m_strips.SetStepDouble();

	float const landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
	float const landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();

	float const lowX = -landSizeX * 0.5f;
	float const lowZ = -landSizeZ * 0.5f;
	float const highX = landSizeX * 1.5f;
	float const highZ = landSizeZ * 1.5f;
	int const maxXb = (highX - lowX) / m_cellSize;
	int const maxZb = (highZ - lowZ) / m_cellSize;

	WaterTriangleStrip *strip = new WaterTriangleStrip;
	strip->m_startRenderVertIndex = 0;
	int degen = 0;
	WaterVertex vertex1, vertex2;

	for (int zb = 0; zb < maxZb; ++zb)
	{
		float fz = lowZ + (float)zb * m_cellSize;

		for (int xb = 0; xb < maxXb; ++xb)
		{
			float fx = lowX + (float)xb * m_cellSize;
			bool needed1 = IsVertNeeded(fx - m_cellSize, fz);
			bool needed2 = IsVertNeeded(fx - m_cellSize, fz + m_cellSize);
			bool needed3 = IsVertNeeded(fx, fz);
			bool needed4 = IsVertNeeded(fx, fz + m_cellSize);
			bool needed5 = IsVertNeeded(fx + m_cellSize, fz);
			bool needed6 = IsVertNeeded(fx + m_cellSize, fz + m_cellSize);

			if (needed1 || needed2 || needed3 || needed4 || needed5 || needed6)
			{
				// Is needed, so add it to the strip
				vertex1.m_pos.Set(fx, 0.0f, fz);
				vertex2.m_pos.Set(fx, 0.0f, fz + m_cellSize);
				if(degen==1)
				{
					m_renderVerts.PutData(vertex1);
					m_renderVerts.PutData(vertex1);
				}
				degen = 2;
				m_renderVerts.PutData(vertex1);
				m_renderVerts.PutData(vertex2);
			}
			else
			{
				// Not needed, add degenerated joint.
				if(degen==2)
				{
					m_renderVerts.PutData(vertex2);
					m_renderVerts.PutData(vertex2);
					degen = 1;
				}
			}
		}
	}

	strip->m_numVerts = m_renderVerts.NumUsed();
	m_strips.PutData(strip);

	// Down-size the finished FastDArrays to fit their data tightly
	m_renderVerts.SetSize(m_renderVerts.NumUsed());
	m_strips.SetSize(m_strips.NumUsed());

	// Up-size the empty FastDArrays to be the same size as the vertex array
	m_waterDepths = new float[m_renderVerts.NumUsed()];
	m_shoreNoise = new float[m_renderVerts.NumUsed()];

	// Create other per-vertex arrays
	for (int i = 0; i < m_renderVerts.Size(); ++i)
	{
		LegacyVector3 const &pos = m_renderVerts[i].m_pos;
		float depth = m_waterDepthMap->GetValue(pos.x, pos.z);
		m_waterDepths[i] = depth;
		float shoreness = 1.0f - depth;
		float whiteness = shoreness + sfrand(shoreness) * shoreNoiseFactor;
		whiteness *= shoreBrighteningFactor;
		m_shoreNoise[i] = whiteness;
	}

	delete m_waterDepthMap; m_waterDepthMap = NULL;
}


void Water::RenderFlatWaterTiles(
		float posNorth, float posSouth, float posEast, float posWest, float height,
		float texNorth1, float texSouth1, float texEast1, float texWest1,
		float texNorth2, float texSouth2, float texEast2, float texWest2, int steps)
{
    float sizeX = posWest - posEast;
    float sizeZ = posSouth - posNorth;
	float posStepX = sizeX / (float)steps;
	float posStepZ = sizeZ / (float)steps;

	float texSizeX1 = texWest1 - texEast1;
	float texSizeZ1 = texSouth1 - texNorth1;
	float texStepX1 = texSizeX1 / (float)steps;
	float texStepZ1 = texSizeZ1 / (float)steps;

	float texSizeX2 = texWest2 - texEast2;
	float texSizeZ2 = texSouth2 - texNorth2;
	float texStepX2 = texSizeX2 / (float)steps;
	float texStepZ2 = texSizeZ2 / (float)steps;

		for (int j = 0; j < steps; ++j)
		{
			float pz = posNorth + j * posStepZ;
			float tz1 = texNorth1 + j * texStepZ1;
			float tz2 = texNorth2 + j * texStepZ2;

			for (int i = 0; i < steps; ++i)
			{
				float px = posEast + i * posStepX;

				if (m_flatWaterTiles->GetData(i, j) == false) continue;

				float tx1 = texEast1 + i * texStepX1;
				float tx2 = texEast2 + i * texStepX2;
			}
		}
}


void Water::RenderFlatWater()
{
    Landscape *land = &g_app->m_location->m_landscape;


    if( g_app->m_negativeRenderer )
    {
    }
    else
    {
    }

	char waterFilename[256];
	sprintf( waterFilename, "terrain/%s", g_app->m_location->m_levelFile->m_waterColourFilename );

    if( Location::ChristmasModEnabled() == 1 )
    {
        strcpy( waterFilename, "terrain/water_icecaps.bmp" );
    }

	// JAK HACK (DISABLED)

    float landSizeX = land->GetWorldSizeX();
    float landSizeZ = land->GetWorldSizeZ();
	float borderX = landSizeX / 2.0f;
    float borderZ = landSizeZ / 2.0f;

	float const timeFactor = g_gameTime / 30.0f;

	RenderFlatWaterTiles(
		landSizeZ + borderZ, -borderZ, -borderX, landSizeX + borderX, -9.0f,
		timeFactor, timeFactor + 30.0f, timeFactor, timeFactor + 30.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		m_flatWaterTiles->GetNumColumns());
}

bool isIdentical(const LegacyVector3& a,const LegacyVector3& b,const LegacyVector3& c)
{
	return a.x==b.x && a.x==c.x && a.z==b.z && a.z==c.z;
}

void Water::UpdateDynamicWater()
{
	float const scaleFactor = 7.0f;

	//
	// Generate lookup tables

	for (int i = 0; i < m_waveTableSizeX; ++i)
	{
		float x = (float)i * m_cellSize;
		float heightForX = sinf(x * 0.01f + g_gameTime * 0.65f) * 1.2f;
		heightForX += sinf(x * 0.03f + g_gameTime * 1.5f) * 0.9f;
		m_waveTableX[i] = heightForX * scaleFactor;
	}
	for (int i = 0; i < m_waveTableSizeZ; ++i)
	{
		float z = (float)i * m_cellSize;
		float heightForZ = sinf(z * 0.02f + g_gameTime * 0.75f) * 0.9f;
		heightForZ += sinf(z * 0.03f + g_gameTime * 1.85f) * 0.65f;
		m_waveTableZ[i] = heightForZ * scaleFactor;
	}


	//
	// Go through all the strips, updating vertex heights and poly colours

	int totalNumVertices = 0;

	for (int i = 0; i < m_strips.Size(); ++i)
	{
		WaterTriangleStrip *strip = m_strips[i];

		float prevHeight1 = 0.0f;
		float prevHeight2 = 0.0f;

		totalNumVertices += strip->m_numVerts;

		//
		// Set through the triangle strip in pairs of vertices

		int const finalVertIndex = strip->m_startRenderVertIndex + strip->m_numVerts - 1;
		for (int j = strip->m_startRenderVertIndex; j < finalVertIndex; ++j)
		{
			WaterVertex *vertex1 = &m_renderVerts[j];
			WaterVertex *vertex2 = &m_renderVerts[j+1];

			float const landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
			float const landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
			float const lowX = -landSizeX * 0.5f;
			float const lowZ = -landSizeZ * 0.5f;
			int indexX = int((vertex1->m_pos.x-lowX)/m_cellSize+0.1f);
			int indexZ = int((vertex1->m_pos.z-lowZ)/m_cellSize+0.1f);
			DarwiniaDebugAssert(indexX < m_waveTableSizeX);
			DarwiniaDebugAssert(indexZ + 1 < m_waveTableSizeZ);

			// Update the height and calc brightness for FIRST vertex of the pair
			vertex1->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ];
			vertex1->m_pos.y *= m_waterDepths[j];
			if(j>=2 && isIdentical(m_renderVerts[j-2].m_pos,m_renderVerts[j-1].m_pos,vertex1->m_pos))
			{
				// end of degenerated joint
				m_renderVerts[j-2].m_pos.y = vertex1->m_pos.y;
				m_renderVerts[j-1].m_pos.y = vertex1->m_pos.y;
			}
			float brightness = (prevHeight1 + prevHeight2 + vertex1->m_pos.y) * waveBrightnessScale;
			float shoreness = 1.0f - m_waterDepths[j];
			brightness *= shoreness;
			brightness += m_shoreNoise[j];
			prevHeight1 = vertex1->m_pos.y;

			// Update the height and calc brightness for SECOND vertex of the pair
			++j;
			vertex2->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ + 1];
			vertex2->m_pos.y *= m_waterDepths[j];
			float brightness2 = (prevHeight2 + prevHeight1 + vertex2->m_pos.y) * waveBrightnessScale;
			shoreness = 1.0f - m_waterDepths[j];
			brightness2 *= shoreness;
			brightness2 += m_shoreNoise[j];
			prevHeight2 = vertex2->m_pos.y;

			// Now update the colours for the two vertices (and hence triangles), but
			// mix their colours together to reduce the sawtooth effect caused by too
			// much contrast between two triangles in the same quad.
			vertex1->m_col = GetColour(Round(brightness2 * 0.7f + brightness * 0.3f));
			vertex2->m_col = GetColour(Round(brightness * 0.7f + brightness2 * 0.3f));

			// Update vertex normals
			float dx1 = -(m_waveTableX[indexX+1] - m_waveTableX[indexX-1])*m_waterDepths[j-1];
			float dx2 = -(m_waveTableX[indexX+1] - m_waveTableX[indexX-1])*m_waterDepths[j  ];
			float dz1 = -(m_waveTableZ[indexZ+1] - m_waveTableZ[indexZ  ])*m_waterDepths[j-1];
			float dz2 = -(m_waveTableZ[indexZ+2] - m_waveTableZ[indexZ+1])*m_waterDepths[j  ];
			// realistic, but with artifacts around islands in wild water
			vertex1->m_normal = LegacyVector3(dx1,vertex1->m_pos.y,dz1);
			vertex2->m_normal = LegacyVector3(dx2,vertex2->m_pos.y,dz2);
			// no artifacts around islands in wild water, but much less realistic
			//vertex1->m_normal = LegacyVector3(0,-vertex1->m_pos.y,0);
			//vertex2->m_normal = LegacyVector3(0,-vertex2->m_pos.y,0);

			if(j>=2 && isIdentical(vertex1->m_pos,vertex2->m_pos,m_renderVerts[j-2].m_pos))
			{
				// start of degenerated joint
				vertex1->m_pos.y = m_renderVerts[j-2].m_pos.y;
				vertex2->m_pos.y = m_renderVerts[j-2].m_pos.y;
			}
		}
	}

}


void Water::RenderDynamicWater()
{
	// TODO Phase 10: Implement D3D11 vertex buffer rendering for water strips
}

void Water::Render()
{
	m_renderWaterEffect = g_prefsManager->GetInt("RenderPixelShader", 2) == 1;
	if( g_app->m_editing )
	{
        START_PROFILE(g_app->m_profiler,  "Render Water" );

		Landscape *land = &g_app->m_location->m_landscape;
        float size = 100.0f;

        END_PROFILE(g_app->m_profiler,  "Render Water" );
	}
	else
	{
		if( g_prefsManager->GetInt( "RenderWaterDetail" ) > 0 )
        {
			//Advance();
			START_PROFILE(g_app->m_profiler,  "Render Water" );
			RenderFlatWater();
			RenderDynamicWater();
            END_PROFILE(g_app->m_profiler,  "Render Water" );
        }
        else
        {
            START_PROFILE(g_app->m_profiler,  "Render Water" );
    		RenderFlatWater();
            END_PROFILE(g_app->m_profiler,  "Render Water" );
        }
	}

    g_app->m_location->SetupFog();
    g_app->m_renderer->CheckOpenGLState();
}

void Water::Advance()
{
	if( !g_app->m_editing && g_prefsManager->GetInt( "RenderWaterDetail" ) > 0 )
	{
		START_PROFILE(g_app->m_profiler,  "Advance Water" );
		UpdateDynamicWater();
		END_PROFILE(g_app->m_profiler,  "Advance Water" );
	}
}
