#pragma once

#include "building.h"

class GodDish : public Building
{
  public:
    bool m_activated;
    float m_timer;
    int m_numSpawned;
    bool m_spawnSpam;

    GodDish();

    void Initialise(Building* _template) override;

    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool IsInView() override;

    void Activate();
    void DeActivate();
    void SpawnSpam(bool _isResearch);

    void TriggerSpam();
};
