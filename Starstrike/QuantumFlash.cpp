#include "pch.h"
#include "QuantumFlash.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Game.h"
#include "Light.h"
#include "Random.h"

static Bitmap* quantum_flash_texture = nullptr;

QuantumFlash::QuantumFlash()
  : length(8000),
    width(0),
    shade(1.0),
    npolys(16),
    nverts(64),
    mtl(nullptr),
    verts(nullptr),
    polys(nullptr),
    beams(nullptr),
    texture(quantum_flash_texture)
{
  trans = true;
  luminous = true;

  if (!texture || texture->Width() < 1)
  {
    DataLoader* loader = DataLoader::GetLoader();
    loader->SetDataPath("Explosions/");
    loader->LoadTexture("quantum.pcx", quantum_flash_texture, Bitmap::BMP_TRANSLUCENT);
    loader->SetDataPath();
    texture = quantum_flash_texture;
  }

  loc = Vec3(0.0f, 0.0f, 1000.0f);

  mtl = NEW Material;
  verts = NEW VertexSet(nverts);
  polys = NEW Poly[npolys];
  beams = NEW Matrix[npolys];

  mtl->Kd = Color::White;
  mtl->tex_diffuse = texture;
  mtl->tex_emissive = texture;
  mtl->blend = Video::BLEND_ADDITIVE;
  mtl->luminous = true;

  verts->nverts = nverts;

  for (int i = 0; i < npolys; i++)
  {
    verts->loc[4 * i + 0] = Point(width, 0, 1000);
    verts->loc[4 * i + 1] = Point(width, -length, 1000);
    verts->loc[4 * i + 2] = Point(-width, -length, 1000);
    verts->loc[4 * i + 3] = Point(-width, 0, 1000);

    for (int n = 0; n < 4; n++)
    {
      verts->diffuse[4 * i + n] = D3DCOLOR_RGBA(255, 255, 255, 255);
      verts->specular[4 * i + n] = D3DCOLOR_RGBA(0, 0, 0, 255);
      verts->tu[4 * i + n] = (n < 2) ? 0.0f : 1.0f;
      verts->tv[4 * i + n] = (n > 0 && n < 3) ? 1.0f : 0.0f;
    }

    beams[i].Roll(Random(-2 * PI, 2 * PI));
    beams[i].Pitch(Random(-2 * PI, 2 * PI));
    beams[i].Yaw(Random(-2 * PI, 2 * PI));

    polys[i].nverts = 4;
    polys[i].visible = 1;
    polys[i].sortval = 0;
    polys[i].vertex_set = verts;
    polys[i].material = mtl;

    polys[i].verts[0] = 4 * i + 0;
    polys[i].verts[1] = 4 * i + 1;
    polys[i].verts[2] = 4 * i + 2;
    polys[i].verts[3] = 4 * i + 3;
  }

  radius = static_cast<float>(length);
  length = 0;
  name = "QuantumFlash";
}

QuantumFlash::~QuantumFlash()
{
  delete mtl;
  delete verts;
  delete [] polys;
  delete [] beams;
}

void QuantumFlash::Render(Video* video, DWORD flags)
{
  if (hidden || !visible || !video || ((flags & RENDER_ADDITIVE) == 0))
    return;

  const Camera* cam = video->GetCamera();

  if (cam)
  {
    UpdateVerts(cam->Pos());
    video->DrawPolys(npolys, polys);
  }
}

void QuantumFlash::UpdateVerts(const Point& cam_pos)
{
  if (length < radius)
  {
    length += radius / 80;
    width += 1;
  }

  for (int i = 0; i < npolys; i++)
  {
    Matrix& m = beams[i];

    m.Yaw(0.05);
    auto vpn = Point(m(2, 0), m(2, 1), m(2, 2));

    Point head = loc;
    Point tail = loc - vpn * length;
    Point vtail = tail - head;
    Point vcam = cam_pos - loc;
    Point vtmp = vcam.cross(vtail);
    vtmp.Normalize();
    Point vlat = vtmp * -width;

    verts->loc[4 * i + 0] = head + vlat;
    verts->loc[4 * i + 1] = tail + vlat * 8.0f;
    verts->loc[4 * i + 2] = tail - vlat * 8.0f;
    verts->loc[4 * i + 3] = head - vlat;

    DWORD color = D3DCOLOR_RGBA(static_cast<BYTE>(255 * shade), static_cast<BYTE>(255 * shade),
                                static_cast<BYTE>(255 * shade), 255);

    for (int n = 0; n < 4; n++)
      verts->diffuse[4 * i + n] = color;
  }
}

void QuantumFlash::SetOrientation(const Matrix& o) { orientation = o; }

void QuantumFlash::SetShade(double s)
{
  if (s < 0)
    s = 0;
  else
    if (s > 1)
      s = 1;

  shade = s;
}
