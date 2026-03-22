// GameLogic/GameAppSim.h
//
// Simulation-side application state.  Contains all data members that
// simulation code accesses through g_app->.  This header has NO
// dependency on NeuronClient, GameMain, or any GPU/rendering headers.
//
// GameApp (Starstrike) inherits GameAppSim + GameMain.
// GameLogic .cpp files include this header instead of GameApp.h.
// GameRender / Starstrike continue to include GameApp.h (which
// includes this header and adds GameMain inheritance).

#pragma once

#include "rgb_colour.h"

class Camera;
class Location;
class Server;
class ClientToServer;
class Renderer;
class UserInput;
class Resource;
class SoundSystem;
class LocationInput;
class LangTable;
class GlobalWorld;
class ParticleSystem;
class TaskManager;
class TaskManagerInterface;
class Script;
class Profiler;
class MouseCursor;
class GameCursor;
class GameCursor2D;
class StartSequence;
class BitmapRGBA;
class Sepulveda;

class SoundLibrary2d;

class GameAppSim
{
  public:
    virtual ~GameAppSim() = default;

    // Library Code Objects
    UserInput* m_userInput = nullptr;
    Resource* m_resource = nullptr;
    SoundSystem* m_soundSystem = nullptr;
    ParticleSystem* m_particleSystem = nullptr;
    LangTable* m_langTable = nullptr;
    Profiler* m_profiler = nullptr;

    // Things that are the world
    GlobalWorld* m_globalWorld = nullptr;
    Location* m_location = nullptr;
    int m_locationId = -1;

    // Everything else
    Camera* m_camera = nullptr;
    Server* m_server = nullptr; // Server process, can be NULL if client
    ClientToServer* m_clientToServer = nullptr; // Clients connection to Server
    Renderer* m_renderer = nullptr;
    LocationInput* m_locationInput = nullptr;
    TaskManager* m_taskManager = nullptr;
    TaskManagerInterface* m_taskManagerInterface = nullptr;
    Script* m_script = nullptr;
    GameCursor* m_gameCursor = nullptr;
    GameCursor2D* m_gameCursor2D = nullptr;
    StartSequence* m_startSequence = nullptr;

    bool m_bypassNetworking = false;
    bool m_negativeRenderer = false;
    int m_difficultyLevel = 0; // Cached from preferences
    bool m_largeMenus = false;

    // State flags
    bool m_paused = false;
    bool m_editing = false;

    // Requested state flags
    int m_requestedLocationId = -1; // -1 for global world
    bool m_requestToggleEditing = false;
    bool m_requestQuit = false;

    char m_userProfileName[256];
    char m_requestedMission[256];
    char m_requestedMap[256];
    bool m_levelReset = false;
    char m_gameDataFile[256];

    RGBAColour m_backgroundColour;

    bool m_atMainMenu = false; // true when the player is viewing the darwinia/mutliwinia menu
    int m_gameMode = GameModeNone;

    enum
    {
      GameModeNone,
      GameModePrologue,
      GameModeCampaign,
      GameModeMultiwinia,
      NumGameModes
    };

    // --- Lifecycle methods ---
    // Overridden in GameApp (Starstrike).  Default implementations are
    // no-ops so that a headless/server build does not require Starstrike.

    virtual void SetProfileName([[maybe_unused]] const char* _profileName) {}
    virtual bool LoadProfile() { return false; }
    virtual void ResetLevel([[maybe_unused]] bool _global) {}

    virtual void SetLanguage([[maybe_unused]] const char* _language,
                             [[maybe_unused]] bool _test) {}

    virtual void LoadPrologue() {}
    virtual void LoadCampaign() {}

    virtual void UpdateDifficultyFromPreferences() {}

    // --- Static utilities ---

    static const char* GetProfileDirectory();
    static const char* GetPreferencesPath();
    static const char* GetScreenshotDirectory();
};

extern GameAppSim* g_app;
