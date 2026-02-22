#ifndef INCLUDED_LEVEL_FILE
#define INCLUDED_LEVEL_FILE

#include <stdlib.h>
#include "llist.h"
#include "worldobject.h"
#include "landscape.h"

#define CAMERA_MOUNT_MAX_NAME_LEN	63
#define CAMERA_ANIM_MAX_NAME_LEN	63
#define MAX_FILENAME_LEN			256
#define MAGIC_MOUNT_NAME_START_POS	"CamPosBefore"

class Building;
class TextReader;
class Route;
class GlobalEventCondition;
class FileWriter;

class CameraMount
{
  public:
    char m_name[CAMERA_MOUNT_MAX_NAME_LEN + 1];
    LegacyVector3 m_pos;
    LegacyVector3 m_front;
    LegacyVector3 m_up;
};

class CamAnimNode
{
  public:
    enum
    {
      TransitionMove,
      TransitionCut,
      TransitionNumModes
    };

    int m_transitionMode;
    char* m_mountName;
    float m_duration;

    CamAnimNode()
      : m_transitionMode(TransitionMove),
        m_mountName(nullptr),
        m_duration(1.0f) {}

    ~CamAnimNode()
    {
      free(m_mountName);
      m_mountName = nullptr;
    }

    static int GetTransitModeId(const char* _word);
    static const char* GetTransitModeName(int _modeId);
};

class CameraAnimation
{
  public:
    LList<CamAnimNode*> m_nodes;
    char m_name[CAMERA_ANIM_MAX_NAME_LEN + 1];

    ~CameraAnimation() { m_nodes.EmptyAndDelete(); }
};

class InstantUnit
{
  public:
    InstantUnit()
      : m_type(-1),
        m_teamId(-1),
        m_posX(0.0f),
        m_posZ(0.0f),
        m_number(0),
        m_inAUnit(false),
        m_state(-1),
        m_spread(200.0f),
        m_waypointX(0.0f),
        m_waypointZ(0.0f),
        m_routeId(-1),
        m_routeWaypointId(-1) {}

    int m_type;
    int m_teamId;
    float m_posX;
    float m_posZ;
    int m_number;
    bool m_inAUnit;
    int m_state;
    float m_spread;
    float m_waypointX;
    float m_waypointZ;
    int m_routeId;
    int m_routeWaypointId;
};

class LandscapeFlattenArea
{
  public:
    LegacyVector3 m_centre;
    float m_size;
};

class LandscapeDef
{
  public:
    LList<LandscapeTile*> m_tiles;
    LList<LandscapeFlattenArea*> m_flattenAreas;
    float m_cellSize;
    int m_worldSizeX;
    int m_worldSizeZ;
    float m_outsideHeight;

    LandscapeDef()
      : m_cellSize(12.0f),
        m_worldSizeX(2000),
        m_worldSizeZ(2000),
        m_outsideHeight(-10) {}

    ~LandscapeDef()
    {
      m_tiles.EmptyAndDelete();
      m_flattenAreas.EmptyAndDelete();
    }
};

class RunningProgram
{
  public:
    int m_type;
    int m_count;
    int m_state;
    int m_data;
    float m_waypointX;
    float m_waypointZ;

    float m_positionX[10];
    float m_positionZ[10];
    int m_health[10];
};

// ***************************************************************************
// Class LevelFile
// ***************************************************************************

class LevelFile
{
  void ParseMissionFile(const char* _filename);
  void ParseMapFile(const char* _filename);

  void ParseCameraMounts(TextReader* _in);
  void ParseCameraAnims(TextReader* _in);
  void ParseBuildings(TextReader* _in, bool _dynamic);
  void ParseInstantUnits(TextReader* _in);
  void ParseLandscapeData(TextReader* _in);
  void ParseLandscapeTiles(TextReader* _in);
  void ParseLandFlattenAreas(TextReader* _in);
  void ParseLights(TextReader* _in);
  void ParseRoute(TextReader* _in, int _id);
  void ParseRoutes(TextReader* _in);
  void ParsePrimaryObjectives(TextReader* _in);
  void ParseRunningPrograms(TextReader* _in);
  void ParseDifficulty(TextReader* _in);

  void GenerateAutomaticObjectives();

  public:
    char m_missionFilename[MAX_FILENAME_LEN];
    char m_mapFilename[MAX_FILENAME_LEN];

    char m_landscapeColourFilename[MAX_FILENAME_LEN];
    char m_wavesColourFilename[MAX_FILENAME_LEN];
    char m_waterColourFilename[MAX_FILENAME_LEN];

    LList<CameraMount*> m_cameraMounts;
    LList<CameraAnimation*> m_cameraAnimations;
    LList<Building*> m_buildings;
    LList<InstantUnit*> m_instantUnits;
    LList<Light*> m_lights;
    LList<Route*> m_routes;
    LList<RunningProgram*> m_runningPrograms;
    LList<GlobalEventCondition*> m_primaryObjectives;
    LList<GlobalEventCondition*> m_secondaryObjectives; // This data isn't stored in the map or mission files
    // directly, but is calculated at load time for your
    // convenience
    int m_levelDifficulty; // The difficulty factor that this level represents.

    LandscapeDef m_landscape;

    LevelFile();
    LevelFile(const char* _missionFilename, const char* _mapFilename);
    ~LevelFile();

    Building* GetBuilding(int _id);
    CameraMount* GetCameraMount(const char* _name);
    int GetCameraAnimId(const char* _name);
    void RemoveBuilding(int _id);
    int GenerateNewRouteId();
    Route* GetRoute(int _id);

    void GenerateInstantUnits();
    void GenerateDynamicBuildings();
};

#endif
