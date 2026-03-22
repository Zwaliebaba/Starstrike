#pragma once

#include "GameVector3.h"
#include "GameMatrix.h"
#include "rgb_colour.h"
#include "worldobject.h"

#include <variant>

class ShapeStatic;

// ---------------------------------------------------------------------------
// Particle type identifiers for GameSimEvent::ParticleSpawn.
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
// Sound source type identifiers for GameSimEvent::SoundOtherEvent.
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
// GameSimEvent — deferred side-effect produced by simulation code.
//
// Simulation methods (Advance, Damage, ChangeHealth, ...) push events into
// the global SimEventQueue instead of calling rendering / audio APIs
// directly.  The client drains the queue each frame; the server discards it.
//
// Each event type is a separate aggregate struct held in a std::variant.
// The compiler enforces exhaustive handling via std::visit.
// ---------------------------------------------------------------------------

struct GameSimEvent
{
  // --- Variant alternatives -----------------------------------------------

  struct ParticleSpawn
  {
    Neuron::Math::GameVector3 pos;
    Neuron::Math::GameVector3 vel;
    int        particleType;
    float      particleSize;       // -1.0f = use particle-type default
    RGBAColour particleColour;     // RGBAColour(0) = use particle-type default
  };

  struct Explosion
  {
    const ShapeStatic*        shape;     // non-owning; resource-manager lifetime
    Neuron::Math::GameMatrix  transform; // world transform at time of explosion
    float                     fraction;  // 0.0–1.0
  };

  struct SoundStop
  {
    WorldObjectId objectId;
    const char*   eventName;   // nullptr = stop all
  };

  struct SoundEntityEvent
  {
    WorldObjectId objectId;
    const char*   eventName;
  };

  struct SoundBuildingEvent
  {
    WorldObjectId objectId;
    const char*   eventName;
  };

  struct SoundOtherEvent
  {
    Neuron::Math::GameVector3 pos;
    WorldObjectId objectId;
    int           soundSourceType; // SimSoundSource::Type*
    const char*   eventName;
  };

  using Variant = std::variant<
    ParticleSpawn, Explosion, SoundStop,
    SoundEntityEvent, SoundBuildingEvent, SoundOtherEvent>;

  Variant data;

  // --- Visitor convenience -------------------------------------------------

  template<typename Visitor>
  decltype(auto) Visit(Visitor&& _vis) const
  {
    return std::visit(std::forward<Visitor>(_vis), data);
  }

  // --- Factory helpers (preserve producer call-site compatibility) ----------

  [[nodiscard]] static GameSimEvent MakeParticle(const Neuron::Math::GameVector3& _pos, const Neuron::Math::GameVector3& _vel, int _particleType,
                                                  float _size = -1.0f, RGBAColour _colour = RGBAColour(0))
  {
    return { .data = ParticleSpawn{ _pos, _vel, _particleType, _size, _colour } };
  }

  [[nodiscard]] static GameSimEvent MakeExplosion(const ShapeStatic* _shape, const Neuron::Math::GameMatrix& _transform, float _fraction)
  {
    return { .data = Explosion{ _shape, _transform, _fraction } };
  }

  [[nodiscard]] static GameSimEvent MakeSoundStop(WorldObjectId _objectId, const char* _eventName = nullptr)
  {
    return { .data = SoundStop{ _objectId, _eventName } };
  }

  [[nodiscard]] static GameSimEvent MakeSoundEntity(WorldObjectId _objectId, const char* _eventName)
  {
    return { .data = SoundEntityEvent{ _objectId, _eventName } };
  }

  [[nodiscard]] static GameSimEvent MakeSoundBuilding(WorldObjectId _objectId, const char* _eventName)
  {
    return { .data = SoundBuildingEvent{ _objectId, _eventName } };
  }

  [[nodiscard]] static GameSimEvent MakeSoundOther(const Neuron::Math::GameVector3& _pos, WorldObjectId _objectId, int _soundSourceType,
                                                    const char* _eventName)
  {
    return { .data = SoundOtherEvent{ _pos, _objectId, _soundSourceType, _eventName } };
  }
};

// Note: std::variant on MSVC is not trivially copyable even when all
// alternatives are trivially copyable (MSVC STL implementation detail).
// The queue still works correctly — variant is copyable, just not *trivially* so.

static_assert(sizeof(GameSimEvent) <= 88, "GameSimEvent variant exceeds 88-byte target");

// Back-compat alias so call sites can still write SimEvent::MakeParticle(...)
using SimEvent = GameSimEvent;
