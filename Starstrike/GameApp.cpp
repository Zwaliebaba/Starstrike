#include "pch.h"
#include "clienttoserver.h"
#include "language_table.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "system_info.h"
#include "text_renderer.h"
#include "filesys_utils.h"
#include "file_writer.h"
#include "prefs_other_window.h"
#include "sound_library_2d.h"
#include "soundsystem.h"
#include "GameApp.h"
#include "camera.h"
#include "global_world.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "script.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface_icons.h"
#include "gamecursor.h"
#include "gamecursor_2d.h"

GameApp* g_gameApp = nullptr;

#define GAMEDATAFILE "game.txt"

GameApp::GameApp()
{
  g_gameApp = this;
  g_context = this;

  // Load resources

  g_prefsManager = new PrefsManager(GetPreferencesPath());

  m_bypassNetworking = g_prefsManager->GetInt("BypassNetwork") ? true : false;

  m_negativeRenderer = g_prefsManager->GetInt("RenderNegative", 0) ? true : false;
  if (m_negativeRenderer)
    m_backgroundColour.Set(255, 255, 255, 255);
  else
    m_backgroundColour.Set(0, 0, 0, 0);

  UpdateDifficultyFromPreferences();

#ifdef PROFILER_ENABLED
  m_profiler = new Profiler();
#endif

  m_renderer = new Renderer();
  m_renderer->Initialise();

  m_gameCursor = new GameCursor();
  m_gameCursor2D = new GameCursor2D(*m_gameCursor);
  m_soundSystem = new SoundSystem();
  m_clientToServer = new ClientToServer();
  m_userInput = new UserInput();

  m_camera = new Camera();

  strncpy(m_gameDataFile, "game.txt", sizeof(m_gameDataFile));
  m_gameDataFile[sizeof(m_gameDataFile) - 1] = '\0';

  //
  // Determine default language if possible

  const char* language = g_prefsManager->GetString("TextLanguage");
  if (_stricmp(language, "unknown") == 0)
  {
    char* defaultLang = g_systemInfo->m_localeInfo.m_language;
    char langFilename[512];
    snprintf(langFilename, sizeof(langFilename), "%slanguage\\%s.txt", FileSys::GetHomeDirectoryA().c_str(), defaultLang);
    if (DoesFileExist(langFilename))
      g_prefsManager->SetString("TextLanguage", defaultLang);
    else
      g_prefsManager->SetString("TextLanguage", "english");
  }
  language = g_prefsManager->GetString("TextLanguage");

  SetLanguage(language, g_prefsManager->GetInt("TextLanguageTest", 0));

  SetProfileName(g_prefsManager->GetString("UserProfile", "none"));

  m_particleSystem = new ParticleSystem();
  m_taskManager = new TaskManager();
  m_script = new Script();
  
  m_taskManagerInterface = new TaskManagerInterfaceIcons();

  m_soundSystem->Initialise();

  int menuOption = g_prefsManager->GetInt(OTHER_LARGEMENUS, 0);
  if (menuOption == 2) // (todo) or is running in media center and tenFootMode == -1
    m_largeMenus = true;

  //
  // Load save games

  LoadProfile();
}

GameApp::~GameApp()
{
  SAFE_DELETE(m_globalWorld);
  SAFE_DELETE(m_langTable);
  SAFE_DELETE(m_taskManagerInterface);
  SAFE_DELETE(m_script);
  SAFE_DELETE(m_taskManager);
  SAFE_DELETE(m_particleSystem);
  SAFE_DELETE(m_camera);
  SAFE_DELETE(m_userInput);
  SAFE_DELETE(m_clientToServer);
  SAFE_DELETE(m_soundSystem);
  SAFE_DELETE(m_gameCursor2D);
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
  // used to control the game play (g_context->m_difficultyLevel) is
  // consistent with the user preferences.

  // Preferences value is 1-based, m_difficultyLevel is 0-based.
  m_difficultyLevel = g_prefsManager->GetInt(OTHER_DIFFICULTY, 1) - 1;
  if (m_difficultyLevel < 0)
    m_difficultyLevel = 0;
}

void GameApp::SetLanguage(const char* _language, bool _test)
{
  //
  // Delete existing language data

  if (m_langTable)
  {
    delete m_langTable;
    m_langTable = nullptr;
  }

  //
  // Load the language text file

  char langFilename[256];
  snprintf(langFilename, sizeof(langFilename), "language/%s.txt", _language);

  m_langTable = new LangTable(langFilename);

  if (_test)
    m_langTable->TestAgainstEnglish();

  //
  // Load the MOD language file if it exists

  snprintf(langFilename, sizeof(langFilename), "strings_%s.txt", _language);
  TextReader* modLangFile = Resource::GetTextReader(langFilename);
  if (!modLangFile)
  {
    snprintf(langFilename, sizeof(langFilename), "strings_default.txt");
    modLangFile = Resource::GetTextReader(langFilename);
  }

  if (modLangFile)
  {
    delete modLangFile;
    m_langTable->ParseLanguageFile(langFilename);
  }

  //
  // Load localised fonts if they exist

  char fontFilename[256];
  snprintf(fontFilename, sizeof(fontFilename), "textures/speccy_font_%s.bmp", _language);
  if (!Resource::DoesTextureExist(fontFilename))
    snprintf(fontFilename, sizeof(fontFilename), "textures/speccy_font_normal.bmp");
  g_gameFont.Initialise(fontFilename);

  snprintf(fontFilename, sizeof(fontFilename), "textures/editor_font_%s.bmp", _language);
  if (!Resource::DoesTextureExist(fontFilename))
    snprintf(fontFilename, sizeof(fontFilename), "textures/editor_font_normal.bmp");
  g_editorFont.Initialise(fontFilename);

  if (g_inputManager)
    m_langTable->RebuildTables();
}

void GameApp::SetProfileName(const char* _profileName)
{
  strncpy(m_userProfileName, _profileName, sizeof(m_userProfileName));
  m_userProfileName[sizeof(m_userProfileName) - 1] = '\0';

  if (_stricmp(_profileName, "AttractMode") != 0)
  {
    g_prefsManager->SetString("UserProfile", m_userProfileName);
    g_prefsManager->Save();
  }
}

const char* GameContext::GetProfileDirectory()
{
  return "";
}

const char* GameContext::GetPreferencesPath()
{
  // good leak #1
  static char* path = nullptr;

  if (path == nullptr)
  {
    const char* profileDir = GetProfileDirectory();
    size_t pathSize = strlen(profileDir) + 32;
    path = new char[pathSize];
    snprintf(path, pathSize, "%spreferences.txt", profileDir);
  }

  return path;
}

const char* GameContext::GetScreenshotDirectory()
{
  return "";
}

bool GameApp::LoadProfile()
{
  DebugTrace("Loading profile {}\n", m_userProfileName);

  if ((_stricmp(m_userProfileName, "AccessAllAreas") == 0 || _stricmp(m_userProfileName, "AttractMode") == 0) && g_context->m_gameMode !=
    GameModePrologue)
  {
    // Cheat username that opens all locations
    // aimed at beta testers who've completed the game already

    if (m_globalWorld)
    {
      delete m_globalWorld;
      m_globalWorld = nullptr;
    }

    m_globalWorld = new GlobalWorld();
    m_globalWorld->LoadGame("game_unlockall.txt");
    for (int i = 0; i < m_globalWorld->m_buildings.Size(); ++i)
    {
      GlobalBuilding* building = m_globalWorld->m_buildings[i];
      if (building && building->m_type == Building::TypeTrunkPort)
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

    m_globalWorld = new GlobalWorld();
    m_globalWorld->LoadGame(m_gameDataFile);
  }

  return true;
}

void GameApp::ResetLevel(bool _global)
{
  if (m_location)
  {
    m_requestedLocationId = -1;
    m_requestedMission[0] = '\0';
    m_requestedMap[0] = '\0';

    m_levelReset = true;

    //
    // Delete the game file if required

    if (_global)
    {
      if (m_globalWorld)
      {
        delete m_globalWorld;
        m_globalWorld = nullptr;
      }

      m_globalWorld = new GlobalWorld();
      m_globalWorld->LoadGame(m_gameDataFile);
    }
  }
}

void GameApp::LoadPrologue()
{
  m_gameMode = GameModePrologue;

  m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

  strncpy(m_gameDataFile, "game_demo2.txt", sizeof(m_gameDataFile));
  m_gameDataFile[sizeof(m_gameDataFile) - 1] = '\0';
  LoadProfile();

  m_requestedLocationId = m_globalWorld->GetLocationId("launchpad");
  GlobalLocation* gloc = m_globalWorld->GetLocation(m_requestedLocationId);
  strncpy(m_requestedMap, gloc->m_mapFilename.c_str(), sizeof(m_requestedMap));
  m_requestedMap[sizeof(m_requestedMap) - 1] = '\0';
  strncpy(m_requestedMission, gloc->m_missionFilename.c_str(), sizeof(m_requestedMission));
  m_requestedMission[sizeof(m_requestedMission) - 1] = '\0';

  m_atMainMenu = false;

  g_prefsManager->SetInt("RenderSpecialLighting", 1);
  g_prefsManager->SetInt("CurrentGameMode", 0);
  g_prefsManager->Save();
}

void GameApp::LoadCampaign()
{
  m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

  //m_atMainMenu = false;

  strncpy(m_gameDataFile, "game.txt", sizeof(m_gameDataFile));
  m_gameDataFile[sizeof(m_gameDataFile) - 1] = '\0';
  LoadProfile();
  m_gameMode = GameModeCampaign;
  m_requestedLocationId = -1;
  g_prefsManager->SetInt("RenderSpecialLighting", 0);
  g_prefsManager->SetInt("CurrentGameMode", 1);
  g_prefsManager->Save();
}

// --- Lifecycle overrides ---

void GameApp::OnActivated()
{
  GameMain::OnActivated();

  if (g_soundLibrary2d)
    g_soundLibrary2d->Resume();
}

void GameApp::OnDeactivated()
{
  GameMain::OnDeactivated();

  if (g_soundLibrary2d)
    g_soundLibrary2d->Pause();
}

void GameApp::OnSuspending()
{
  GameMain::OnSuspending();

  if (g_soundLibrary2d)
    g_soundLibrary2d->Pause();
}

void GameApp::OnResuming()
{
  GameMain::OnResuming();

  if (g_soundLibrary2d)
    g_soundLibrary2d->Resume();
}
