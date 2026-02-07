#include "pch.h"
#include "Polygon.h"
#include "Bitmap.h"

VertexSet::VertexSet(int m)
  : nverts(0),
    space(OBJECT_SPACE),
    tu1(nullptr),
    tv1(nullptr),
    tangent(nullptr),
    binormal(nullptr) { Resize(m); }

VertexSet::~VertexSet() { Delete(); }

void VertexSet::Resize(int m, bool preserve)
{
  // easy cases (no data will be preserved):
  if (!m || !nverts || !preserve)
  {
    bool additional_tex_coords = (tu1 != nullptr);

    Delete();

    nverts = m;

    if (nverts <= 0)
      ZeroMemory(this, sizeof(VertexSet));

    else
    {
      loc = NEW Vec3[nverts];
      nrm = NEW Vec3[nverts];
      s_loc = NEW Vec3[nverts];
      tu = NEW float[nverts];
      tv = NEW float[nverts];
      rw = NEW float[nverts];
      diffuse = NEW DWORD[nverts];
      specular = NEW DWORD[nverts];

      if (additional_tex_coords)
        CreateAdditionalTexCoords();

      if (!loc || !nrm || !s_loc || !rw || !tu || !tv || !diffuse || !specular)
      {
        nverts = 0;

        delete [] loc;
        delete [] nrm;
        delete [] s_loc;
        delete [] rw;
        delete [] tu;
        delete [] tv;
        delete [] tu1;
        delete [] tv1;
        delete [] diffuse;
        delete [] specular;

        ZeroMemory(this, sizeof(VertexSet));
      }
    }
  }

  // actually need to copy data:
  else
  {
    int np = nverts;

    nverts = m;

    if (nverts < np)
      np = nverts;

    auto new_loc = NEW Vec3[nverts];
    auto new_nrm = NEW Vec3[nverts];
    auto new_s_loc = NEW Vec3[nverts];
    auto new_rw = NEW float[nverts];
    auto new_tu = NEW float[nverts];
    auto new_tv = NEW float[nverts];
    float* new_tu1 = nullptr;
    float* new_tv1 = nullptr;
    auto new_diffuse = NEW DWORD[nverts];
    auto new_specular = NEW DWORD[nverts];

    if (tu1)
      new_tu1 = NEW float[nverts];

    if (tv1)
      new_tv1 = NEW float[nverts];

    if (new_loc)
    {
      CopyMemory(new_loc, loc, np * sizeof(Vec3));
      delete [] loc;
      loc = new_loc;
    }

    if (new_nrm)
    {
      CopyMemory(new_nrm, nrm, np * sizeof(Vec3));
      delete [] nrm;
      nrm = new_nrm;
    }

    if (new_s_loc)
    {
      CopyMemory(new_s_loc, s_loc, np * sizeof(Vec3));
      delete [] s_loc;
      s_loc = new_s_loc;
    }

    if (new_tu)
    {
      CopyMemory(new_tu, tu, np * sizeof(float));
      delete [] tu;
      tu = new_tu;
    }

    if (new_tv)
    {
      CopyMemory(new_tv, tv, np * sizeof(float));
      delete [] tv;
      tv = new_tv;
    }

    if (new_tu1)
    {
      CopyMemory(new_tu1, tu1, np * sizeof(float));
      delete [] tu1;
      tu = new_tu1;
    }

    if (new_tv1)
    {
      CopyMemory(new_tv1, tv1, np * sizeof(float));
      delete [] tv1;
      tv = new_tv1;
    }

    if (new_diffuse)
    {
      CopyMemory(new_diffuse, diffuse, np * sizeof(DWORD));
      delete [] diffuse;
      diffuse = new_diffuse;
    }

    if (new_specular)
    {
      CopyMemory(new_specular, specular, np * sizeof(DWORD));
      delete [] specular;
      specular = new_specular;
    }

    if (!loc || !nrm || !s_loc || !rw || !tu || !tv || !diffuse || !specular)
    {
      Delete();
      ZeroMemory(this, sizeof(VertexSet));
    }
  }
}

void VertexSet::Delete()
{
  if (nverts)
  {
    delete [] loc;
    delete [] nrm;
    delete [] s_loc;
    delete [] rw;
    delete [] tu;
    delete [] tv;
    delete [] tu1;
    delete [] tv1;
    delete [] diffuse;
    delete [] specular;
    delete [] tangent;
    delete [] binormal;

    tangent = nullptr;
    binormal = nullptr;
  }
}

void VertexSet::Clear()
{
  if (nverts)
  {
    ZeroMemory(loc, sizeof(Vec3) * nverts);
    ZeroMemory(nrm, sizeof(Vec3) * nverts);
    ZeroMemory(s_loc, sizeof(Vec3) * nverts);
    ZeroMemory(tu, sizeof(float) * nverts);
    ZeroMemory(tv, sizeof(float) * nverts);
    ZeroMemory(rw, sizeof(float) * nverts);
    ZeroMemory(diffuse, sizeof(DWORD) * nverts);
    ZeroMemory(specular, sizeof(DWORD) * nverts);

    if (tu1)
      ZeroMemory(tu1, sizeof(float) * nverts);

    if (tv1)
      ZeroMemory(tv1, sizeof(float) * nverts);

    if (tangent)
      ZeroMemory(tangent, sizeof(Vec3) * nverts);

    if (binormal)
      ZeroMemory(binormal, sizeof(Vec3) * nverts);
  }
}

void VertexSet::CreateTangents()
{
  if (tangent)
    delete [] tangent;
  if (binormal)
    delete [] binormal;

  tangent = nullptr;
  binormal = nullptr;

  if (nverts)
  {
    tangent = NEW Vec3[nverts];
    binormal = NEW Vec3[nverts];
  }
}

void VertexSet::CreateAdditionalTexCoords()
{
  if (tu1)
    delete [] tu1;
  if (tv1)
    delete [] tv1;

  tu1 = nullptr;
  tv1 = nullptr;

  if (nverts)
  {
    tu1 = NEW float[nverts];
    tv1 = NEW float[nverts];
  }
}

bool VertexSet::CopyVertex(int dst, int src)
{
  if (src >= 0 && src < nverts && dst >= 0 && dst < nverts)
  {
    loc[dst] = loc[src];
    nrm[dst] = nrm[src];
    s_loc[dst] = s_loc[src];
    tu[dst] = tu[src];
    tv[dst] = tv[src];
    diffuse[dst] = diffuse[src];
    specular[dst] = specular[src];

    if (tu1)
      tu1[dst] = tu1[src];

    if (tv1)
      tv1[dst] = tv1[src];

    if (tangent)
      tangent[dst] = tangent[src];

    if (binormal)
      binormal[dst] = binormal[src];

    return true;
  }

  return false;
}

VertexSet* VertexSet::Clone() const
{
  auto result = NEW VertexSet(nverts);

  CopyMemory(result->loc, loc, nverts * sizeof(Vec3));
  CopyMemory(result->nrm, nrm, nverts * sizeof(Vec3));
  CopyMemory(result->s_loc, s_loc, nverts * sizeof(Vec3));
  CopyMemory(result->rw, rw, nverts * sizeof(float));
  CopyMemory(result->tu, tu, nverts * sizeof(float));
  CopyMemory(result->tv, tv, nverts * sizeof(float));
  CopyMemory(result->diffuse, diffuse, nverts * sizeof(DWORD));
  CopyMemory(result->specular, specular, nverts * sizeof(DWORD));

  if (tu1)
  {
    if (!result->tu1)
      result->tu1 = NEW float[nverts];

    CopyMemory(result->tu1, tu1, nverts * sizeof(float));
  }

  if (tv1)
  {
    if (!result->tv1)
      result->tv1 = NEW float[nverts];

    CopyMemory(result->tv1, tv1, nverts * sizeof(float));
  }

  if (tangent)
  {
    if (!result->tangent)
      result->tangent = NEW Vec3[nverts];

    CopyMemory(result->tangent, tangent, nverts * sizeof(Vec3));
  }

  if (binormal)
  {
    if (!result->binormal)
      result->binormal = NEW Vec3[nverts];

    CopyMemory(result->binormal, binormal, nverts * sizeof(Vec3));
  }

  return result;
}

void VertexSet::CalcExtents(Point& plus, Point& minus)
{
  plus = Point(-1e6, -1e6, -1e6);
  minus = Point(1e6, 1e6, 1e6);

  for (int i = 0; i < nverts; i++)
  {
    if (loc[i].x > plus.x)
      plus.x = loc[i].x;
    if (loc[i].x < minus.x)
      minus.x = loc[i].x;
    if (loc[i].y > plus.y)
      plus.y = loc[i].y;
    if (loc[i].y < minus.y)
      minus.y = loc[i].y;
    if (loc[i].z > plus.z)
      plus.z = loc[i].z;
    if (loc[i].z < minus.z)
      minus.z = loc[i].z;
  }
}

Poly::Poly(int init)
  : nverts(0),
    visible(1),
    vertex_set(nullptr),
    material(nullptr),
    sortval(0),
    flatness(0) {}

// Check to see if a test point is within the bounds of the poly.
// The point is assumed to be coplanar with the poly.  Return 1 if
// the point is inside, 0 if the point is outside.

Vec2 projverts[Poly::MAX_VERTS];

static inline double extent3(double a, double b, double c)
{
  double d1 = fabs(a - b);
  double d2 = fabs(a - c);
  double d3 = fabs(b - c);

  if (d1 > d2)
  {
    if (d1 > d3)
      return d1;
    return d3;
  }
  if (d2 > d3)
    return d2;
  return d3;
}

int Poly::Contains(const Vec3& pt) const
{
  // find largest 2d projection of this 3d Poly:
  int projaxis;

  double pnx = fabs(plane.normal.x);
  double pny = fabs(plane.normal.y);
  double pnz = fabs(plane.normal.z);

  if (pnx > pny)
  {
    if (pnx > pnz)
    {
      if (plane.normal.x > 0)
        projaxis = 1;
      else
        projaxis = -1;
    }
    else if (plane.normal.z > 0)
      projaxis = 3;
    else
      projaxis = -3;
  }
  else if (pny > pnz)
  {
    if (plane.normal.y > 0)
      projaxis = 2;
    else
      projaxis = -2;
  }
  else if (plane.normal.z > 0)
    projaxis = 3;
  else
    projaxis = -3;

  int i;

  for (i = 0; i < nverts; i++)
  {
    Vec3 loc = vertex_set->loc[verts[i]];
    switch (projaxis)
    {
      case 1:
        projverts[i] = Vec2(loc.y, loc.z);
        break;
      case -1:
        projverts[i] = Vec2(loc.z, loc.y);
        break;
      case 2:
        projverts[i] = Vec2(loc.z, loc.x);
        break;
      case -2:
        projverts[i] = Vec2(loc.x, loc.z);
        break;
      case 3:
        projverts[i] = Vec2(loc.x, loc.y);
        break;
      case -3:
        projverts[i] = Vec2(loc.y, loc.x);
        break;
    }
  }

  // now project the test point into the same plane:
  Vec2 test;
  switch (projaxis)
  {
    case 1:
      test.x = pt.y;
      test.y = pt.z;
      break;
    case -1:
      test.x = pt.z;
      test.y = pt.y;
      break;
    case 2:
      test.x = pt.z;
      test.y = pt.x;
      break;
    case -2:
      test.x = pt.x;
      test.y = pt.z;
      break;
    case 3:
      test.x = pt.x;
      test.y = pt.y;
      break;
    case -3:
      test.x = pt.y;
      test.y = pt.x;
      break;
  }

  constexpr float INSIDE_EPSILON = -0.01f;

  // if the test point is outside of any segment,
  // it is outside the entire convex Poly.
  for (i = 0; i < nverts - 1; i++)
  {
    if (verts[i] != verts[i + 1])
    {
      Vec2 segment = projverts[i + 1] - projverts[i];
      Vec2 segnorm = segment.normal();
      Vec2 tdelta = projverts[i] - test;
      float inside = segnorm * tdelta;
      if (inside < INSIDE_EPSILON)
        return 0;
    }
  }

  // check last segment, too:
  if (verts[0] != verts[nverts - 1])
  {
    Vec2 segment = projverts[0] - projverts[nverts - 1];
    float inside = segment.normal() * (projverts[0] - test);
    if (inside < INSIDE_EPSILON)
      return 0;
  }

  // still here? must be inside:
  return 1;
}

Material::Material()
  : power(1.0f),
    brilliance(1.0f),
    bump(0.0f),
    blend(MTL_SOLID),
    shadow(true),
    luminous(false),
    tex_diffuse(nullptr),
    tex_specular(nullptr),
    tex_bumpmap(nullptr),
    tex_emissive(nullptr),
    tex_alternate(nullptr),
    tex_detail(nullptr),
    thumbnail(nullptr)
{
  ambient_value = 0.2f;
  diffuse_value = 1.0f;
  specular_value = 0.0f;
  emissive_value = 0.0f;
}

Material::~Material()
{
  // these objects are owned by the shared
  // bitmap cache, so don't delete them now:
  tex_diffuse = nullptr;
  tex_specular = nullptr;
  tex_bumpmap = nullptr;
  tex_emissive = nullptr;
  tex_alternate = nullptr;
  tex_detail = nullptr;

  // the thumbnail is unique to the material,
  // so it is never cached:
  if (thumbnail)
    delete thumbnail;
}

int Material::operator ==(const Material& m) const
{
  if (this == &m)
    return 1;

  if (Ka != m.Ka)
    return 0;
  if (Kd != m.Kd)
    return 0;
  if (Ks != m.Ks)
    return 0;
  if (Ke != m.Ke)
    return 0;
  if (power != m.power)
    return 0;
  if (brilliance != m.brilliance)
    return 0;
  if (bump != m.bump)
    return 0;
  if (blend != m.blend)
    return 0;
  if (shadow != m.shadow)
    return 0;
  if (tex_diffuse != m.tex_diffuse)
    return 0;
  if (tex_specular != m.tex_specular)
    return 0;
  if (tex_bumpmap != m.tex_bumpmap)
    return 0;
  if (tex_emissive != m.tex_emissive)
    return 0;
  if (tex_alternate != m.tex_alternate)
    return 0;
  if (tex_detail != m.tex_detail)
    return 0;

  return name == m.name;
}

void Material::Clear()
{
  Ka = ColorValue();
  Kd = ColorValue();
  Ks = ColorValue();
  Ke = ColorValue();

  power = 1.0f;
  bump = 0.0f;
  blend = MTL_SOLID;
  shadow = true;

  tex_diffuse = nullptr;
  tex_specular = nullptr;
  tex_bumpmap = nullptr;
  tex_emissive = nullptr;
  tex_alternate = nullptr;
  tex_detail = nullptr;
}

static char shader_name[Material::NAMELEN];

const char* Material::GetShader(int pass) const
{
  int level = 0;
  if (pass > 1)
    pass--;

  for (int i = 0; i < NAMELEN; i++)
  {
    if (shader[i] == '/')
    {
      level++;

      if (level > pass)
        return nullptr;
    }

    else if (shader[i] != 0)
    {
      if (level == pass)
      {
        ZeroMemory(shader_name, NAMELEN);

        char* s = shader_name;
        while (i < NAMELEN && shader[i] != 0 && shader[i] != '/')
          *s++ = shader[i++];

        return shader_name;
      }
    }

    else
      return nullptr;
  }

  return nullptr;
}

void Material::CreateThumbnail(int size)
{
  if (!thumbnail)
    thumbnail = NEW Bitmap(size, size);

  if (!thumbnail || thumbnail->Width() != thumbnail->Height())
    return;

  size = thumbnail->Width();

  auto image = NEW DWORD[size * size];
  DWORD* dst = image;

  for (int j = 0; j < size; j++)
  {
    for (int i = 0; i < size; i++)
      *dst++ = GetThumbColor(i, j, size);
  }

  thumbnail->CopyHighColorImage(size, size, image, Bitmap::BMP_SOLID);
}

DWORD Material::GetThumbColor(int i, int j, int size)
{
  Color result = Color::LightGray;

  double x = i - size / 2;
  double y = j - size / 2;
  double r = 0.9 * size / 2;
  double d = sqrt(x * x + y * y);

  if (d <= r)
  {
    double z = sqrt(r * r - x * x - y * y);

    Point loc(x, y, z);
    Point nrm = loc;
    nrm.Normalize();
    Point light(1, -1, 1);
    light.Normalize();
    Point eye(0, 0, 1);

    ColorValue c = Ka * ColorValue(0.25f, 0.25f, 0.25f);  // ambient light
    ColorValue white(1, 1, 1);

    double diffuse = nrm * light;
    double v = 1 - (acos(nrm.y) / PI);
    double u = asin(nrm.x / sin(acos(nrm.y))) / PI + 0.5;

    ColorValue cd = Kd;
    ColorValue cs = Ks;
    ColorValue ce = Ke;

    if (tex_diffuse)
    {
      int tu = static_cast<int>(u * tex_diffuse->Width());
      int tv = static_cast<int>(v * tex_diffuse->Height());
      cd = Kd * tex_diffuse->GetColor(tu, tv);
    }

    if (tex_emissive)
    {
      int tu = static_cast<int>(u * tex_emissive->Width());
      int tv = static_cast<int>(v * tex_emissive->Height());
      ce = Ke * tex_emissive->GetColor(tu, tv);
    }

    if (tex_bumpmap && bump != 0 && nrm.z > 0)
    {
      // compute derivatives B(u,v)
      int tu = static_cast<int>(u * tex_bumpmap->Width());
      int tv = static_cast<int>(v * tex_bumpmap->Height());

      DWORD tmpred = tex_bumpmap->GetColor(tu, tv).Red();
      double du1 = tmpred - tex_bumpmap->GetColor(tu - 1, tv).Red();
      double du2 = tex_bumpmap->GetColor(tu + 1, tv).Red() - tmpred;

      double dv1 = tmpred - tex_bumpmap->GetColor(tu, tv - 1).Red();
      double dv2 = tex_bumpmap->GetColor(tu, tv + 1).Red() - tmpred;

      double du = (du1 + du2) / 512 * 1e-8;
      double dv = (dv1 + dv2) / 512 * 1e-8;

      if (du || dv)
      {
        Point Nu = nrm.cross(Point(0, -1, 0));
        Nu.Normalize();
        Point Nv = nrm.cross(Point(1, 0, 0));
        Nv.Normalize();

        nrm += (Nu * du * bump);
        nrm += (Nv * dv * bump);
        nrm.Normalize();

        diffuse = nrm * light;
        v = 1 - (acos(nrm.y) / PI);
        u = asin(nrm.x / sin(acos(nrm.y))) / PI + 0.5;
      }
    }

    if (tex_specular)
    {
      int tu = static_cast<int>(u * tex_specular->Width());
      int tv = static_cast<int>(v * tex_specular->Height());
      cs = Ks * tex_specular->GetColor(tu, tv);
    }

    // anisotropic diffuse lighting
    if (brilliance >= 0)
      diffuse = pow(diffuse, static_cast<double>(brilliance));

    // forward lighting
    if (diffuse > 0)
    {
      // diffuse
      c += cd * (white * diffuse);

      // specular
      if (power > 0)
      {
        double spec = ((nrm * 2.0f * (nrm * light) - light) * eye);
        if (spec > 0.01)
        {
          spec = pow(spec, static_cast<double>(power));
          c += cs * (white * spec);
        }
      }
    }

    // back lighting
    else
    {
      diffuse *= -0.5;
      c += cd * (white * diffuse);

      // specular
      if (power > 0)
      {
        light *= -1;

        double spec = ((nrm * 2.0f * (nrm * light) - light) * eye);
        if (spec > 0.01)
        {
          spec = pow(spec, static_cast<double>(power));
          c += cs * (white * spec) * 0.7;
        }
      }
    }

    c += ce;

    result = c.ToColor();
  }

  return result.Value();
}
