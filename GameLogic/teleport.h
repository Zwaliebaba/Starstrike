#pragma once

#include "LegacyVector3.h"

#include "building.h"

class Entity;

struct TeleportMap
{
  int m_teamId;
  int m_fromUnitId;
  int m_toUnitId;
};

class Teleport : public Building
{
  protected:
    float m_timeSync;
    float m_sendPeriod; // Minimum time between sends
    static LList<TeleportMap> m_teleportMap; // Maps units going in onto units comingout

    ShapeMarkerData* m_entrance;

    void RenderSpirit(const LegacyVector3& _pos, int _teamId);

  public:
    LList<WorldObjectId> m_inTransit; // Entities on the move

    Teleport();

    void SetShape(ShapeStatic* _shape) override;

    bool Advance() override;

    void EnterTeleport(WorldObjectId _id, bool _relay = false); // Relay=true means i've entered directly from another teleport

    bool IsInView() override;

    virtual bool Connected();
    virtual bool ReadyToSend();

    virtual bool GetEntrance(LegacyVector3& _pos, LegacyVector3& _front);
    virtual bool GetExit(LegacyVector3& _pos, LegacyVector3& _front);

    virtual LegacyVector3 GetStartPoint();
    virtual LegacyVector3 GetEndPoint();

    virtual bool UpdateEntityInTransit(Entity* _entity); // Returns true (remove me) or false (still inside)
};
