#include "pch.h"

#include "BuildingRenderRegistry.h"
#include "EntityRenderRegistry.h"
#include "TreeBuildingRenderer.h"
#include "DefaultBuildingRenderer.h"
#include "WallBuildingRenderer.h"
#include "StaticShapeBuildingRenderer.h"
#include "ScriptTriggerBuildingRenderer.h"
#include "GodDishBuildingRenderer.h"
#include "GunTurretBuildingRenderer.h"
#include "ControlTowerBuildingRenderer.h"
#include "TeleportBuildingRenderer.h"
#include "RadarDishBuildingRenderer.h"
#include "LaserFenceBuildingRenderer.h"
#include "FenceSwitchBuildingRenderer.h"
#include "TriffidBuildingRenderer.h"
#include "SpamBuildingRenderer.h"
#include "ResearchItemBuildingRenderer.h"
#include "TrunkPortBuildingRenderer.h"
#include "FeedingTubeBuildingRenderer.h"
#include "PowerBuildingRenderer.h"
#include "GeneratorBuildingRenderer.h"
#include "PylonEndBuildingRenderer.h"
#include "SolarPanelBuildingRenderer.h"
#include "NullBuildingRenderer.h"
#include "SpawnBuildingRenderer.h"
#include "SpawnPointBuildingRenderer.h"
#include "MasterSpawnPointBuildingRenderer.h"
#include "ReceiverBuildingRenderer.h"
#include "SpiritProcessorBuildingRenderer.h"
#include "SpiritReceiverBuildingRenderer.h"
#include "BlueprintBuildingRenderer.h"
#include "BlueprintStoreBuildingRenderer.h"
#include "BlueprintConsoleBuildingRenderer.h"
#include "BlueprintRelayBuildingRenderer.h"
#include "MineBuildingRenderer.h"
#include "TrackJunctionBuildingRenderer.h"
#include "RefineryBuildingRenderer.h"
#include "MineShaftBuildingRenderer.h"
#include "AITargetBuildingRenderer.h"
#include "FuelBuildingRenderer.h"
#include "FuelGeneratorBuildingRenderer.h"
#include "FuelStationBuildingRenderer.h"
#include "EscapeRocketBuildingRenderer.h"
#include "ConstructionYardBuildingRenderer.h"
#include "DisplayScreenBuildingRenderer.h"
#include "DynamicNodeBuildingRenderer.h"
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
static TreeBuildingRenderer          s_treeBuildingRenderer;
static DefaultBuildingRenderer       s_defaultBuildingRenderer;
static WallBuildingRenderer          s_wallBuildingRenderer;
static StaticShapeBuildingRenderer   s_staticShapeBuildingRenderer;
static ScriptTriggerBuildingRenderer s_scriptTriggerBuildingRenderer;
static GodDishBuildingRenderer       s_godDishBuildingRenderer;
static GunTurretBuildingRenderer     s_gunTurretBuildingRenderer;
static ControlTowerBuildingRenderer  s_controlTowerBuildingRenderer;
static RadarDishBuildingRenderer     s_radarDishBuildingRenderer;
static LaserFenceBuildingRenderer    s_laserFenceBuildingRenderer;
static FenceSwitchBuildingRenderer   s_fenceSwitchBuildingRenderer;
static TriffidBuildingRenderer       s_triffidBuildingRenderer;
static SpamBuildingRenderer          s_spamBuildingRenderer;
static ResearchItemBuildingRenderer  s_researchItemBuildingRenderer;
static TrunkPortBuildingRenderer     s_trunkPortBuildingRenderer;
static FeedingTubeBuildingRenderer   s_feedingTubeBuildingRenderer;
static PowerBuildingRenderer         s_powerBuildingRenderer;
static GeneratorBuildingRenderer     s_generatorBuildingRenderer;
static PylonEndBuildingRenderer      s_pylonEndBuildingRenderer;
static SolarPanelBuildingRenderer         s_solarPanelBuildingRenderer;
static NullBuildingRenderer               s_nullBuildingRenderer;
static SpawnBuildingRenderer              s_spawnBuildingRenderer;
static SpawnPointBuildingRenderer         s_spawnPointBuildingRenderer;
static MasterSpawnPointBuildingRenderer   s_masterSpawnPointBuildingRenderer;
static ReceiverBuildingRenderer           s_receiverBuildingRenderer;
static SpiritProcessorBuildingRenderer    s_spiritProcessorBuildingRenderer;
static SpiritReceiverBuildingRenderer     s_spiritReceiverBuildingRenderer;
static BlueprintStoreBuildingRenderer     s_blueprintStoreBuildingRenderer;
static BlueprintConsoleBuildingRenderer   s_blueprintConsoleBuildingRenderer;
static BlueprintRelayBuildingRenderer     s_blueprintRelayBuildingRenderer;
static MineBuildingRenderer               s_mineBuildingRenderer;
static TrackJunctionBuildingRenderer      s_trackJunctionBuildingRenderer;
static RefineryBuildingRenderer           s_refineryBuildingRenderer;
static MineShaftBuildingRenderer          s_mineShaftBuildingRenderer;
static AITargetBuildingRenderer           s_aiTargetBuildingRenderer;
static FuelBuildingRenderer               s_fuelBuildingRenderer;
static FuelGeneratorBuildingRenderer      s_fuelGeneratorBuildingRenderer;
static FuelStationBuildingRenderer        s_fuelStationBuildingRenderer;
static EscapeRocketBuildingRenderer       s_escapeRocketBuildingRenderer;
static ConstructionYardBuildingRenderer   s_constructionYardBuildingRenderer;
static DisplayScreenBuildingRenderer      s_displayScreenBuildingRenderer;
static DynamicNodeBuildingRenderer        s_dynamicNodeBuildingRenderer;
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
    g_buildingRenderRegistry.Register(Building::TypeTree,             &s_treeBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeFactory,          &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeCave,             &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeBridge,           &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypePowerstation,     &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeLibrary,          &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeIncubator,        &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeAntHill,          &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSafeArea,         &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeUpgradePort,      &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypePrimaryUpgradePort, &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeWall,             &s_wallBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeStaticShape,      &s_staticShapeBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeScriptTrigger,    &s_scriptTriggerBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeGodDish,           &s_godDishBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeGunTurret,         &s_gunTurretBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeControlTower,      &s_controlTowerBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeRadarDish,          &s_radarDishBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeLaserFence,        &s_laserFenceBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeFenceSwitch,       &s_fenceSwitchBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeTriffid,            &s_triffidBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSpam,               &s_spamBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeResearchItem,       &s_researchItemBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeTrunkPort,          &s_trunkPortBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeFeedingTube,        &s_feedingTubeBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeGenerator,           &s_generatorBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypePylon,               &s_powerBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypePylonStart,          &s_powerBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypePylonEnd,            &s_pylonEndBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSolarPanel,          &s_solarPanelBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSpawnLink,            &s_spawnBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSpawnPoint,           &s_spawnPointBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSpawnPointMaster,     &s_masterSpawnPointBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSpawnPopulationLock,  &s_nullBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSpiritReceiver,       &s_spiritReceiverBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeReceiverLink,         &s_receiverBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeReceiverSpiritSpawner, &s_receiverBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeSpiritProcessor,      &s_spiritProcessorBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeBlueprintStore,        &s_blueprintStoreBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeBlueprintConsole,      &s_blueprintConsoleBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeBlueprintRelay,        &s_blueprintRelayBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeTrackLink,              &s_mineBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeTrackJunction,          &s_trackJunctionBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeTrackStart,             &s_mineBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeTrackEnd,               &s_mineBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeRefinery,               &s_refineryBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeMine,                   &s_mineShaftBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeYard,                   &s_constructionYardBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeDisplayScreen,          &s_displayScreenBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeAITarget,               &s_aiTargetBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeAISpawnPoint,           &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeFuelGenerator,          &s_fuelGeneratorBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeFuelPipe,               &s_fuelBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeFuelStation,            &s_fuelStationBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeEscapeRocket,           &s_escapeRocketBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeDynamicHub,             &s_defaultBuildingRenderer);
    g_buildingRenderRegistry.Register(Building::TypeDynamicNode,            &s_dynamicNodeBuildingRenderer);

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
