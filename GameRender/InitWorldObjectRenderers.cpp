#include "pch.h"

#include "WorldObjectRenderRegistry.h"
#include "BoxKiteRenderer.h"
#include "SnowRenderer.h"
#include "WeaponRenderer.h"
#include "MiscEffectRenderer.h"
#include "worldobject.h"

// Instances live for the lifetime of the process — registered once at init.
static SnowRenderer                  s_snowRenderer;
static BoxKiteRenderer               s_boxKiteRenderer;
static WeaponRenderer                s_weaponRenderer;
static MiscEffectRenderer            s_miscEffectRenderer;

void InitWorldObjectRenderers()
{
    g_worldObjectRenderRegistry.Register(WorldObject::EffectSnow, &s_snowRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectBoxKite, &s_boxKiteRenderer);

    // Weapon effects
    g_worldObjectRenderRegistry.Register(WorldObject::EffectThrowableGrenade, &s_weaponRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectThrowableAirstrikeMarker, &s_weaponRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectThrowableAirstrikeBomb, &s_weaponRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectThrowableControllerGrenade, &s_weaponRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectRocket, &s_weaponRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectShockwave, &s_weaponRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectMuzzleFlash, &s_weaponRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectGunTurretShell, &s_weaponRenderer);

    // Misc effects
    g_worldObjectRenderRegistry.Register(WorldObject::EffectGunTurretTarget, &s_miscEffectRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectSpamInfection, &s_miscEffectRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectOfficerOrders, &s_miscEffectRenderer);
    g_worldObjectRenderRegistry.Register(WorldObject::EffectZombie, &s_miscEffectRenderer);
}
