#pragma once

#include "LegacyVector3.h"
#include "matrix34.h"
#include "rgb_colour.h"
#include "worldobject.h"

class ShapeStatic;

// ---------------------------------------------------------------------------
// Particle type identifiers for SimEvent::ParticleSpawn.
// Values must match Particle::Type enum in particle_system.h.
// ---------------------------------------------------------------------------

namespace SimParticle
{
  constexpr int TypeRocketTrail = 0;
  constexpr int TypeExplosionCore = 1;
  constexpr int TypeExplosionDebris = 2;
  constexpr int TypeMuzzleFlash = 3;
  constexpr int TypeFire = 4;
  constexpr int TypeControlFlash = 5;
  constexpr int TypeSpark = 6;
  constexpr int TypeBlueSpark = 7;
  constexpr int TypeBrass = 8;
  constexpr int TypeMissileTrail = 9;
  constexpr int TypeMissileFire = 10;
  constexpr int TypeDarwinianFire = 11;
  constexpr int TypeLeaf = 12;
}

// ---------------------------------------------------------------------------
// Sound source type identifiers for SimEvent::SoundOtherEvent.
// Values must match SoundSourceBlueprint enum in soundsystem.h.
// ---------------------------------------------------------------------------

namespace SimSoundSource
{
  constexpr int TypeLaser = 0;
  constexpr int TypeGrenade = 1;
  constexpr int TypeRocket = 2;
  constexpr int TypeAirstrikeBomb = 3;
  constexpr int TypeSpirit = 4;
}

// ---------------------------------------------------------------------------
// SimEvent — deferred side-effect produced by simulation code.
//
// Simulation methods (Advance, Damage, ChangeHealth, ...) push events into
// the global SimEventQueue instead of calling rendering / audio APIs
// directly.  The client drains the queue each frame; the server discards it.
// ---------------------------------------------------------------------------

struct SimEvent
{
  enum Type
  {
    ParticleSpawn,
    Explosion,
    SoundStop,
    SoundEntityEvent,
    SoundBuildingEvent,
    SoundOtherEvent,
  };

  Type type;

  // Spatial data
  LegacyVector3 pos;
  LegacyVector3 vel;

  // Object identification (sound events, Explosion)
  WorldObjectId objectId;
  int objectType; // Building::Type or Entity::Type

  // ParticleSpawn fields
  int particleType; // SimParticle::Type*
  float particleSize; // -1.0f = use particle-type default
  RGBAColour particleColour; // RGBAColour(0) = use particle-type default

  // Explosion fields
  const ShapeStatic* shape; // non-owning; resource-manager lifetime
  Matrix34 transform; // world transform at time of explosion
  float fraction; // 0.0-1.0, fraction of shape to explode

  // Sound event fields
  const char* eventName; // must point to a string literal
  int soundSourceType; // SimSoundSource::Type* (SoundOtherEvent only)

  // ---------------------------------------------------------------------------
  // Factory helpers — return a zero-initialised event with the relevant fields.
  // ---------------------------------------------------------------------------

  [[nodiscard]] static SimEvent MakeParticle(const LegacyVector3& _pos, const LegacyVector3& _vel, int _particleType, float _size = -1.0f,
                                             RGBAColour _colour = RGBAColour(0))
  {
    SimEvent e = {};
    e.type = ParticleSpawn;
    e.pos = _pos;
    e.vel = _vel;
    e.particleType = _particleType;
    e.particleSize = _size;
    e.particleColour = _colour;
    return e;
  }

  [[nodiscard]] static SimEvent MakeExplosion(const ShapeStatic* _shape, const Matrix34& _transform, float _fraction)
  {
    SimEvent e = {};
    e.type = Explosion;
    e.shape = _shape;
    e.transform = _transform;
    e.fraction = _fraction;
    return e;
  }

  [[nodiscard]] static SimEvent MakeSoundStop(WorldObjectId _objectId, const char* _eventName = nullptr)
  {
    SimEvent e = {};
    e.type      = SoundStop;
    e.objectId  = _objectId;
    e.eventName = _eventName;
    return e;
  }

  [[nodiscard]] static SimEvent MakeSoundEntity(WorldObjectId _objectId, int _objectType, const LegacyVector3& _pos,
                                                const LegacyVector3& _vel, const char* _eventName)
  {
    SimEvent e = {};
    e.type = SoundEntityEvent;
    e.objectId = _objectId;
    e.objectType = _objectType;
    e.pos = _pos;
    e.vel = _vel;
    e.eventName = _eventName;
    return e;
  }

  [[nodiscard]] static SimEvent MakeSoundBuilding(WorldObjectId _objectId, int _objectType, const char* _eventName)
  {
    SimEvent e = {};
    e.type = SoundBuildingEvent;
    e.objectId = _objectId;
    e.objectType = _objectType;
    e.eventName = _eventName;
    return e;
  }

  [[nodiscard]] static SimEvent MakeSoundOther(const LegacyVector3& _pos, WorldObjectId _objectId, int _soundSourceType,
                                               const char* _eventName)
  {
    SimEvent e = {};
    e.type = SoundOtherEvent;
    e.pos = _pos;
    e.objectId = _objectId;
    e.soundSourceType = _soundSourceType;
    e.eventName = _eventName;
    return e;
  }
};
