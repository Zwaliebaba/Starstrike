// GameLogic/GameContext.h
//
// Plain simulation-state struct.  Contains all data members that
// simulation code accesses through g_context->.  This header has NO
// dependency on NeuronClient, GameMain, or any GPU/rendering headers.
//
// GameApp (Starstrike) inherits GameContext (for data) + GameMain
// (for rendering/window lifecycle).
//
// GameLogic .cpp files include this header instead of GameApp.h.
// GameRender / Starstrike continue to include GameApp.h (which
#pragma once

#include "rgb_colour.h"

class Camera;
class Location;
class Server;
class ClientToServer;
class Renderer;
class UserInput;
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
class SoundLibrary2d;

struct GameContext
{
    // Library Code Objects
    UserInput* m_userInput = nullptr;
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

    // --- Static utilities ---

    static const char* GetProfileDirectory();
    static const char* GetPreferencesPath();
    static const char* GetScreenshotDirectory();
};

inline GameContext* g_context = {};
