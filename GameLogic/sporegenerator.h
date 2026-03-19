#pragma once

#include "entity.h"

#define SPOREGENERATOR_NUMTAILS 4

class SporeGenerator : public Entity
{
  public:
    float m_retargetTimer;
    float m_eggTimer;
    LegacyVector3 m_targetPos;
    int m_spiritId;

  protected:
    bool SearchForRandomPos();
    bool SearchForSpirits();

    bool AdvanceToTargetPosition();
    bool AdvanceIdle();
    bool AdvanceEggLaying();
    bool AdvancePanic();

    void RenderTail(const LegacyVector3& _from, const LegacyVector3& _to, float _size);

    ShapeMarkerData* m_eggMarker;
    ShapeMarkerData* m_tail[SPOREGENERATOR_NUMTAILS];

    enum
    {
      StateIdle,
      StateEggLaying,
      StatePanic
    };

    int m_state;

  public:
    SporeGenerator();

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;

    bool IsInView() override;
    void Render(float _predictionTime) override;
};
