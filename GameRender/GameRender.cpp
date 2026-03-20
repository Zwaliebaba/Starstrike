#include "pch.h"

#include "BuildingRenderRegistry.h"
#include "EntityRenderRegistry.h"
#include "TreeBuildingRenderer.h"
#include "ArmyAntRenderer.h"
#include "EggRenderer.h"
#include "OfficerRenderer.h"
#include "TriffidEggRenderer.h"
#include "EngineerRenderer.h"
#include "ArmourRenderer.h"
#include "TripodRenderer.h"
#include "CentipedeRenderer.h"
#include "SpiderRenderer.h"
#include "SoulDestroyerRenderer.h"
#include "DarwinianRenderer.h"
#include "ViriiRenderer.h"
#include "LanderRenderer.h"
#include "LaserTrooperRenderer.h"
#include "building.h"
#include "entity.h"

// Instances live for the lifetime of the process — registered once at init.
static TreeBuildingRenderer  s_treeBuildingRenderer;
static ArmyAntRenderer       s_armyAntRenderer;
static EggRenderer           s_eggRenderer;
static OfficerRenderer       s_officerRenderer;
static TriffidEggRenderer    s_triffidEggRenderer;
static EngineerRenderer      s_engineerRenderer;
static ArmourRenderer        s_armourRenderer;
static TripodRenderer        s_tripodRenderer;
static CentipedeRenderer     s_centipedeRenderer;
static SpiderRenderer        s_spiderRenderer;
static SoulDestroyerRenderer s_soulDestroyerRenderer;
static DarwinianRenderer     s_darwinianRenderer;
static ViriiRenderer         s_viriiRenderer;
static LanderRenderer        s_landerRenderer;
static LaserTrooperRenderer  s_laserTrooperRenderer;

void InitGameRenderers()
{
    g_buildingRenderRegistry.Register(Building::TypeTree, &s_treeBuildingRenderer);

    g_entityRenderRegistry.Register(Entity::TypeArmyAnt,       &s_armyAntRenderer);
    g_entityRenderRegistry.Register(Entity::TypeEgg,            &s_eggRenderer);
    g_entityRenderRegistry.Register(Entity::TypeOfficer,        &s_officerRenderer);
    g_entityRenderRegistry.Register(Entity::TypeTriffidEgg,     &s_triffidEggRenderer);
    g_entityRenderRegistry.Register(Entity::TypeEngineer,       &s_engineerRenderer);
    g_entityRenderRegistry.Register(Entity::TypeArmour,         &s_armourRenderer);
    g_entityRenderRegistry.Register(Entity::TypeTripod,         &s_tripodRenderer);
    g_entityRenderRegistry.Register(Entity::TypeCentipede,      &s_centipedeRenderer);
    g_entityRenderRegistry.Register(Entity::TypeSpider,         &s_spiderRenderer);
    g_entityRenderRegistry.Register(Entity::TypeSoulDestroyer,  &s_soulDestroyerRenderer);
    g_entityRenderRegistry.Register(Entity::TypeDarwinian,      &s_darwinianRenderer);
    g_entityRenderRegistry.Register(Entity::TypeVirii,          &s_viriiRenderer);
    g_entityRenderRegistry.Register(Entity::TypeLander,         &s_landerRenderer);
    g_entityRenderRegistry.Register(Entity::TypeLaserTroop,     &s_laserTrooperRenderer);
}
