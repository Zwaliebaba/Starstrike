#include "pch.h"
#include "clienttoserver.h"
#include "language_table.h"
#include "mouse_cursor.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "system_info.h"
#include "text_renderer.h"
#include "filesys_utils.h"
#include "file_writer.h"
#include "prefs_other_window.h"
#include "sound_stream_decoder.h"
#include "soundsystem.h"
#include "GameApp.h"
#include "camera.h"
#include "global_world.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "script.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface_icons.h"
#include "gamecursor.h"
#include "level_file.h"
#include "game_menu.h"

GameApp* g_app = nullptr;

#define GAMEDATAFILE "game.txt"

GameApp::GameApp()
  : m_userInput(nullptr),
    m_resource(nullptr),
    m_soundSystem(nullptr),
    m_particleSystem(nullptr),
    m_langTable(nullptr),
    m_profiler(nullptr),
    m_globalWorld(nullptr),
    m_location(nullptr),
    m_locationId(-1),
    m_camera(nullptr),
    m_server(nullptr),
    m_clientToServer(nullptr),
    m_renderer(nullptr),
    m_locationInput(nullptr),
    m_effectProcessor(nullptr),
    m_taskManager(nullptr),
    m_script(nullptr),
    m_startSequence(nullptr),
    m_gameMenu(nullptr),
    m_negativeRenderer(false),
    m_difficultyLevel(0),
    m_largeMenus(false),
    m_paused(false),
    m_editing(false),
    m_requestedLocationId(-1),
    m_requestToggleEditing(false),
    m_requestQuit(false),
    m_levelReset(false),
    m_atMainMenu(false),
    m_gameMode(GameModeNone)
{
  g_app = this;

  // Load resources

  m_resource = new Resource();

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
  m_soundSystem = new SoundSystem();
  m_clientToServer = new ClientToServer();
  m_userInput = new UserInput();

  m_camera = new Camera();
  m_gameMenu = new GameMenu();

  strcpy(m_gameDataFile, "game.txt");

  //
  // Determine default language if possible

  char* language = g_prefsManager->GetString("TextLanguage");
  if (_stricmp(language, "unknown") == 0)
  {
    char* defaultLang = g_systemInfo->m_localeInfo.m_language;
    char langFilename[512];
    sprintf(langFilename, "%slanguage\\%s.txt", FileSys::GetHomeDirectoryA().c_str(), defaultLang);
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
  SAFE_DELETE(m_gameCursor);
  SAFE_DELETE(m_renderer);
#ifdef PROFILER_ENABLED
  SAFE_DELETE(m_profiler);
#endif
  SAFE_DELETE(g_prefsManager);
  SAFE_DELETE(m_resource);
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

void GameApp::SetLanguage(char* _language, bool _test)
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
#if defined(TARGET_OS_LINUX) && defined(TARGET_DEMOGAME)
  sprintf(langFilename, "language/%s_demo.txt", _language);
#else
  sprintf(langFilename, "language/%s.txt", _language);
#endif

  m_langTable = new LangTable(langFilename);

  if (_test)
    m_langTable->TestAgainstEnglish();

  //
  // Load the MOD language file if it exists

  sprintf(langFilename, "strings_%s.txt", _language);
  TextReader* modLangFile = g_app->m_resource->GetTextReader(langFilename);
  if (!modLangFile)
  {
    sprintf(langFilename, "strings_default.txt");
    modLangFile = g_app->m_resource->GetTextReader(langFilename);
  }

  if (modLangFile)
  {
    delete modLangFile;
    m_langTable->ParseLanguageFile(langFilename);
  }

  //
  // Load localised fonts if they exist

  char fontFilename[256];
  sprintf(fontFilename, "textures/speccy_font_%s.bmp", _language);
  if (!g_app->m_resource->DoesTextureExist(fontFilename))
    sprintf(fontFilename, "textures/speccy_font_normal.bmp");
  g_gameFont.Initialise(fontFilename);

  sprintf(fontFilename, "textures/editor_font_%s.bmp", _language);
  if (!g_app->m_resource->DoesTextureExist(fontFilename))
    sprintf(fontFilename, "textures/editor_font_normal.bmp");
  g_editorFont.Initialise(fontFilename);

  if (g_inputManager)
    m_langTable->RebuildTables();
}

void GameApp::SetProfileName(char* _profileName)
{
  strcpy(m_userProfileName, _profileName);

  if (_stricmp(_profileName, "AttractMode") != 0)
  {
    g_prefsManager->SetString("UserProfile", m_userProfileName);
    g_prefsManager->Save();
  }
}

#if defined(TARGET_OS_LINUX) || defined(TARGET_OS_MACOSX)
#include <sys/types.h>
#include <sys/stat.h>
#endif

const char* GameApp::GetProfileDirectory()
{
#if defined(TARGET_OS_LINUX)

  static char userdir[256]; const char* home = getenv("HOME"); if (home != NULL)
  {
    sprintf(userdir, "%s/.darwinia", home);
    mkdir(userdir, 0777);

    sprintf(userdir, "%s/.darwinia/%s/", home, DARWINIA_GAMETYPE);
    mkdir(userdir, 0777);
    return userdir;
  }
  else // Current directory if no home
    return "";

#elif defined(TARGET_OS_MACOSX)

  static char userdir[256]; const char* home = getenv("HOME"); if (home != NULL)
  {
    sprintf(userdir, "%s/Library", home);
    mkdir(userdir, 0777);

    sprintf(userdir, "%s/Library/Application Support", home);
    mkdir(userdir, 0777);

    sprintf(userdir, "%s/Library/Application Support/Darwinia", home);
    mkdir(userdir, 0777);

    sprintf(userdir, "%s/Library/Application Support/Darwinia/%s/", home, DARWINIA_GAMETYPE);
    mkdir(userdir, 0777);

    return userdir;
  }
  else // Current directory if no home
    return "";

#else
#ifdef TARGET_OS_VISTA
  if (IsRunningVista())
  {
    static char userdir[256];

    PWSTR path;
    SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &path);
    wcstombs(userdir, path, sizeof(userdir));
    CoTaskMemFree(path);

#ifdef TARGET_VISTA_DEMO2
  const char* subdir = "\\Darwinia Demo 2\\";
#else
  const char* subdir = "\\Darwinia\\";
#endif
  strncat(userdir, subdir, sizeof(userdir)); CreateDirectory(userdir); return userdir;
    }
    else
#endif // TARGET_OS_VISTA
  {
    return "";
  }
#endif
}

const char* GameApp::GetPreferencesPath()
{
  // good leak #1
  static char* path = nullptr;

  if (path == nullptr)
  {
    const char* profileDir = GetProfileDirectory();
    path = new char[strlen(profileDir) + 32];
    sprintf(path, "%spreferences.txt", profileDir);
  }

  return path;
}

const char* GameApp::GetScreenshotDirectory()
{
#ifdef TARGET_OS_VISTA
  static char dir[MAX_PATH]; SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, dir); sprintf(dir, "%s\\", dir); return dir;
#else
  return "";
#endif
}

bool GameApp::LoadProfile()
{
  DebugTrace("Loading profile %s\n", m_userProfileName);

  if ((_stricmp(m_userProfileName, "AccessAllAreas") == 0 || _stricmp(m_userProfileName, "AttractMode") == 0) && g_app->m_gameMode !=
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

  strcpy(m_gameDataFile, "game_demo2.txt");
  LoadProfile();

  m_requestedLocationId = m_globalWorld->GetLocationId("launchpad");
  GlobalLocation* gloc = m_globalWorld->GetLocation(m_requestedLocationId);
  strcpy(m_requestedMap, gloc->m_mapFilename);
  strcpy(m_requestedMission, gloc->m_missionFilename);

  m_atMainMenu = false;

  g_prefsManager->SetInt("RenderSpecialLighting", 1);
  g_prefsManager->SetInt("CurrentGameMode", 0);
  g_prefsManager->Save();
}

void GameApp::LoadCampaign()
{
  m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

  //m_atMainMenu = false;

  strcpy(m_gameDataFile, "game.txt");
  LoadProfile();
  m_gameMode = GameModeCampaign;
  m_requestedLocationId = -1;
  g_prefsManager->SetInt("RenderSpecialLighting", 0);
  g_prefsManager->SetInt("CurrentGameMode", 1);
  g_prefsManager->Save();
}
