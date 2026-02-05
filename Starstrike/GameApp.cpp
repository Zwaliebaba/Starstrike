#include "pch.h"
#include "GameApp.h"
#include "DX9TextRenderer.h"
#include "AuthoritativeServer.h"
#include "PredictiveClient.h"
#include "camera.h"
#include "game_menu.h"
#include "gamecursor.h"
#include "global_world.h"
#include "location.h"
#include "location_input.h"
#include "particle_system.h"
#include "preferences.h"
#include "prefs_other_window.h"
#include "profiler.h"
#include "renderer.h"
#include "script.h"
#include "soundsystem.h"
#include "taskmanager.h"
#include "taskmanager_interface_icons.h"
#include "user_input.h"

void SetPreferenceOverrides(); // See main.cpp

GameApp* g_app = nullptr;

#define GAMEDATAFILE "game.txt"

namespace
{
  const std::vector<hstring> g_supportedLanguages =
  {
 L"ENG", L"FRA", L"ITA", L"DEU", L"ESP", L"RUS", L"POL", L"POR", L"JPN", L"KOR", L"CHS", L"CHT"
  };
}

GameApp::GameApp()
  : m_userInput(nullptr),
    m_soundSystem(nullptr),
    m_particleSystem(nullptr),
    m_profiler(nullptr),
    m_globalWorld(nullptr),
    m_location(nullptr),
    m_locationId(-1),
    m_camera(nullptr),
    m_server(nullptr),
    m_client(nullptr),
    m_networkEntityManager(nullptr),
    m_renderer(nullptr),
    m_locationInput(nullptr),
    m_taskManager(nullptr),
    m_script(nullptr),
    m_startSequence(nullptr),
    m_gameMenu(nullptr),
    m_negativeRenderer(false),
    m_difficultyLevel(0),
    m_largeMenus(false),
    m_paused(false),
    m_requestedLocationId(-1),
    m_requestQuit(false),
    m_levelReset(false),
    m_atMainMenu(false),
    m_gameMode(GameModeNone)
{
  g_app = this;

  g_prefsManager = NEW PrefsManager(GetPreferencesPath());
  SetPreferenceOverrides();

  m_negativeRenderer = g_prefsManager->GetInt("RenderNegative", 0) ? true : false;
  if (m_negativeRenderer)
    m_backgroundColor.Set(255, 255, 255, 255);
  else
    m_backgroundColor.Set(0, 0, 0, 0);

  UpdateDifficultyFromPreferences();

#ifdef PROFILER_ENABLED
  m_profiler = NEW Profiler();
#endif

  m_renderer = NEW Renderer();
  m_renderer->Initialize();

  m_gameCursor = NEW GameCursor();
  m_soundSystem = NEW SoundSystem();
  
  // Initialize client with server address from preferences
  auto serverAddress = g_prefsManager->GetString("ServerAddress", "127.0.0.1");
  m_client = NEW PredictiveClient();
  m_client->Initialize(serverAddress);
  
  // Initialize network entity manager
  m_networkEntityManager = NEW NetworkEntityManager();
  m_networkEntityManager->SetClient(m_client);
  g_networkEntityManager = m_networkEntityManager;
  
  m_userInput = NEW UserInput();

  m_camera = NEW Camera();
  m_gameMenu = NEW GameMenu();

  m_gameDataFile = "game.txt";

  //
  // Determine default language if possible
  m_isoLang = Windows::System::UserProfile::GlobalizationPreferences::Languages().GetAt(0);
  auto lang = Windows::Globalization::Language(m_isoLang).AbbreviatedName();
  auto country = Windows::Globalization::GeographicRegion().DisplayName();

  Strings::SetLanguage(m_isoLang);

  SetLanguage(lang);

  SetProfileName(g_prefsManager->GetString("UserProfile", "none"));

  m_particleSystem = NEW ParticleSystem();
  m_taskManager = NEW TaskManager();
  m_script = NEW Script();

  m_taskManagerInterface = NEW TaskManagerInterfaceIcons();

  m_soundSystem->Initialize();

  int menuOption = g_prefsManager->GetInt(OTHER_LARGEMENUS, 0);
  if (menuOption == 2) // (todo) or is running in media center and tenFootMode == -1
    m_largeMenus = true;

  //
  // Load save games

  std::ignore = LoadProfile();
}

GameApp::~GameApp()
{
  g_networkEntityManager = nullptr;
  SAFE_DELETE(m_networkEntityManager);
  SAFE_DELETE(m_globalWorld);
  SAFE_DELETE(m_taskManagerInterface);
  SAFE_DELETE(m_script);
  SAFE_DELETE(m_taskManager);
  SAFE_DELETE(m_particleSystem);
  SAFE_DELETE(m_camera);
  SAFE_DELETE(m_userInput);
  SAFE_DELETE(m_client);
  SAFE_DELETE(m_server);
  SAFE_DELETE(m_soundSystem);
  SAFE_DELETE(m_gameCursor);
  SAFE_DELETE(m_renderer);
#ifdef PROFILER_ENABLED
  SAFE_DELETE(m_profiler);
#endif
  SAFE_DELETE(g_prefsManager);
}

void GameApp::UpdateDifficultyFromPreferences()
{
  // This method is called to make sure that the difficulty setting
  // used to control the game play (g_app->m_difficultyLevel) is
  // consistent with the user preferences.

  // Preferences value is 1-based, m_difficultyLevel is 0-based.
  m_difficultyLevel = g_prefsManager->GetInt(OTHER_DIFFICULTY, 1) - 1;
  if (m_difficultyLevel < 0)
    m_difficultyLevel = 0;
}

void GameApp::SetLanguage(hstring _language)
{
  //
  // Load localised fonts if they exist

  if (std::ranges::find(g_supportedLanguages, _language) == g_supportedLanguages.end())
  {
    _language = L"ENG";
  }

  hstring fontFilename = L"Fonts\\SpeccyFont-" + _language + L".dds";
  g_gameFont.Startup(fontFilename);

  fontFilename = L"Fonts\\EditorFont-" + _language + L".dds";
  g_editorFont.Startup(fontFilename);
}

void GameApp::SetProfileName(const char* _profileName)
{
  m_userProfileName = _profileName;

  if (_stricmp(_profileName, "AttractMode") != 0)
  {
    g_prefsManager->SetString("UserProfile", m_userProfileName.c_str());
    g_prefsManager->Save();
  }
}

const char* GameApp::GetProfileDirectory()
{
    return "";
}

const char* GameApp::GetPreferencesPath()
{
  // good leak #1
  static char* path = nullptr;

  if (path == nullptr)
  {
    const char* profileDir = GetProfileDirectory();
    path = NEW char[strlen(profileDir) + 32];
    sprintf(path, "%spreferences.txt", profileDir);
  }

  return path;
}

bool GameApp::LoadProfile()
{
  DebugTrace("Loading profile {}\n", m_userProfileName);

  if (m_userProfileName == "AccessAllAreas")
  {
    // Cheat username that opens all locations
    // aimed at beta testers who've completed the game already

    if (m_globalWorld)
    {
      delete m_globalWorld;
      m_globalWorld = nullptr;
    }

    m_globalWorld = NEW GlobalWorld();
    m_globalWorld->LoadGame("game_unlockall.txt");
    for (int i = 0; i < m_globalWorld->m_buildings.Size(); ++i)
    {
      GlobalBuilding* building = m_globalWorld->m_buildings[i];
      if (building && building->m_buildingType == BuildingType::TypeTrunkPort)
        building->m_online = true;
    }
    for (int i = 0; i < m_globalWorld->m_locations.Size(); ++i)
    {
      GlobalLocation* loc = m_globalWorld->m_locations[i];
      loc->m_available = true;
    }
  }
  else
  {
    if (m_globalWorld)
    {
      delete m_globalWorld;
      m_globalWorld = nullptr;
    }

    m_globalWorld = NEW GlobalWorld();
    m_globalWorld->LoadGame(m_gameDataFile);
  }

  return true;
}

void GameApp::LoadCampaign()
{
  m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

  m_gameDataFile = "game.txt";
  LoadProfile();
  m_gameMode = GameModeCampaign;
  m_requestedLocationId = -1;
  g_prefsManager->SetInt("RenderSpecialLighting", 0);
  g_prefsManager->SetInt("CurrentGameMode", 1);
  g_prefsManager->Save();
}
