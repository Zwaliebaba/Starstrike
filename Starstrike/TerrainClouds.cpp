#include "pch.h"
#include "TerrainClouds.h"
#include "CameraView.h"
#include "Light.h"
#include "Scene.h"
#include "Terrain.h"
#include "TerrainRegion.h"

TerrainClouds::TerrainClouds(Terrain* terr, int t)
  : terrain(terr),
    type(t)
{
  nverts = 0;
  npolys = 0;
  mverts = nullptr;
  verts = nullptr;
  polys = nullptr;

  loc = Point(0, 15000, 0);
  radius = 25000.0f;

  BuildClouds();
}

TerrainClouds::~TerrainClouds()
{
  delete [] mverts;
  delete verts;
  delete [] polys;
}

static constexpr double BANK_SIZE = 20000;
static constexpr int CIRRUS_BANKS = 4;
static constexpr int CUMULUS_BANKS = 4;

void TerrainClouds::BuildClouds()
{
  if (type == 0)
  {
    nbanks = CIRRUS_BANKS;
    nverts = 4 * nbanks;
    npolys = 2 * nbanks;
  }
  else
  {
    nbanks = CUMULUS_BANKS;
    nverts = 8 * nbanks;
    npolys = 3 * nbanks;
  }

  Bitmap* cloud_texture = terrain->CloudTexture(type);
  Bitmap* shade_texture = terrain->ShadeTexture(type);

  mtl_cloud.name = "cloud";
  mtl_cloud.Ka = Color::White;
  mtl_cloud.Kd = Color::White;
  mtl_cloud.luminous = true;
  mtl_cloud.blend = Material::MTL_TRANSLUCENT;
  mtl_cloud.tex_diffuse = cloud_texture;

  mtl_shade.name = "shade";
  mtl_shade.Ka = Color::White;
  mtl_shade.Kd = Color::White;
  mtl_shade.luminous = true;
  mtl_shade.blend = Material::MTL_TRANSLUCENT;
  mtl_shade.tex_diffuse = shade_texture;

  verts = NEW VertexSet(nverts);
  mverts = NEW Vec3[nverts];
  polys = NEW Poly[npolys];

  verts->nverts = nverts;

  // initialize vertices
  Vec3* pVert = mverts;
  float* pTu = verts->tu;
  float* pTv = verts->tv;

  int i, j, n;
  double az = 0;
  double r = 0;

  for (n = 0; n < nbanks; n++)
  {
    double xloc = r * cos(az);
    double yloc = r * sin(az);
    double alt = rand() / 32.768;

    for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 2; j++)
      {
        *pVert = Vec3(static_cast<float>((2 * j - 1) * BANK_SIZE + xloc), static_cast<float>(alt),
                      static_cast<float>((2 * i - 1) * BANK_SIZE + yloc));

        *pTu++ = static_cast<float>(-j);
        *pTv++ = static_cast<float>(i);

        float dist = pVert->length();
        if (dist > radius)
          radius = dist;

        pVert++;
      }
    }

    if (type > 0)
    {
      for (i = 0; i < 2; i++)
      {
        for (j = 0; j < 2; j++)
        {
          *pVert = Vec3(static_cast<float>((2 * j - 1) * BANK_SIZE + xloc), static_cast<float>(alt - 100),
                        static_cast<float>((2 * i - 1) * BANK_SIZE + yloc));

          *pTu++ = static_cast<float>(-j);
          *pTv++ = static_cast<float>(i);

          float dist = pVert->length();
          if (dist > radius)
            radius = dist;

          pVert++;
        }
      }
    }

    az += (0.66 + rand() / 32768.0) * 0.25 * PI;

    if (r < BANK_SIZE)
      r += BANK_SIZE;
    else if (r < 1.75 * BANK_SIZE)
      r += BANK_SIZE / 4;
    else
      r += BANK_SIZE / 8;
  }

  // create the polys
  for (i = 0; i < npolys; i++)
  {
    Poly* p = polys + i;
    p->nverts = 4;
    p->vertex_set = verts;
    p->material = (i < 4 * nbanks) ? &mtl_cloud : &mtl_shade;
    p->visible = 1;
    p->sortval = (i < 4 * nbanks) ? 1 : 2;
  }

  // build main patch polys: (facing down)
  Poly* p = polys;

  int stride = 4;
  if (type > 0)
    stride = 8;

  // clouds:
  for (n = 0; n < nbanks; n++)
  {
    p->verts[0] = 0 + n * stride;
    p->verts[1] = 1 + n * stride;
    p->verts[2] = 3 + n * stride;
    p->verts[3] = 2 + n * stride;
    p++;

    // reverse side: (facing up)
    p->verts[0] = 0 + n * stride;
    p->verts[3] = 1 + n * stride;
    p->verts[2] = 3 + n * stride;
    p->verts[1] = 2 + n * stride;
    p++;
  }

  // shades:
  if (type > 0)
  {
    for (n = 0; n < nbanks; n++)
    {
      p->verts[0] = 4 + n * stride;
      p->verts[1] = 5 + n * stride;
      p->verts[2] = 7 + n * stride;
      p->verts[3] = 6 + n * stride;
      p++;
    }
  }

  // update the verts and colors of each poly:
  for (i = 0; i < npolys; i++)
  {
    Poly* p = polys + i;
    WORD* v = p->verts;

    p->plane = Plane(mverts[v[0]], mverts[v[1]], mverts[v[2]]);
  }
}

void TerrainClouds::Update()
{
  if (!nverts || !mverts || !verts)
    return;

  for (int i = 0; i < nverts; ++i)
    verts->loc[i] = mverts[i] + loc;
}

void TerrainClouds::Illuminate(Color ambient, List<Light>& lights)
{
  int i, n;

  if (terrain)
  {
    DWORD cloud_color = terrain->GetRegion()->CloudColor().Value() | Color(0, 0, 0, 255).Value();
    DWORD shade_color = terrain->GetRegion()->ShadeColor().Value() | Color(0, 0, 0, 255).Value();

    int stride = 4;
    if (type > 0)
      stride = 8;

    for (i = 0; i < nbanks; i++)
    {
      for (n = 0; n < 4; n++)
      {
        verts->diffuse[stride * i + n] = cloud_color;
        verts->specular[stride * i + n] = 0xff000000;
      }

      if (type > 0)
      {
        for (; n < 8; n++)
        {
          verts->diffuse[stride * i + n] = shade_color;
          verts->specular[stride * i + n] = 0xff000000;
        }
      }
    }
  }
}

void TerrainClouds::Render(Video* video, DWORD flags)
{
  if ((flags & RENDER_ALPHA) == 0)
    return;

  if (video && life && polys && npolys && verts)
  {
    if (scene)
      Illuminate(scene->Ambient(), scene->Lights());

    video->SetRenderState(Video::FOG_ENABLE, false);
    video->DrawPolys(npolys, polys);
  }
}
