#pragma once

#include "WorldObjectRenderer.h"

class OfficerOrders;
class SpamInfection;
class Zombie;

// Render companion for miscellaneous WorldObject effects that don't belong
// to the weapon family: OfficerOrders (waypoint starburst),
// SpamInfection (trail), Zombie (ghost sprite).
// EffectGunTurretTarget is a no-op (empty render) and is handled as the
// default case.
class MiscEffectRenderer : public WorldObjectRenderer
{
  public:
    void Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx) override;

  private:
    static void RenderOfficerOrders(const OfficerOrders& _orders, float _predictionTime);
    static void RenderSpamInfection(const SpamInfection& _infection, float _predictionTime);
    static void RenderZombie(const Zombie& _zombie, float _predictionTime);
};
