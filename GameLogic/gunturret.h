#pragma once

#include "building.h"

#define GUNTURRET_RETARGETTIMER     3.0f
#define GUNTURRET_MINRANGE          100.0f
#define GUNTURRET_MAXRANGE          300.0f
#define GUNTURRET_NUMSTATUSMARKERS  5
#define GUNTURRET_NUMBARRELS        4
#define GUNTURRET_OWNERSHIPTIMER    1.0f

class GunTurret : public Building
{
    friend class GunTurretBuildingRenderer;

  protected:
    ShapeStatic* m_turret;
    ShapeStatic* m_barrel;
    ShapeMarkerData* m_barrelMount;
    ShapeMarkerData* m_barrelEnd[GUNTURRET_NUMBARRELS];
    ShapeMarkerData* m_statusMarkers[GUNTURRET_NUMSTATUSMARKERS];

    LegacyVector3 m_turretFront;
    LegacyVector3 m_barrelUp;

    bool m_aiTargetCreated;

    LegacyVector3 m_target;
    WorldObjectId m_targetId;
    float m_fireTimer;
    int m_nextBarrel;
    float m_retargetTimer;
    bool m_targetCreated;

    float m_health;
    float m_ownershipTimer;

    void PrimaryFire();
    bool SearchForTargets();
    void SearchForRandomPos();
    void RecalculateOwnership();

  public:
    GunTurret();

    void Initialise(Building* _template) override;
    void SetDetail(int _detail) override;

    void ExplodeBody();
    void Damage(float _damage) override;
    bool Advance() override;

    LegacyVector3 GetTarget();

    bool IsInView() override;

    bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                    LegacyVector3* norm) override;
};

class GunTurretTarget : public WorldObject
{
  public:
    int m_buildingId;

    GunTurretTarget(int _buildingId);
    bool Advance() override;
};
