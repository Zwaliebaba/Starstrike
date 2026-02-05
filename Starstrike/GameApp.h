#pragma once

#include "AuthoritativeServer.h"
#include "PredictiveClient.h"
#include "NetworkEntityManager.h"
#include "StartSequence.h"
#include "camera.h"
#include "game_menu.h"
#include "gamecursor.h"
#include "gamemain.h"
#include "global_world.h"
#include "location.h"
#include "location_input.h"
#include "particle_system.h"
#include "renderer.h"
#include "rgb_colour.h"
#include "script.h"
#include "soundsystem.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "user_input.h"

class GameApp : public GameMain
{
  public:
    // Library Code Objects
    UserInput* m_userInput;
    SoundSystem* m_soundSystem;
    ParticleSystem* m_particleSystem;
    Profiler* m_profiler;

    // Things that are the world
    GlobalWorld* m_globalWorld;
    Location* m_location;
    int m_locationId;

    // Everything else
    Camera* m_camera;
    AuthoritativeServer* m_server;                  
    PredictiveClient* m_client;
    NetworkEntityManager* m_networkEntityManager;
    Renderer* m_renderer;
    LocationInput* m_locationInput;
    TaskManager* m_taskManager;
    TaskManagerInterface* m_taskManagerInterface;
    Script* m_script;
    GameCursor* m_gameCursor;
    StartSequence* m_startSequence;
    GameMenu* m_gameMenu;

    bool m_negativeRenderer;
    int m_difficultyLevel;			// Cached from preferences
    bool m_largeMenus;

    // State flags
    bool m_paused;


    // Requested state flags
    int m_requestedLocationId;		// -1 for global world
    bool m_requestQuit;

    std::string m_userProfileName;
    std::string m_requestedMission;
    std::string m_requestedMap;
    bool m_levelReset;
    std::string m_gameDataFile;

    RGBAColor m_backgroundColor;

    bool m_atMainMenu;       // true when the player is viewing the darwinia/mutliwinia menu
    int m_gameMode;

    hstring m_isoLang;

    enum
    {
      GameModeNone,
      GameModeCampaign,
      GameModeMultiwinia,
      NumGameModes
    };

    GameApp();
    ~GameApp() override;

    Windows::Foundation::IAsyncAction Startup() override { co_return; }
    void Shutdown() override {}
    void Update([[maybe_unused]] float _deltaT) override {}
    void RenderScene() override {}

    void SetProfileName(const char* _profileName);
    bool LoadProfile();

    void SetLanguage(hstring _language);

    void LoadCampaign();

    static const char* GetProfileDirectory();
    static const char* GetPreferencesPath();

    void UpdateDifficultyFromPreferences();
};

extern GameApp* g_app;
