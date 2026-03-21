#pragma once

#include "building.h"

class Triffid : public Building
{
    friend class TriffidBuildingRenderer;
  protected:
    ShapeMarkerData* m_launchPoint;
    ShapeMarkerData* m_stem;
    float m_timerSync;
    float m_damage;
    bool m_triggered;
    float m_triggerTimer;
    bool m_renderDamaged;

  public:
    enum
    {
      SpawnVirii,
      SpawnCentipede,
      SpawnSpider,
      SpawnSpirits,
      SpawnEggs,
      SpawnTriffidEggs,
      SpawnDarwinians,
      NumSpawnTypes
    };

    bool m_spawn[NumSpawnTypes];
    float m_size;
    float m_reloadTime;
    float m_pitch;
    float m_force;
    float m_variance; // Horizontal

    int m_useTrigger; // Num enemies required to trigger
    LegacyVector3 m_triggerLocation; // Offset from m_pos
    float m_triggerRadius;

    Matrix34 GetHead(); // So to speak

    Triffid();

    void Initialise(Building* _template) override;
    bool Advance() override;
    void Launch();
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    void Damage(float _damage) override;

    bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen = 1e10, LegacyVector3* _pos = nullptr,
                    LegacyVector3* _norm = nullptr) override;

    static const char* GetSpawnName(int _spawnType);
    static const char* GetSpawnNameTranslated(int _spawnType);

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;
};

// ============================================================================
// Triffid Egg

#define TRIFFIDEGG_BOUNCEFRICTION 0.65f

class TriffidEggRenderer;

class TriffidEgg : public Entity
{
  friend class TriffidEggRenderer;

  protected:
    LegacyVector3 m_up;
    float m_force;
    float m_timerSync;
    float m_life;

  public:
    float m_size;
    int m_spawnType;
    LegacyVector3 m_spawnPoint;
    float m_spawnRange;

    TriffidEgg();

    void ChangeHealth(int _amount) override;
    void Spawn();
    bool Advance(Unit* _unit) override;
    void Render(float _predictionTime) override;
};
