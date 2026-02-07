#include "pch.h"
#include "Bolt.h"
#include "Bitmap.h"
#include "Camera.h"
#include "Video.h"

Bolt::Bolt(double len, double wid, Bitmap* tex, int share)
  : length(len),
    width(wid),
    shade(1.0),
    poly(0),
    vset(4),
    texture(tex),
    shared(share),
    vpn(0, 1, 0)
{
  trans = true;

  loc = Vec3(0.0f, 0.0f, 1000.0f);

  vset.nverts = 4;

  vset.loc[0] = Point(width, 0, 1000);
  vset.loc[1] = Point(width, -length, 1000);
  vset.loc[2] = Point(-width, -length, 1000);
  vset.loc[3] = Point(-width, 0, 1000);

  vset.tu[0] = 0.0f;
  vset.tv[0] = 0.0f;
  vset.tu[1] = 1.0f;
  vset.tv[1] = 0.0f;
  vset.tu[2] = 1.0f;
  vset.tv[2] = 1.0f;
  vset.tu[3] = 0.0f;
  vset.tv[3] = 1.0f;

  Plane plane(vset.loc[0], vset.loc[1], vset.loc[2]);

  for (int i = 0; i < 4; i++)
    vset.nrm[i] = plane.normal;

  mtl.Ka = Color::White;
  mtl.Kd = Color::White;
  mtl.Ks = Color::Black;
  mtl.Ke = Color::White;
  mtl.tex_diffuse = texture;
  mtl.tex_emissive = texture;
  mtl.blend = Video::BLEND_ADDITIVE;

  poly.nverts = 4;
  poly.vertex_set = &vset;
  poly.material = &mtl;
  poly.verts[0] = 0;
  poly.verts[1] = 1;
  poly.verts[2] = 2;
  poly.verts[3] = 3;

  radius = static_cast<float>((length > width) ? (length) : (width * 2));

  if (texture)
  {
    name = texture->GetFilename();
  }
}

Bolt::~Bolt() {}

void Bolt::Render(Video* video, DWORD flags)
{
  if ((flags & RENDER_ADDITIVE) == 0)
    return;

  if (visible && !hidden && video && life)
  {
    const Camera* camera = video->GetCamera();

    Point head = loc;
    Point tail = origin;
    Point vtail = tail - head;
    Point vcam = camera->Pos() - loc;
    Point vtmp = vcam.cross(vtail);
    vtmp.Normalize();
    Point vlat = vtmp * -width;
    Vec3 vnrm = camera->vpn() * -1.0;

    vset.loc[0] = head + vlat;
    vset.loc[1] = tail + vlat;
    vset.loc[2] = tail - vlat;
    vset.loc[3] = head - vlat;

    vset.nrm[0] = vnrm;
    vset.nrm[1] = vnrm;
    vset.nrm[2] = vnrm;
    vset.nrm[3] = vnrm;

    ColorValue white(static_cast<float>(shade), static_cast<float>(shade), static_cast<float>(shade));
    mtl.Ka = white;
    mtl.Kd = white;
    mtl.Ks = Color::Black;
    mtl.Ke = white;

    video->DrawPolys(1, &poly);
  }
}

void Bolt::Update() {}

void Bolt::TranslateBy(const Point& ref)
{
  loc = loc - ref;
  origin = origin - ref;
}

void Bolt::SetOrientation(const Matrix& o)
{
  vpn = Point(o(2, 0), o(2, 1), o(2, 2));
  origin = loc + (vpn * -length);
}

void Bolt::SetDirection(const Point& v)
{
  vpn = v;
  origin = loc + (vpn * -length);
}

void Bolt::SetEndPoints(const Point& from, const Point& to)
{
  loc = to;
  origin = from;
  vpn = to - from;
  length = vpn.Normalize();
  radius = static_cast<float>(length);
}

void Bolt::SetTextureOffset(double from, double to)
{
  vset.tu[0] = static_cast<float>(from);
  vset.tu[1] = static_cast<float>(to);
  vset.tu[2] = static_cast<float>(to);
  vset.tu[3] = static_cast<float>(from);
}
