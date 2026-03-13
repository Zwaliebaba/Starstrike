#pragma once

#include "building.h"

class FileWriter;

// ****************************************************************************
// Class PowerBuilding
// ****************************************************************************

class PowerBuilding : public Building
{
  protected:
    int m_powerLink;
    ShapeMarker* m_powerLocation;

    LList<float> m_surges;

  public:
    PowerBuilding();

    void Initialise(Building* _template) override;
    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool IsInView() override;
    LegacyVector3 GetPowerLocation();
    virtual void TriggerSurge(float _initValue);

    void ListSoundEvents(LList<char*>* _list) override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;
};

// ****************************************************************************
// Class Generator
// ****************************************************************************

class Generator : public PowerBuilding
{
  protected:
    ShapeMarker* m_counter;

    float m_timerSync;
    int m_numThisSecond;
    bool m_enabled;

  public:
    float m_throughput;

    Generator();

    void TriggerSurge(float _initValue) override;

    void ReprogramComplete() override;

    char* GetObjectiveCounter() override;

    void ListSoundEvents(LList<char*>* _list) override;

    bool Advance() override;
    void Render(float _predictionTime) override;
};

// ****************************************************************************
// Class Pylon
// ****************************************************************************

class Pylon : public PowerBuilding
{
  public:
    Pylon();
    bool Advance() override;
};

// ****************************************************************************
// Class PylonStart
// ****************************************************************************

class PylonStart : public PowerBuilding
{
  public:
    int m_reqBuildingId;

    PylonStart();

    void Initialise(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;
};

// ****************************************************************************
// Class PylonEnd
// ****************************************************************************

class PylonEnd : public PowerBuilding
{
  public:
    PylonEnd();

    void TriggerSurge(float _initValue) override;
    void RenderAlphas(float _predictionTime) override;
};

// ****************************************************************************
// Class SolarPanel
// ****************************************************************************

#define SOLARPANEL_NUMGLOWS 4
#define SOLARPANEL_NUMSTATUSMARKERS 5

class SolarPanel : public PowerBuilding
{
  protected:
    ShapeMarker* m_glowMarker[SOLARPANEL_NUMGLOWS];
    ShapeMarker* m_statusMarkers[SOLARPANEL_NUMSTATUSMARKERS];

    bool m_operating;

  public:
    SolarPanel();

    void Initialise(Building* _template) override;
    bool Advance() override;

    void Render(float _predictionTime) override;
    void RenderPorts() override;
    void RenderAlphas(float _predictionTime) override;

    void ListSoundEvents(LList<char*>* _list) override;
};

