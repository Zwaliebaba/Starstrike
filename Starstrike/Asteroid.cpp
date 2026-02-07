#include "pch.h"
#include "Asteroid.h"
#include "DataLoader.h"
#include "Sim.h"
#include "Solid.h"

static auto asteroid_velocity = Point(0, 0, 0);
static Model* asteroid_model[32];

Asteroid::Asteroid(int t, const Vec3& pos, double m)
  : Debris(asteroid_model[t % 6], pos, asteroid_velocity, m) { life = -1; }

void Asteroid::Initialize()
{
  ZeroMemory(asteroid_model, sizeof(asteroid_model));

  DataLoader* loader = DataLoader::GetLoader();
  auto old_path = loader->GetDataPath();
  loader->SetDataPath("Galaxy\\Asteroids\\");

  int n = 0;

  auto a = NEW Model;
  if (a)
  {
    a->Load("a1.mag", 100);
    asteroid_model[n++] = a;
  }

  a = NEW Model;
  if (a)
  {
    a->Load("a2.mag", 50);
    asteroid_model[n++] = a;
  }

  a = NEW Model;
  if (a)
  {
    a->Load("a1.mag", 8);
    asteroid_model[n++] = a;
  }

  a = NEW Model;
  if (a)
  {
    a->Load("a2.mag", 10);
    asteroid_model[n++] = a;
  }

  a = NEW Model;
  if (a)
  {
    a->Load("a3.mag", 30);
    asteroid_model[n++] = a;
  }

  a = NEW Model;
  if (a)
  {
    a->Load("a4.mag", 20);
    asteroid_model[n++] = a;
  }

  loader->SetDataPath(old_path);
}

void Asteroid::Close()
{
  for (int i = 0; i < 32; i++)
    delete asteroid_model[i];

  ZeroMemory(asteroid_model, sizeof(asteroid_model));
}
