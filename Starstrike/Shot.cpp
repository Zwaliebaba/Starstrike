#include "pch.h"
#include "Shot.h"
#include "AudioConfig.h"
#include "Bitmap.h"
#include "Bolt.h"
#include "DriveSprite.h"
#include "Game.h"
#include "Light.h"
#include "Random.h"
#include "SeekerAI.h"
#include "Ship.h"
#include "Sim.h"
#include "Solid.h"
#include "Sound.h"
#include "Sprite.h"
#include "Terrain.h"
#include "Trail.h"
#include "Weapon.h"

Shot::Shot(const Point& pos, const Camera& shot_cam, WeaponDesign* dsn, const Ship* ship)
  : owner(ship),
    charge(1.0f),
    offset(1.0e5f),
    altitude_agl(-1.0e6f),
    eta(0),
    first_frame(true),
    hit_target(false),
    flash(nullptr),
    flare(nullptr),
    trail(nullptr),
    sound(nullptr),
    design(dsn)
{
  obj_type = SIM_SHOT;
  type = design->type;
  primary = design->primary;
  beam = design->beam;
  base_damage = design->damage;
  armed = false;

  radius = 10.0f;

  if (primary || design->decoy_type || !design->guided)
  {
    straight = true;
    armed = true;
  }

  cam.Clone(shot_cam);

  life = design->life;
  velocity = cam.vpn() * static_cast<double>(design->speed);

  MoveTo(pos);

  if (beam)
    origin = pos + (shot_cam.vpn() * -design->length);

  switch (design->graphic_type)
  {
    case Graphic::BOLT:
    {
      auto s = NEW Bolt(design->length, design->width, design->shot_img, 1);
      s->SetDirection(cam.vpn());
      rep = s;
    }
    break;

    case Graphic::SPRITE:
    {
      Sprite* s = nullptr;

      if (design->animation)
        s = NEW DriveSprite(design->animation, design->anim_length);
      else
        s = NEW DriveSprite(design->shot_img);

      s->Scale(design->scale);
      rep = s;
    }
    break;

    case Graphic::SOLID:
    {
      auto s = NEW Solid;
      s->UseModel(design->shot_model);
      rep = s;

      radius = rep->Radius();
    }
    break;
  }

  if (rep)
    rep->MoveTo(pos);

  light = nullptr;

  if (design->light > 0)
  {
    light = NEW Light(design->light);
    light->SetColor(design->light_color);
  }

  mass = design->mass;
  drag = design->drag;
  thrust = 0.0f;

  dr_drg = design->roll_drag;
  dp_drg = design->pitch_drag;
  dy_drg = design->yaw_drag;

  SetAngularRates(design->roll_rate, design->pitch_rate, design->yaw_rate);

  if (design->flash_img != nullptr)
  {
    flash = NEW Sprite(design->flash_img);
    flash->Scale(design->flash_scale);
    flash->MoveTo(pos - cam.vpn() * design->length);
    flash->SetLuminous(true);
  }

  if (design->flare_img != nullptr)
  {
    flare = NEW DriveSprite(design->flare_img);
    flare->Scale(design->flare_scale);
    flare->MoveTo(pos);
  }

  if (owner)
  {
    iff_code = static_cast<BYTE>(owner->GetIFF());
    Observe((SimObject*)owner);
  }

  m_name = std::format("Shot({})", design->name);
}

Shot::~Shot()
{
  GRAPHIC_DESTROY(flash);
  GRAPHIC_DESTROY(flare);
  GRAPHIC_DESTROY(trail);

  if (sound)
  {
    sound->Stop();
    sound->Release();
  }
}

std::string Shot::DesignName() const { return design->name; }

void Shot::SetCharge(float c)
{
  charge = c;

  // trim beam life to amount of energy available:
  if (beam)
    life = design->life * charge / design->charge;
}

void Shot::SetFuse(double seconds)
{
  if (seconds > 0 && !beam)
    life = seconds;
}

void Shot::SeekTarget(SimObject* target, System* sub)
{
  if (dir && !primary)
  {
    auto seeker = static_cast<SeekerAI*>(dir);
    SimObject* old_target = seeker->GetTarget();

    if (old_target->Type() == SIM_SHIP)
    {
      auto tgt_ship = static_cast<Ship*>(old_target);
      tgt_ship->DropThreat(this);
    }
  }

  delete dir;
  dir = nullptr;

  if (target)
  {
    auto seeker = NEW SeekerAI(this);
    seeker->SetTarget(target, sub);
    seeker->SetPursuit(design->guided);
    seeker->SetDelay(1);

    dir = seeker;

    if (!primary && target->Type() == SIM_SHIP)
    {
      auto tgt_ship = static_cast<Ship*>(target);
      tgt_ship->AddThreat(this);
    }
  }
}

bool Shot::IsTracking(Ship* tgt) const { return tgt && (GetTarget() == tgt); }

SimObject* Shot::GetTarget() const
{
  if (dir)
  {
    auto seeker = static_cast<SeekerAI*>(dir);

    if (seeker->GetDelay() <= 0)
      return seeker->GetTarget();
  }

  return nullptr;
}

bool Shot::IsFlak() const { return design && design->flak; }

bool Shot::IsHostileTo(const SimObject* o) const
{
  if (o)
  {
    if (o->Type() == SIM_SHIP)
    {
      auto s = (Ship*)o;

      if (s->IsRogue())
        return true;

      if (s->GetIFF() > 0 && s->GetIFF() != GetIFF())
        return true;
    }

    else if (o->Type() == SIM_SHOT || o->Type() == SIM_DRONE)
    {
      auto s = (Shot*)o;

      if (s->GetIFF() > 0 && s->GetIFF() != GetIFF())
        return true;
    }
  }

  return false;
}

void Shot::ExecFrame(double seconds)
{
  altitude_agl = -1.0e6f;

  // add random flickering effect:
  double flicker = 0.75 + static_cast<double>(rand()) / 8e4;
  if (flicker > 1)
    flicker = 1;

  if (flare)
    flare->SetShade(flicker);
  else if (beam)
  {
    auto blob = static_cast<Bolt*>(rep);
    blob->SetShade(flicker);
    offset -= static_cast<float>(seconds * 10);
  }

  if (Game::Paused())
    return;

  if (beam)
  {
    if (!first_frame)
    {
      if (life > 0)
      {
        life -= seconds;

        if (life < 0)
          life = 0;
      }
    }
  }
  else
  {
    origin = Location();

    if (!first_frame)
      Physical::ExecFrame(seconds);
    else
      Physical::ExecFrame(0);

    double len = design->length;
    if (len < 50)
      len = 50;

    if (!trail && life > 0 && design->life - life > 0.2)
    {
      if (design->trail.length())
      {
        trail = NEW Trail(design->trail_img, design->trail_length);

        if (design->trail_width > 0)
          trail->SetWidth(design->trail_width);

        if (design->trail_dim > 0)
          trail->SetDim(design->trail_dim);

        trail->AddPoint(Location() + Heading() * -100.0f);

        Scene* scene = nullptr;

        if (rep)
          scene = rep->GetScene();

        if (scene)
          scene->AddGraphic(trail);
      }
    }

    if (trail)
      trail->AddPoint(Location());

    if (!armed)
    {
      auto seeker = static_cast<SeekerAI*>(dir);

      if (seeker && seeker->GetDelay() <= 0)
        armed = true;
    }

    // handle submunitions:
    else if (design->det_range > 0 && design->det_count > 0)
    {
      if (dir && !primary)
      {
        auto seeker = static_cast<SeekerAI*>(dir);
        SimObject* target = seeker->GetTarget();

        if (target)
        {
          double range = Point(Location() - target->Location()).length();

          if (range < design->det_range)
          {
            life = 0;

            Sim* sim = Sim::GetSim();
            WeaponDesign* child_design = WeaponDesign::Find(design->det_child);

            if (sim && child_design)
            {
              double spread = design->det_spread;

              Camera aim_cam;
              aim_cam.Clone(Cam());
              aim_cam.LookAt(target->Location());

              for (int i = 0; i < design->det_count; i++)
              {
                Shot* child = sim->CreateShot(Location(), aim_cam, child_design, owner, owner->GetRegion());

                child->SetCharge(child_design->charge);

                if (child_design->guided)
                  child->SeekTarget(target, seeker->GetSubTarget());

                if (child_design->beam)
                  child->SetBeamPoints(Location(), target->Location());

                if (i)
                  aim_cam.LookAt(target->Location());
                aim_cam.Pitch(Random(-spread, spread));
                aim_cam.Yaw(Random(-spread, spread));
              }
            }
          }
        }
      }
    }

    if (flash && !first_frame)
      GRAPHIC_DESTROY(flash);

    if (thrust < design->thrust)
      thrust += static_cast<float>(seconds * 5.0e3);
    else
      thrust = design->thrust;
  }

  first_frame = false;

  if (flare)
    flare->MoveTo(Location());
}

void Shot::Disarm()
{
  if (armed && !primary)
  {
    armed = false;
    delete dir;
    dir = nullptr;
  }
}

void Shot::Destroy() { life = 0; }

void Shot::SetBeamPoints(const Point& from, const Point& to)
{
  if (beam)
  {
    MoveTo(to);
    origin = from;

    if (sound)
      sound->SetLocation(from);

    if (rep)
    {
      auto s = static_cast<Bolt*>(rep);
      s->SetEndPoints(from, to);

      double len = Point(to - from).length() / 500;
      s->SetTextureOffset(offset, offset + len);
    }
  }

  if (flash)
    flash->MoveTo(origin);
}

double Shot::AltitudeMSL() const { return Location().y; }

double Shot::AltitudeAGL() const
{
  if (altitude_agl < -1000)
  {
    auto pThis = (Shot*)this; // cast-away const
    Point loc = Location();
    Terrain* terrain = region->GetTerrain();

    if (terrain)
      pThis->altitude_agl = static_cast<float>(loc.y - terrain->Height(loc.x, loc.z));

    else
      pThis->altitude_agl = static_cast<float>(loc.y);

    if (!_finite(altitude_agl))
      pThis->altitude_agl = 0.0f;
  }

  return altitude_agl;
}

void Shot::Initialize() {}

void Shot::Close() {}

double Shot::Damage() const
{
  double damage = 0;

  // beam damage based on length:
  if (beam)
  {
    double fade = 1;

    if (design)
    {
      // linear fade with distance:
      double len = Point(origin - Location()).length();

      if (len > design->min_range)
        fade = (design->length - len) / (design->length - design->min_range);
    }

    damage = base_damage * charge * fade * Game::FrameTime();
  }

  // energy wep damage based on time:
  else if (primary)
    damage = base_damage * charge * life;

    // missile damage is constant:
  else
    damage = base_damage * charge;

  return damage;
}

double Shot::Length() const
{
  if (design)
    return design->length;

  return 500;
}

void Shot::Activate(Scene& scene)
{
  SimObject::Activate(scene);

  if (trail)
    scene.AddGraphic(trail);

  if (flash)
    scene.AddGraphic(flash);

  if (flare)
    scene.AddGraphic(flare);

  if (first_frame)
  {
    if (design->sound_resource)
    {
      sound = design->sound_resource->Duplicate();

      if (sound)
      {
        long max_vol = AudioConfig::EfxVolume();
        long volume = -1000;

        if (volume > max_vol)
          volume = max_vol;

        if (beam)
        {
          sound->SetLocation(origin);
          sound->SetVolume(volume);
          sound->Play();
        }
        else
        {
          sound->SetLocation(Location());
          sound->SetVolume(volume);
          sound->Play();
          sound = nullptr;  // fire and forget:
        }
      }
    }
  }
}

void Shot::Deactivate(Scene& scene)
{
  SimObject::Deactivate(scene);

  if (trail)
    scene.DelGraphic(trail);

  if (flash)
    scene.DelGraphic(flash);

  if (flare)
    scene.DelGraphic(flare);
}

int Shot::GetIFF() const { return iff_code; }

Color Shot::MarkerColor() const { return Ship::IFFColor(GetIFF()); }

std::string Shot::GetObserverName() const { return m_name; }

bool Shot::Update(SimObject* obj)
{
  if (obj == (SimObject*)owner)
    owner = nullptr;

  return SimObserver::Update(obj);
}
