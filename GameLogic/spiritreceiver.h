#pragma once

#include "building.h"

class SpiritProcessor;
class UnprocessedSpirit;

// ****************************************************************************
// Class ReceiverBuilding
// ****************************************************************************

class ReceiverBuilding : public Building
{
  protected:
    int m_spiritLink;
    ShapeMarkerData* m_spiritLocation;

    LList<float> m_spirits;

  public:
    ReceiverBuilding();

    void Initialise(Building* _template) override;
    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool IsInView() override;
    virtual LegacyVector3 GetSpiritLocation();
    virtual void TriggerSpirit(float _initValue);

    static SpiritProcessor* GetSpiritProcessor();

    static void BeginRenderUnprocessedSpirits();
    static void RenderUnprocessedSpirit(const LegacyVector3& _pos, float _life = 1.0f); // gl friendly
    static void RenderUnprocessedSpirit_basic(const LegacyVector3& _pos, float _life = 1.0f); // dx friendly
    static void RenderUnprocessedSpirit_detail(const LegacyVector3& _pos, float _life = 1.0f); // dx friendly
    static void EndRenderUnprocessedSpirits();

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;
};

// ****************************************************************************
// Class SpiritProcessor
// ****************************************************************************

class SpiritProcessor : public ReceiverBuilding
{
  protected:
    float m_timerSync;
    int m_numThisSecond;
    float m_spawnSync;
    float m_throughput;

  public:
    LList<UnprocessedSpirit*> m_floatingSpirits;

    SpiritProcessor();

    void TriggerSpirit(float _initValue) override;

    const char* GetObjectiveCounter() override;

    void Initialise(Building* _building) override;
    bool Advance() override;
    bool IsInView() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;
};

// ****************************************************************************
// Class ReceiverLink
// ****************************************************************************

class ReceiverLink : public ReceiverBuilding
{
  public:
    ReceiverLink();
    bool Advance() override;
};

// ****************************************************************************
// Class ReceiverSpiritSpawner
// ****************************************************************************

class ReceiverSpiritSpawner : public ReceiverBuilding
{
  public:
    ReceiverSpiritSpawner();
    bool Advance() override;
};

// ****************************************************************************
// Class SpiritReceiver
// ****************************************************************************

#define SPIRITRECEIVER_NUMSTATUSMARKERS 5

class SpiritReceiver : public ReceiverBuilding
{
  protected:
    ShapeMarkerData* m_headMarker;
    ShapeStatic* m_headShape;
    ShapeMarkerData* m_spiritLink;
    ShapeMarkerData* m_statusMarkers[SPIRITRECEIVER_NUMSTATUSMARKERS];

  public:
    SpiritReceiver();

    LegacyVector3 GetSpiritLocation() override;

    void Initialise(Building* _template) override;
    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderPorts() override;
    void RenderAlphas(float _predictionTime) override;
};

// ****************************************************************************
// Class UnprocessedSpirit
// ****************************************************************************

class UnprocessedSpirit : public WorldObject
{
  protected:
    float m_timeSync;
    float m_positionOffset; // Used to make them float around a bit
    float m_xaxisRate;
    float m_yaxisRate;
    float m_zaxisRate;

  public:
    LegacyVector3 m_hover;

    enum
    {
      StateUnprocessedFalling,
      StateUnprocessedFloating,
      StateUnprocessedDeath
    };

    int m_state;

    UnprocessedSpirit();

    bool Advance() override;
    float GetLife(); // Returns 0.0f-1.0f (0.0f=dead, 1.0f=alive)
};
