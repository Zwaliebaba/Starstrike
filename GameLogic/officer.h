#pragma once

#include "entity.h"
#include "flag.h"

#define OFFICER_ATTACKRANGE     10.0f
#define OFFICER_ABSORBRANGE     10.0f

class Officer : public Entity
{
  public:
    enum
    {
      StateIdle,
      StateToWaypoint,
      StateGivingOrders
    };

    enum
    {
      OrderNone,
      OrderGoto,
      OrderFollow,
      NumOrderTypes
    };

    int m_state;
    LegacyVector3 m_wayPoint;
    int m_wayPointTeleportId; // Id of teleport we wish to walk into

    int m_shield;
    bool m_demoted;
    bool m_absorb;
    float m_absorbTimer;

    int m_orders;
    LegacyVector3 m_orderPosition; // Position in the world
    int m_ordersBuildingId; // Id of target building eg Teleport

    ShapeMarkerData* m_flagMarker;
    Flag m_flag;

  protected:
    bool AdvanceIdle();
    bool AdvanceToWaypoint();
    bool AdvanceGivingOrders();
    bool AdvanceToTargetPosition();

    bool SearchForRandomPosition();

    void Absorb();

    void RenderFlag(float _predictionTime);
    void RenderShield(float _predictionTime);
    void RenderSpirit(const LegacyVector3& _pos);

  public:
    Officer();
    ~Officer() override;

    void Begin() override;
    void Render(float _predictionTime) override;

    bool Advance(Unit* _unit) override;

    void ChangeHealth(int amount) override;

    void SetWaypoint(const LegacyVector3& _wayPoint);
    void SetOrders(const LegacyVector3& _orders);

    void SetNextMode();
    void SetPreviousMode();

    void CancelOrderSounds();

    static const char* GetOrderType(int _orderType);
};

class OfficerOrders : public WorldObject
{
  public:
    LegacyVector3 m_wayPoint;
    float m_arrivedTimer;

    OfficerOrders();

    bool Advance() override;
    void Render(float _time) override;
};
