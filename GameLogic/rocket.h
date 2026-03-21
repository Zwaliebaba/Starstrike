#pragma once

#include "building.h"

class FuelBuilding : public Building
{
    friend class FuelBuildingRenderer;

  protected:
    ShapeMarkerData* m_fuelMarker;
    static ShapeStatic* s_fuelPipe;

  public:
    int m_fuelLink;
    float m_currentLevel;

    FuelBuilding();

    void Initialise(Building* _template) override;

    virtual void ProvideFuel(float _level);

    LegacyVector3 GetFuelPosition();

    FuelBuilding* GetLinkedBuilding();

    bool Advance() override;

    bool IsInView() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    void Destroy(float _intensity) override;
};

// ============================================================================

class FuelGenerator : public FuelBuilding
{
    friend class FuelGeneratorBuildingRenderer;

  protected:
    ShapeStatic* m_pump;
    ShapeMarkerData* m_pumpTip;
    float m_pumpMovement;
    float m_previousPumpPos;

    LegacyVector3 GetPumpPos();

  public:
    float m_surges;

    FuelGenerator();

    void ProvideSurge();

    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    const char* GetObjectiveCounter() override;
};

// ============================================================================

class FuelPipe : public FuelBuilding
{
  public:
    FuelPipe();

    bool Advance() override;
};

// ============================================================================

class FuelStation : public FuelBuilding
{
  protected:
    ShapeMarkerData* m_entrance;

  public:
    FuelStation();

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    LegacyVector3 GetEntrance();

    bool Advance() override;
    bool IsLoading();
    bool BoardRocket(WorldObjectId id);

    bool PerformDepthSort(LegacyVector3& _centerPos) override;
};

// ============================================================================

class EscapeRocket : public FuelBuilding
{
    friend class EscapeRocketBuildingRenderer;

  protected:
    ShapeMarkerData* m_booster;
    ShapeMarkerData* m_window[3];
    ShapeStatic* m_rocketLowRes;
    float m_shadowTimer;
    float m_cameraShake;

    LegacyVector3 m_vel;

  public:
    enum
    {
      StateRefueling,
      StateLoading,
      StateIgnition,
      StateReady,
      StateCountdown,
      StateExploding,
      StateFlight,
      NumStates
    };

    int m_state;
    float m_fuel;
    int m_pipeCount;
    int m_passengers;
    float m_countdown;
    float m_damage;
    int m_spawnBuildingId;
    bool m_spawnCompleted;

  protected:
    void Refuel();
    void SetupSpectacle();
    void SetupAttackers();

    void AdvanceRefueling();
    void AdvanceLoading();
    void AdvanceIgnition();
    void AdvanceReady();
    void AdvanceCountdown();
    void AdvanceFlight();
    void AdvanceExploding();

    void SetupSounds();

  public:
    EscapeRocket();

    void Initialise(Building* _template) override;
    bool Advance() override;

    void ProvideFuel(float _level) override;
    bool SafeToLaunch();
    bool BoardRocket(WorldObjectId _id);
    void Damage(float _damage) override;

    bool IsSpectacle();
    bool IsInView() override;

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;

    const char* GetObjectiveCounter() override;

    static int GetStateId(const char* _state);
};
