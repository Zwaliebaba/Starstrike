#include "pch.h"
#include "file_writer.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "hi_res_time.h"
#include "math_utils.h"
#include "text_stream_readers.h"
#include "profiler.h"
#include "language_table.h"
#include "GameSimEventQueue.h"
#include "triffid.h"
#include "GameContext.h"
#include "location.h"
#include "team.h"
#include "main.h"
#include "entity_grid.h"

Triffid::Triffid()
  : Building(),
    m_launchPoint(nullptr),
    m_stem(nullptr),
    m_timerSync(0.0f),
    m_damage(50.0f),
    m_triggered(false),
    m_triggerTimer(0.0f),
    m_size(1.0f),
    m_reloadTime(60.0f),
    m_pitch(0.6f),
    m_force(250.0f),
    m_variance(M_PI / 8.0f),
    m_useTrigger(0),
    m_triggerRadius(100.0f)
{
  m_type = TypeTriffid;

  SetShape(Resource::GetShapeStatic("triffidhead.shp"));

  m_launchPoint = m_shape->GetMarkerData("MarkerLaunchPoint");
  m_stem = m_shape->GetMarkerData("MarkerTriffidStem");

  DEBUG_ASSERT(m_launchPoint);
  DEBUG_ASSERT(m_stem);

  m_triggerTimer = syncfrand(5.0f);

  memset(m_spawn, 0, NumSpawnTypes * sizeof(bool));
}

Matrix34 Triffid::GetHead()
{
  LegacyVector3 _pos = m_pos + g_upVector * m_size * 30.0f;
  LegacyVector3 _front = m_front;
  LegacyVector3 _up = g_upVector;

  float timer = g_gameTime + m_id.GetUniqueId() * 10.0f;

  if (m_damage > 0.0f)
  {
    _front.RotateAroundY(sinf(timer) * m_variance * 0.5f);
    LegacyVector3 right = _front ^ _up;

    float pitchVariation = 1.0f + sinf(timer) * 0.1f;
    _front.RotateAround(right * m_pitch * 0.5f * pitchVariation);

    _pos += LegacyVector3(sinf(timer / 1.3f) * 4, sinf(timer / 1.5f), sinf(timer / 1.2f) * 3);

    float justFiredFraction = (m_timerSync - GetHighResTime()) / m_reloadTime;
    justFiredFraction = std::min(justFiredFraction, 1.0f);
    justFiredFraction = std::max(justFiredFraction, 0.0f);
    justFiredFraction = pow(justFiredFraction, 5);

    _pos -= _front * justFiredFraction * 10.0f;
  }
  else
  {
    // We are on fire, so thrash around a lot
    LegacyVector3 right = _front ^ _up;
    _front = g_upVector;
    _up = right ^ _front;

    _front.RotateAround(right * sinf(timer * 5.0f) * m_variance);
    _up.RotateAroundY(sinf(timer * 3.5f) * m_variance);

    _pos += LegacyVector3(sinf(timer * 3.3f) * 8, sinf(timer * 4.5f), sinf(timer * 3.2f) * 6);
  }

  Matrix34 result(_front, _up, _pos);

  float size = m_size * (1.0f + sinf(timer) * 0.1f);

  result.f *= size;
  result.u *= size;
  result.r *= size;

  return result;
}

bool Triffid::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, [[maybe_unused]] LegacyVector3* _pos,
                         [[maybe_unused]] LegacyVector3* _norm)
{
  Matrix34 mat = GetHead();

  RayPackage ray(_rayStart, _rayDir, _rayLen);

  return m_shape->RayHit(&ray, mat, true);
}

void Triffid::Damage(float _damage)
{
  Building::Damage(_damage);

  bool dead = (m_damage <= 0.0f);

  m_damage += _damage;

  if (m_damage <= 0.0f && !dead)
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "Burn"));
}

void Triffid::Launch()
{
  //
  // Determine what sort of egg to launch

  LList<int> possibleSpawns;
  for (int i = 0; i < NumSpawnTypes; ++i)
    if (m_spawn[i])
      possibleSpawns.PutData(i);

  if (possibleSpawns.Size() == 0)
  {
    // Can't spawn anything
    return;
  }

  int spawnIndex = syncrand() % possibleSpawns.Size();
  int spawnType = possibleSpawns[spawnIndex];

  //
  // Fire the egg

  Matrix34 mat = GetHead();
  Matrix34 launchMat = m_shape->GetMarkerWorldMatrix(m_launchPoint, mat);
  LegacyVector3 velocity = launchMat.f;
  velocity.SetLength(m_force * m_size * (1.0f + syncsfrand(0.2f)));

  WorldObjectId wobjId = g_context->m_location->SpawnEntities(launchMat.pos, m_id.GetTeamId(), -1, Entity::TypeTriffidEgg, 1, velocity, 0.0f);
  auto triffidEgg = static_cast<TriffidEgg*>(g_context->m_location->GetEntitySafe(wobjId, Entity::TypeTriffidEgg));
  if (triffidEgg)
  {
    triffidEgg->m_spawnType = spawnType;
    triffidEgg->m_size = 1.0f + syncsfrand(0.3f);
    triffidEgg->m_spawnPoint = m_pos + m_triggerLocation;
    triffidEgg->m_roamRange = m_triggerRadius;
  }

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "LaunchEgg"));
}

void Triffid::Initialise(Building* _template)
{
  Building::Initialise(_template);

  auto triffid = static_cast<Triffid*>(_template);

  m_size = triffid->m_size;
  m_pitch = triffid->m_pitch;
  m_force = triffid->m_force;
  m_variance = triffid->m_variance;
  m_reloadTime = triffid->m_reloadTime;

  m_useTrigger = triffid->m_useTrigger;
  m_triggerLocation.x = triffid->m_triggerLocation.x;
  m_triggerLocation.z = triffid->m_triggerLocation.z;
  m_triggerRadius = triffid->m_triggerRadius;

  m_triggerLocation.y = g_context->m_location->m_landscape.m_heightMap->GetValue(m_triggerLocation.x, m_triggerLocation.z);

  memcpy(m_spawn, triffid->m_spawn, NumSpawnTypes * sizeof(bool));

  float timeAdd = syncrand() % static_cast<int>(m_reloadTime);
  timeAdd += 10.0f;
  m_timerSync = GetHighResTime() + timeAdd;
}

bool Triffid::Advance()
{
  //
  // Burn if we have been damaged

  if (m_damage <= 0.0f)
  {
    Matrix34 headMat = GetHead();
    LegacyVector3 fireSpawn = headMat.pos;
    fireSpawn += LegacyVector3(sfrand(10.0f * m_size), sfrand(10.0f * m_size), sfrand(10.0f * m_size));
    float fireSize = 100.0f + sfrand(100.0f * m_size);
    g_simEventQueue.Push(SimEvent::MakeParticle(fireSpawn, g_zeroVector, SimParticle::TypeFire, fireSize));

    fireSpawn = m_pos + LegacyVector3(sfrand(10.0f * m_size), sfrand(10.0f * m_size), sfrand(10.0f * m_size));
    g_simEventQueue.Push(SimEvent::MakeParticle(fireSpawn, g_zeroVector, SimParticle::TypeFire, fireSize));

    if (frand(100.0f) < 10.0f)
      g_simEventQueue.Push(SimEvent::MakeParticle(fireSpawn, g_zeroVector, SimParticle::TypeExplosionDebris));

    m_size -= 0.006f;
    if (m_size <= 0.3f)
    {
      g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, headMat, 1.0f));
      return true;
    }
    m_timerSync -= 0.5f;
  }

  //
  // Look in our trigger area if required

  bool oldTriggered = m_triggered;

  if (m_useTrigger == 0 || m_damage <= 0.0f)
    m_triggered = true;

  if (m_useTrigger > 0 && GetHighResTime() > m_triggerTimer)
  {
    START_PROFILE(g_context->m_profiler, "CheckTrigger");
    LegacyVector3 triggerPos = m_pos + m_triggerLocation;
    bool enemiesFound = g_context->m_location->m_entityGrid->AreEnemiesPresent(triggerPos.x, triggerPos.z, m_triggerRadius, m_id.GetTeamId());
    m_triggered = enemiesFound;
    m_triggerTimer = GetHighResTime() + 5.0f;
    END_PROFILE(g_context->m_profiler, "CheckTrigger");
  }

  //
  // If we have just triggered, start our random timer

  if (!oldTriggered && m_triggered)
  {
    float timeAdd = syncrand() % static_cast<int>(m_reloadTime);
    timeAdd += 10.0f;
    m_timerSync = GetHighResTime() + timeAdd;
  }

  //
  // Launch an egg if it is time

  if (m_triggered && GetHighResTime() > m_timerSync)
  {
    Launch();
    m_timerSync = GetHighResTime() + m_reloadTime * (1.0f + syncsfrand(0.1f));
  }

  //
  // Flicker if we are damaged

  float healthFraction = m_damage / 50.0f;
  float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
  m_renderDamaged = (frand(0.75f) * (1.0f - fabs(sinf(timeIndex)) * 1.2f) > healthFraction);

  return false;
}

void Triffid::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_size = atof(_in->GetNextToken());
  m_pitch = atof(_in->GetNextToken());
  m_force = atof(_in->GetNextToken());
  m_variance = atof(_in->GetNextToken());
  m_reloadTime = atof(_in->GetNextToken());
  m_useTrigger = atoi(_in->GetNextToken());

  m_triggerLocation.x = atof(_in->GetNextToken());
  m_triggerLocation.z = atof(_in->GetNextToken());
  m_triggerRadius = atof(_in->GetNextToken());

  for (int i = 0; i < NumSpawnTypes; ++i)
    m_spawn[i] = (atoi(_in->GetNextToken()) == 1);
}

void Triffid::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-6.2f %-6.2f %-6.2f %-6.2f %-6.2f ", m_size, m_pitch, m_force, m_variance, m_reloadTime);
  _out->printf("%d %-8.2f %-8.2f %-6.2f ", m_useTrigger, m_triggerLocation.x, m_triggerLocation.z, m_triggerRadius);

  for (int i = 0; i < NumSpawnTypes; ++i)
  {
    if (m_spawn[i])
      _out->printf("1 ");
    else
      _out->printf("0 ");
  }
}

const char* Triffid::GetSpawnName(int _spawnType)
{
  static const char* names[NumSpawnTypes] = {
    "SpawnVirii", "SpawnCentipedes", "SpawnSpider", "SpawnSpirits", "SpawnEggs", "SpawnTriffidEggs", "SpawnDarwinians"
  };

  return names[_spawnType];
}

const char* Triffid::GetSpawnNameTranslated(int _spawnType)
{
  const char* spawnName = GetSpawnName(_spawnType);

  char stringId[256];
  snprintf(stringId, sizeof(stringId), "spawnname_%s", spawnName);

  if (ISLANGUAGEPHRASE(stringId))
    return LANGUAGEPHRASE(stringId);
  return spawnName;
}

// ============================================================================

TriffidEgg::TriffidEgg()
  : Entity(),
    m_force(1.0f),
    m_size(1.0f),
    m_spawnType(Triffid::SpawnVirii)
{
  SetType(TypeTriffidEgg);

  m_up = g_upVector;
  m_shape = Resource::GetShapeStatic("triffidegg.shp");

  m_life = 20.0f + syncfrand(10.0f);
  m_timerSync = GetHighResTime() + m_life;
}

void TriffidEgg::ChangeHealth(int _amount)
{
  bool dead = m_dead;
  Entity::ChangeHealth(_amount);

  if (m_dead && !dead)
  {
    // We just died
    Matrix34 transform(m_front, m_up, m_pos);
    transform.f *= m_size;
    transform.u *= m_size;
    transform.r *= m_size;
    g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, transform, 1.0f));
  }
}

void TriffidEgg::Spawn()
{
  int teamId = m_id.GetTeamId();

  switch (m_spawnType)
  {
  case Triffid::SpawnVirii:
    {
      int numVirii = 5 + syncrand() % 5;
      g_context->m_location->SpawnEntities(m_pos, teamId, -1, TypeVirii, numVirii, g_zeroVector, 0.0f, 100.0f);
      break;
    }

  case Triffid::SpawnCentipede:
    {
      int size = 5 + syncrand() % 5;
      Team* team = &g_context->m_location->m_teams[m_id.GetTeamId()];
      int unitId;
      team->NewUnit(TypeCentipede, size, &unitId, m_pos);
      g_context->m_location->SpawnEntities(m_pos, teamId, unitId, TypeCentipede, size, g_zeroVector, 0.0f, 200.0f);
      break;
    }

  case Triffid::SpawnSpider:
    {
      WorldObjectId id = g_context->m_location->SpawnEntities(m_pos, teamId, -1, TypeSpider, 1, g_zeroVector, 0.0f, 150.0f);
      break;
    }

  case Triffid::SpawnSpirits:
    {
      int numSpirits = 5 + syncrand() % 5;
      for (int i = 0; i < numSpirits; ++i)
      {
        LegacyVector3 vel(syncsfrand(), 0.0f, syncsfrand());
        vel.SetLength(20.0f + syncfrand(20.0f));
        g_context->m_location->SpawnSpirit(m_pos, vel, teamId, WorldObjectId());
      }
      break;
    }

  case Triffid::SpawnEggs:
    {
      int numEggs = 5 + syncrand() % 5;
      for (int i = 0; i < numEggs; ++i)
      {
        LegacyVector3 vel = g_upVector + LegacyVector3(syncsfrand(), 0.0f, syncsfrand());
        vel.SetLength(20.0f + syncfrand(20.0f));
        g_context->m_location->SpawnEntities(m_pos, teamId, -1, TypeEgg, 1, vel, 0.0f, 0.0f);
      }
      break;
    }

  case Triffid::SpawnTriffidEggs:
    {
      int numEggs = 2 + syncrand() % 3;
      for (int i = 0; i < numEggs; ++i)
      {
        LegacyVector3 vel(syncsfrand(), 0.5f + syncfrand(), syncsfrand());
        vel.SetLength(75.0f + syncfrand(50.0f));
        WorldObjectId id = g_context->m_location->SpawnEntities(m_pos, teamId, -1, TypeTriffidEgg, 1, vel, 0.0f, 0.0f);
        auto egg = static_cast<TriffidEgg*>(g_context->m_location->GetEntitySafe(id, TypeTriffidEgg));
        if (egg)
          egg->m_spawnType = syncrand() % (Triffid::NumSpawnTypes - 1);
        // The NumSpawnTypes-1 prevents Darwinians from coming out
      }
      break;
    }

  case Triffid::SpawnDarwinians:
    {
      int numDarwinians = 10 + syncrand() % 10;
      for (int i = 0; i < numDarwinians; ++i)
      {
        LegacyVector3 vel = g_upVector + LegacyVector3(syncsfrand(), 0.0f, syncsfrand());
        vel.SetLength(10.0f + syncfrand(20.0f));
        WorldObjectId id = g_context->m_location->SpawnEntities(m_pos, teamId, -1, TypeDarwinian, 1, vel, 0.0f, 0.0f);
        Entity* entity = g_context->m_location->GetEntity(id);
        entity->m_front.y = 0.0f;
        entity->m_front.Normalise();
        entity->m_onGround = false;
      }
      break;
    }
  }
}

bool TriffidEgg::Advance(Unit* _unit)
{
  if (m_dead)
    return true;

  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  // Fly through the air, bounce
  m_vel.y -= 9.8f * m_force;
  LegacyVector3 direction = m_vel;
  LegacyVector3 right = (g_upVector ^ direction).Normalise();
  m_up.RotateAround(right * SERVER_ADVANCE_PERIOD * m_force * m_force * 30.0f);
  m_front = right ^ m_up;
  m_up.Normalise();
  m_front.Normalise();

  float landHeight = g_context->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  if (m_pos.y < landHeight + 3.0f)
  {
    BounceOffLandscape();
    m_force *= TRIFFIDEGG_BOUNCEFRICTION;
    m_vel *= m_force;
    if (m_pos.y < landHeight + 3.0f)
      m_pos.y = landHeight + 3.0f;
    if (m_force > 0.1f)
      g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "Bounce"));
  }

  // Self right ourselves

  if (m_up.y < 0.3f && m_force < 0.4f)
  {
    m_up = m_up * 0.95f + g_upVector * 0.05f;
    LegacyVector3 selfRight = m_up ^ m_front;
    m_front = selfRight ^ m_up;
    m_up.Normalise();
    m_front.Normalise();
  }

  //
  // Break open if it is time

  if (GetHighResTime() > m_timerSync)
  {
    Matrix34 transform(m_front, m_up, m_pos);
    g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, transform, 1.0f));
    Spawn();
    g_simEventQueue.Push(SimEvent::MakeSoundEntity(m_id, "BurstOpen"));
    return true;
  }

  return Entity::Advance(_unit);
}
