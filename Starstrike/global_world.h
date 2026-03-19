#pragma once

#include "llist.h"
#include "LegacyVector3.h"

class FileWriter;
class TextReader;
class ShapeStatic;
class Building;
class GlobalInternet;

// ****************************************************************************
// GlobalLocation
// ****************************************************************************

class GlobalLocation
{
  public:
    int m_id;
    LegacyVector3 m_pos;
    bool m_available; // Is it connected on the transit system

    std::string m_name;
    std::string m_mapFilename;
    std::string m_missionFilename;
    bool m_missionCompleted;

    int m_numSpirits; // Number of spirits that have died

    GlobalLocation();

    void AddSpirits(int _count = 1);
};

// ****************************************************************************
// GlobalBuilding
// ****************************************************************************

class GlobalBuilding
{
  public:
    int m_id;
    int m_teamId;
    int m_locationId;
    LegacyVector3 m_pos;
    int m_type;
    bool m_online;
    int m_link;
    ShapeStatic* m_shape;

    GlobalBuilding();
};

// ****************************************************************************
// Class GlobalEvent + guests
// ****************************************************************************

class GlobalEventCondition
{
  public:
    enum
    {
      AlwaysTrue,
      // 0
      BuildingOnline,
      // 1
      BuildingOffline,
      // 2
      ResearchOwned,
      // 3
      NotInLocation,
      // 4
      DebugKey,
      // 5
      NeverTrue,
      // 6
      NumConditions // Remember to update GetTypeName
    };

    int m_type;
    int m_id;
    int m_locationId;
    char* m_stringId; // Brief description
    char* m_cutScene; // Filename of cutscene to run

    GlobalEventCondition();
    GlobalEventCondition(const GlobalEventCondition& _other);
    ~GlobalEventCondition();

    bool Evaluate();

    void SetStringId(const char* _stringId);
    void SetCutScene(char* _cutScene);

    void Save(FileWriter* _out);

    static const char* GetTypeName(int _type);
    static int GetType(const char* _typeName);
};

class GlobalEventAction
{
  public:
    enum
    {
      SetMission,
      RunScript,
      MakeAvailable,
      NumActionTypes
    };

    int m_type;
    int m_locationId;
    std::string m_filename;

    GlobalEventAction();

    void Read(TextReader* _in);
    void Write(FileWriter* _file);
    void Execute();

    static const char* GetTypeName(int _type);
};

class GlobalEvent
{
  public:
    LList<GlobalEventCondition*> m_conditions;
    LList<GlobalEventAction*> m_actions;

    GlobalEvent();

    void Read(TextReader* _in);
    void Write(FileWriter* _file);
    bool Evaluate();
    bool Execute(); // Returns true when all done

    void MakeAlwaysTrue();
};

// ****************************************************************************
// Class GlobalResearch
// ****************************************************************************

#define GLOBALRESEARCH_TIMEPERPOINT             10
#define GLOBALRESEARCH_POINTS_CONTROLTOWER      22

class GlobalResearch
{
  public:
    enum
    {
      TypeDarwinian,
      TypeOfficer,
      TypeSquad,
      TypeLaser,
      TypeGrenade,
      TypeRocket,
      TypeController,
      TypeAirStrike,
      TypeArmour,
      TypeTaskManager,
      TypeEngineer,
      NumResearchItems
    };

    int m_researchLevel[NumResearchItems];
    int m_researchProgress[NumResearchItems];
    int m_currentResearch;
    int m_researchPoints;
    float m_researchTimer;

    GlobalResearch();

    bool HasResearch(int _type);
    int CurrentProgress(int _type);
    int CurrentLevel(int _type);

    void AddResearch(int _type);
    void SetCurrentProgress(int _type, int _progress);

    void IncreaseProgress(int _amount);
    void DecreaseProgress(int _amount);
    int RequiredProgress(int _level); // Progress required to reach this level

    void EvaluateLevel(int _type);

    void SetCurrentResearch(int _type);
    void GiveResearchPoints(int _numPoints);
    void AdvanceResearch();

    void Write(FileWriter* _out);
    void Read(TextReader* _in);

    static const char* GetTypeName(int _type);
    static int GetType(const char* _name);

    static const char* GetTypeNameTranslated(int _type);
};

// ****************************************************************************
// Class SphereWorld
// ****************************************************************************

class SphereWorld
{
  public:
    ShapeStatic* m_shapeOuter;
    ShapeStatic* m_shapeMiddle;
    ShapeStatic* m_shapeInner;

    int m_numLocations;
    LList<float>* m_spirits; // An array with one LList<float> per location

    SphereWorld();

    void AddLocation(int _locationId);

    void Render();
    void RenderWorldShape();
    void RenderIslands();
    void RenderTrunkLinks();
    void RenderHeaven();
    void RenderSpirits();
};

// ****************************************************************************
// Class GlobalWorld
// ****************************************************************************

class GlobalWorld
{
  public:
    GlobalInternet* m_globalInternet;
    SphereWorld* m_sphereWorld;
    GlobalResearch* m_research;

    LList<GlobalLocation*> m_locations;
    LList<GlobalBuilding*> m_buildings;
    LList<GlobalEvent*> m_events;
    int m_myTeamId;

    int m_editorMode;
    int m_editorSelectionId;

  protected:
    void ParseLocations(TextReader* _in);
    void ParseBuildings(TextReader* _in);
    void ParseEvents(TextReader* _in);

    void AddLevelBuildingToGlobalBuildings(Building* _building, int _locId);

    int m_nextLocationId;
    int m_nextBuildingId;

    int m_locationRequested; // Stores the location a user has clicked on while we fade out. -1 means no request yet.

  public:
    GlobalWorld();
    ~GlobalWorld();

    void Advance();
    void Render();

    int LocationHit(const LegacyVector3& _pos, const LegacyVector3& _dir, float locationRadius = 5000.0f);

    void AddLocation(GlobalLocation* location);
    void AddBuilding(GlobalBuilding* building);

    GlobalLocation* GetLocation(int _id);
    GlobalLocation* GetLocation(const char* _name);
    GlobalLocation* GetHighlightedLocation(); // ie whats under the mouse
    int GetLocationId(const char* _name);
    int GetLocationIdFromMapFilename(const char* _mapFilename);
    const char* GetLocationName(int _id);
    const char* GetLocationNameTranslated(int _id);
    LegacyVector3 GetLocationPosition(int _id);

    GlobalBuilding* GetBuilding(int _id, int _locationId);
    int GenerateBuildingId();

    bool EvaluateEvents(); // Returns true if an event was triggered
    void TransferSpirits(int _locationId);

    void LoadGame(const char* _filename);

    void LoadLocations(const char* _filename);

    void SetupLights();
    void SetupFog();

    float GetSize();
};

