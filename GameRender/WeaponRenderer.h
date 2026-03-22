#pragma once

#include "WorldObjectRenderer.h"

class Laser;

// Single render companion for all weapon-type WorldObject effects.
// Dispatches internally by m_type to per-weapon static methods.
// Missile is dead code (never instantiated) and has no renderer entry.
class WeaponRenderer : public WorldObjectRenderer
{
  public:
    void Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx) override;

    // Laser is stored by value in Location::m_lasers, not in m_effects.
    // Called directly from the m_lasers render loop.
    static void RenderLaser(const Laser& _laser, float _predictionTime);

  private:
    static void RenderThrowable(const class ThrowableWeapon& _weapon, float _predictionTime);
    static void RenderRocket(const class Rocket& _rocket, float _predictionTime);
    static void RenderShockwave(const class Shockwave& _shockwave, float _predictionTime);
    static void RenderMuzzleFlash(const class MuzzleFlash& _flash, float _predictionTime);
    static void RenderTurretShell(const class TurretShell& _shell, float _predictionTime);
};
