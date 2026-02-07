#include "pch.h"
#include "Graphic.h"
#include "Projector.h"
#include "Scene.h"

int Graphic::id_key = 1;

Graphic::Graphic(const std::string_view _name)
  : id(id_key++),
    loc(0.0f, 0.0f, 0.0f),
    depth(0.0f),
    radius(0.0f),
    life(-1),
    visible(true),
    infinite(false),
    foreground(false),
    background(false),
    hidden(false),
    trans(false),
    shadow(false),
    luminous(false),
    scene(nullptr),
    name(_name.empty() ? "Graphic" : _name)
{
  screen_rect.x = 0;
  screen_rect.y = 0;
  screen_rect.w = 0;
  screen_rect.h = 0;
}

Graphic::~Graphic() {}

int Graphic::operator <(const Graphic& g) const
{
  if (!infinite && g.infinite)
    return 1;
  if (infinite && !g.infinite)
    return 0;

  double za = fabs(Depth());
  double zb = fabs(g.Depth());

  return (za < zb);
}

int Graphic::operator <=(const Graphic& g) const
{
  if (!infinite && g.infinite)
    return 1;
  if (infinite && !g.infinite)
    return 0;

  double za = fabs(Depth());
  double zb = fabs(g.Depth());

  return (za <= zb);
}

void Graphic::SetInfinite(bool b)
{
  infinite = static_cast<BYTE>(b);

  if (infinite)
    depth = 1.0e9f;
}

int Graphic::Nearer(Graphic* a, Graphic* b)
{
  if (a->depth < b->depth)
    return -1;
  if (a->depth == b->depth)
    return 0;
  return 1;
}

int Graphic::Farther(Graphic* a, Graphic* b)
{
  if (a->depth > b->depth)
    return -1;
  if (a->depth == b->depth)
    return 0;
  return 1;
}

void Graphic::Destroy()
{
  if (scene)
    scene->DelGraphic(this);

  delete this;
}

int Graphic::CollidesWith(Graphic& o)
{
  Point delta_loc = loc - o.loc;

  // bounding spheres test:
  if (delta_loc.length() > radius + o.radius)
    return 0;

  return 1;
}

int Graphic::CheckRayIntersection(Point Q, Point w, double len, Point& ipt, bool treat_translucent_polys_as_solid)
{
  return 0;
}

void Graphic::ProjectScreenRect(Projector* p)
{
  screen_rect.x = 2000;
  screen_rect.y = 2000;
  screen_rect.w = 0;
  screen_rect.h = 0;
}

bool Graphic::CheckVisibility(Projector& projector)
{
  if (projector.IsVisible(Location(), Radius()) && projector.ApparentRadius(Location(), Radius()) > 1)
    visible = true;
  else
  {
    visible = false;
    screen_rect.x = 2000;
    screen_rect.y = 2000;
    screen_rect.w = 0;
    screen_rect.h = 0;
  }

  return visible;
}
