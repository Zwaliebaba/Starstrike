#ifndef _INCLUDED_APP_H
#define _INCLUDED_APP_H

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
class EffectProcessor;
class GlobalWorld;
class ParticleSystem;
class TaskManager;
class TaskManagerInterface;
class Script;
class Profiler;
class MouseCursor;
class GameCursor;
class GameMenu;
class StartSequence;
class BitmapRGBA;

class GameApp : public GameMain
{
  public:
    // Library Code Objects
    UserInput* m_userInput;
    Resource* m_resource;
    SoundSystem* m_soundSystem;
    ParticleSystem* m_particleSystem;
    LangTable* m_langTable;
    Profiler* m_profiler;

    // Things that are the world
    GlobalWorld* m_globalWorld;
    Location* m_location;
    int m_locationId;

    // Everything else
    Camera* m_camera;
    Server* m_server; // Server process, can be NULL if client
    ClientToServer* m_clientToServer; // Clients connection to Server
    Renderer* m_renderer;
    LocationInput* m_locationInput;
    EffectProcessor* m_effectProcessor;
    TaskManager* m_taskManager;
    TaskManagerInterface* m_taskManagerInterface;
    Script* m_script;
    GameCursor* m_gameCursor;
    StartSequence* m_startSequence;
    GameMenu* m_gameMenu;

    bool m_bypassNetworking;
    bool m_negativeRenderer;
    int m_difficultyLevel; // Cached from preferences
    bool m_largeMenus;

    // State flags
    bool m_paused;
    bool m_editing;

    // Requested state flags
    int m_requestedLocationId; // -1 for global world
    bool m_requestToggleEditing;
    bool m_requestQuit;

    char m_userProfileName[256];
    char m_requestedMission[256];
    char m_requestedMap[256];
    bool m_levelReset;
    char m_gameDataFile[256];

    RGBAColour m_backgroundColour;

    bool m_atMainMenu; // true when the player is viewing the darwinia/mutliwinia menu
    int m_gameMode;

    enum
    {
      GameModeNone,
      GameModePrologue,
      GameModeCampaign,
      GameModeMultiwinia,
      NumGameModes
    };

    GameApp();
    ~GameApp() override;

    void Startup() override {}
    void Shutdown() override {}
    void Update([[maybe_unused]] float _deltaT) override {}
    void RenderScene() override {}

    void SetProfileName(char* _profileName);
    bool LoadProfile();
    void ResetLevel(bool _global);

    void SetLanguage(char* _language, bool _test);

    void LoadPrologue();
    void LoadCampaign();

    static const char* GetProfileDirectory();
    static const char* GetPreferencesPath();
    static const char* GetScreenshotDirectory();

    void UpdateDifficultyFromPreferences();
};

extern GameApp* g_app;

#endif
