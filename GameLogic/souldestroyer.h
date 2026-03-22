#pragma once

#include "entity.h"
#include "fast_darray.h"

#define SOULDESTROYER_MINSEARCHRANGE       200.0f
#define SOULDESTROYER_MAXSEARCHRANGE       300.0f
#define SOULDESTROYER_DAMAGERANGE          25.0f
#define SOULDESTROYER_MAXSPIRITS           50

class ShapeStatic;

class SoulDestroyer : public Entity
{
    friend class SoulDestroyerRenderer;

  protected:
    LegacyVector3 m_targetPos;
    LegacyVector3 m_up;
    WorldObjectId m_targetEntity;
    LList<LegacyVector3> m_positionHistory;
    FastDArray<float> m_spirits;

    float m_retargetTimer;
    float m_panic;

    static ShapeStatic* s_shapeHead;
    static ShapeStatic* s_shapeTail;
    static ShapeMarkerData* s_tailMarker;

    LegacyVector3 m_spiritPosition[SOULDESTROYER_MAXSPIRITS];

    bool SearchForRandomPosition();
    bool SearchForTargetEnemy();
    bool SearchForRetreatPosition();

    bool AdvanceToTargetPosition();
    void RecordHistoryPosition();
    bool GetTrailPosition(LegacyVector3& _pos, LegacyVector3& _vel);

    void Panic(float _time);

  public:
    SoulDestroyer();

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;

    void Attack(const LegacyVector3& _pos) override;

    void SetWaypoint(LegacyVector3 _waypoint) override;
};

class Zombie : public WorldObject
{
  public:
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    float m_life;

    LegacyVector3 m_hover;
    float m_positionOffset; // Used to make them float around a bit
    float m_xaxisRate;
    float m_yaxisRate;
    float m_zaxisRate;

    Zombie();

    bool Advance() override;
};
