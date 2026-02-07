#include "pch.h"
#include "ShipSolid.h"
#include "Game.h"
#include "Ship.h"
#include "Sim.h"
#include "Skin.h"
#include "StarSystem.h"
#include "TerrainRegion.h"

ShipSolid::ShipSolid(Ship* s)
  : ship(s),
    skin(nullptr),
    in_soup(false) {}

ShipSolid::~ShipSolid() {}

void ShipSolid::TranslateBy(const Point& ref)
{
  true_eye_point = ref;
  Solid::TranslateBy(ref);
}

void ShipSolid::Render(Video* video, DWORD flags)
{
  if (hidden || !visible || !video || Depth() > 5e6)
    return;

  const Skin* s = nullptr;

  if (ship)
    s = ship->GetSkin();
  else
    s = skin;

  if (s)
    s->ApplyTo(model);

  bool fog = false;

  if (ship && ship->IsAirborne())
  {
    fog = true;

    auto rgn = static_cast<TerrainRegion*>(ship->GetRegion()->GetOrbitalRegion());
    double visibility = rgn->GetWeather().Visibility();
    FLOAT fog_density = static_cast<FLOAT>(rgn->FogDensity() * 2.5e-5 * 1 / visibility);
    Color fog_color = rgn->FogColor();

    // Use BLACK fog on secondary lighting pass
    // This will effectively "filter out" the highlights
    // with distance...

    if (flags & RENDER_ADD_LIGHT)
      fog_color = Color::Black;

    video->SetRenderState(Video::FOG_ENABLE, true);
    video->SetRenderState(Video::FOG_COLOR, fog_color.Value());
    video->SetRenderState(Video::FOG_DENSITY, *((DWORD*)&fog_density));
  }

  if (!fog)
    video->SetRenderState(Video::FOG_ENABLE, false);

  Solid::Render(video, flags);

  if (fog)
    video->SetRenderState(Video::FOG_ENABLE, false);

  if (s)
    s->Restore(model);
}
