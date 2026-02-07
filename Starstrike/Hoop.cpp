#include "pch.h"
#include "Hoop.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Game.h"

static Color ils_color;

Hoop::Hoop()
  : mtl(nullptr),
    width(360),
    height(180)
{
  foreground = true;
  radius = static_cast<float>(width);

  DataLoader* loader = DataLoader::GetLoader();

  loader->SetDataPath("HUD\\");
  loader->LoadTexture("ILS.pcx", hoop_texture, Bitmap::BMP_TRANSLUCENT);
  loader->SetDataPath();

  Hoop::CreatePolys();
}

Hoop::~Hoop() {}

void Hoop::SetColor(Color c) { ils_color = c; }

void Hoop::CreatePolys()
{
  auto mtl = NEW Material;

  mtl->tex_diffuse = hoop_texture;
  mtl->blend = Material::MTL_ADDITIVE;

  int w = width / 2;
  int h = height / 2;

  model = NEW Model;
  own_model = true;

  auto surface = NEW Surface;

  surface->SetName("hoop");
  surface->CreateVerts(4);
  surface->CreatePolys(2);

  VertexSet* vset = surface->GetVertexSet();
  Poly* polys = surface->GetPolys();

  ZeroMemory(polys, sizeof(Poly) * 2);

  for (int i = 0; i < 4; i++)
  {
    int x = w;
    int y = h;
    float u = 0;
    float v = 0;

    if (i == 0 || i == 3)
      x = -x;
    else
      u = 1;

    if (i < 2)
      y = -y;
    else
      v = 1;

    vset->loc[i] = Vec3(x, y, 0);
    vset->nrm[i] = Vec3(0, 0, 0);

    vset->tu[i] = u;
    vset->tv[i] = v;
  }

  for (int i = 0; i < 2; i++)
  {
    Poly& poly = polys[i];

    poly.nverts = 4;
    poly.vertex_set = vset;
    poly.material = mtl;

    poly.verts[0] = i ? 3 : 0;
    poly.verts[1] = i ? 2 : 1;
    poly.verts[2] = i ? 1 : 2;
    poly.verts[3] = i ? 0 : 3;

    poly.plane = Plane(vset->loc[poly.verts[0]], vset->loc[poly.verts[2]], vset->loc[poly.verts[1]]);

    surface->AddIndices(6);
  }

  // then assign them to cohesive segments:
  auto segment = NEW Segment;
  segment->npolys = 2;
  segment->polys = &polys[0];
  segment->material = segment->polys->material;

  surface->GetSegments().append(segment);

  model->AddSurface(surface);

  SetLuminous(true);
}

void Hoop::Update()
{
  if (mtl)
    mtl->Ke = ils_color;

  if (model && luminous)
  {
    ListIter<Surface> s_iter = model->GetSurfaces();
    while (++s_iter)
    {
      Surface* surface = s_iter.value();
      VertexSet* vset = surface->GetVertexSet();

      for (int i = 0; i < vset->nverts; i++)
        vset->diffuse[i] = ils_color.Value();
    }

    InvalidateSurfaceData();
  }
}
