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
    bool IsInView() override;

    void Activate();
    void DeActivate();
    void SpawnSpam(bool _isResearch);

    void TriggerSpam();
};
