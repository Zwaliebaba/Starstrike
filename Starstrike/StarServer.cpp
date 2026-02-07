#include "pch.h"
#include "StarServer.h"
#include "Campaign.h"
#include "CombatRoster.h"
#include "DataLoader.h"
#include "Drive.h"
#include "Explosion.h"
#include "FlightDeck.h"
#include "Galaxy.h"
#include "Game.h"
#include "HttpServer.h"
#include "Keyboard.h"
#include "Mission.h"
#include "NetAdminServer.h"
#include "NetGame.h"
#include "NetLayer.h"
#include "NetLobbyServer.h"
#include "NetServer.h"
#include "NetServerConfig.h"
#include "RadioTraffic.h"
#include "Random.h"
#include "Ship.h"
#include "Shot.h"
#include "Sim.h"
#include "SystemDesign.h"
#include "WeaponDesign.h"

StarServer* StarServer::instance = nullptr;

static Mission* current_mission = nullptr;

extern const char* g_versionInfo;

StarServer::StarServer()
  : admin_server(nullptr),
    lobby_server(nullptr),
    loader(nullptr),
    game_mode(MENU_MODE),
    time_mark(0),
    minutes(0)
{
  if (!instance)
    instance = this;

  app_name = L"Starserver 5.0";
  title_text = "Starserver";
  palette_name = "alpha";

  server = true;
  show_mouse = true;

  DataLoader::Initialize();
  loader = DataLoader::GetLoader();

  // no images or sounds in server mode:
  loader->EnableMedia(false);
}

StarServer::~StarServer()
{
  delete admin_server;
  delete lobby_server;

  admin_server = nullptr;
  lobby_server = nullptr;

  // delete all the ships and stuff
  // BEFORE getting rid of the system
  // and weapons catalogs!
  delete world;
  world = nullptr; // don't let base class double delete the world

  Drive::Close();
  Explosion::Close();
  FlightDeck::Close();
  Campaign::Close();
  CombatRoster::Close();
  Galaxy::Close();
  RadioTraffic::Close();
  Ship::Close();
  WeaponDesign::Close();
  SystemDesign::Close();
  DataLoader::Close();
  NetServerConfig::Close();

  instance = nullptr;

  server = false;
}

bool StarServer::Init(HINSTANCE hi, HINSTANCE hpi, LPSTR cmdline, int nCmdShow)
{
  return Game::Init(hi, hpi, cmdline, nCmdShow);
}

bool StarServer::InitGame()
{
  if (!Game::InitGame())
    return false;

  RandomInit();
  NetServerConfig::Initialize();
  SystemDesign::Initialize("sys.def");
  WeaponDesign::Initialize("wep.def");
  Ship::Initialize();
  Galaxy::Initialize();
  CombatRoster::Initialize();
  Campaign::Initialize();

  Drive::Initialize();
  Explosion::Initialize();
  FlightDeck::Initialize();
  Ship::Initialize();
  Shot::Initialize();
  RadioTraffic::Initialize();

  time_mark = GameTime();
  minutes = 0;

  NetServerConfig* server_config = NetServerConfig::GetInstance();
  if (!server_config)
    return false;

  DebugTrace("\n\n\nStarshatter Server Init\n");
  DebugTrace("-----------------------\n");
  DebugTrace("Server Name:       %s\n", (server_config->Name()));
  DebugTrace("Server Type:       %d\n", server_config->GetGameType());

  if (server_config->GetMission().length() > 0)
    DebugTrace("Server Mission:    %s\n", (server_config->GetMission()));

  DebugTrace("Lobby Server Port: %d\n", server_config->GetLobbyPort());
  DebugTrace("Admin Server Port: %d\n", server_config->GetAdminPort());
  DebugTrace("-----------------------\n");

  auto nls = NEW NetLobbyServer;
  NetAdminServer* nas = NetAdminServer::GetInstance(server_config->GetAdminPort());
  nas->SetServerName(server_config->Name());

  lobby_server = nls;
  admin_server = nas;

  return true;
}

void StarServer::SetGameMode(int m)
{
  if (game_mode == m)
    return;

  if (m == LOAD_MODE)
  {
    DebugTrace("  game_mode = LOAD_MODE\n");
    paused = true;
  }

  else if (m == PLAY_MODE)
  {
    DebugTrace("  game_mode = PLAY_MODE\n");

    if (!world)
    {
      CreateWorld();
      InstantiateMission();
    }

    // stand alone server should wait for players to connect
    // before unpausing the simulation...
    SetTimeCompression(1);
    Pause(true);
  }

  else if (m == MENU_MODE)
  {
    DebugTrace("  game_mode = MENU_MODE\n");
    paused = true;

    auto sim = static_cast<Sim*>(world);

    if (sim)
      sim->UnloadMission();
  }

  game_mode = m;
}

void StarServer::SetNextMission(std::string_view script)
{
  if (lobby_server)
    lobby_server->SetServerMission(script);
}

void StarServer::CreateWorld()
{
  RadioTraffic::Initialize();

  // create world
  if (!world)
  {
    auto sim = NEW Sim(nullptr);
    world = sim;
    DebugTrace("  World Created.\n");
  }
}

void StarServer::InstantiateMission()
{
  current_mission = nullptr;

  if (Campaign::GetCampaign())
    current_mission = Campaign::GetCampaign()->GetMission();

  auto sim = static_cast<Sim*>(world);

  if (sim)
  {
    sim->UnloadMission();

    if (current_mission)
    {
      sim->LoadMission(current_mission);
      sim->ExecMission();
      sim->SetTestMode(false);

      DebugTrace("  Mission Instantiated.\n");
    }

    else
      DebugTrace("  *** WARNING: StarServer::InstantiateMission() - no mission selected ***\n");
  }
}

bool StarServer::GameLoop()
{
  if (active && paused)
  {
    UpdateWorld();
    GameState();
  }

  else if (!active)
  {
    UpdateWorld();
    GameState();
    Sleep(10);
  }

  Game::GameLoop();
  return false;  // must return false to keep processing
  // true tells the outer loop to sleep until a
  // windows event is available
}

void StarServer::UpdateWorld()
{
  long new_time = real_time;
  double delta = new_time - frame_time;
  seconds = max_frame_length;
  gui_seconds = delta * 0.001;

  if (frame_time == 0)
    gui_seconds = 0;

  time_comp = 1;

  if (delta < max_frame_length * 1000)
    seconds = delta * 0.001;

  frame_time = new_time;

  Galaxy* galaxy = Galaxy::GetInstance();
  if (galaxy)
    galaxy->ExecFrame();

  Campaign* campaign = Campaign::GetCampaign();
  if (campaign)
    campaign->ExecFrame();

  if (paused)
  {
    if (world)
      world->ExecFrame(0);
  }

  else
  {
    game_time += static_cast<DWORD>(seconds * 1000);

    Drive::StartFrame();

    if (world)
      world->ExecFrame(seconds);
  }

  static DWORD refresh_time = 0;
  if (RealTime() - refresh_time > 1000)
  {
    refresh_time = RealTime();
    RedrawWindow(hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE);
  }
}

void StarServer::GameState()
{
  if (lobby_server)
  {
    lobby_server->ExecFrame();

    if (lobby_server->GetStatus() == NetServerInfo::PERSISTENT)
      paused = NetGame::NumPlayers() < 1;
  }

  if (game_mode == MENU_MODE)
    Sleep(30);

  else if (game_mode == LOAD_MODE)
  {
    CreateWorld();
    InstantiateMission();

    SetGameMode(PLAY_MODE);
  }

  else if (game_mode == PLAY_MODE)
  {
    if (GameTime() - time_mark > 60000)
    {
      time_mark = GameTime();
      minutes++;
      if (minutes > 60)
        DebugTrace("  TIME %2d:%02d:00\n", minutes / 60, minutes % 60);
      else
        DebugTrace("  TIME    %2d:00\n", minutes);
    }

    Sleep(10);
  }
}

bool StarServer::OnPaint()
{
  PAINTSTRUCT paintstruct;
  HDC hdc = BeginPaint(hwnd, &paintstruct);

  std::string txt_title = title_text;
  std::string txt_mode;
  std::string txt_users = GetText("server.no-users");
  char buf[256];

  txt_title += " ";
  txt_title += g_versionInfo;

  switch (game_mode)
  {
    case LOAD_MODE:
    case MENU_MODE:
      txt_mode = GetText("server.mode.lobby");

      if (lobby_server)
      {
        sprintf_s(buf, GetText("server.users").data(), lobby_server->NumUsers());
        txt_users = buf;
      }
      break;

    case PLAY_MODE:
      txt_mode = GetText("server.mode.active");
      if (lobby_server)
        sprintf_s(buf, GetText("server.users-and-players").data(), lobby_server->NumUsers(), NetGame::NumPlayers());
      else
        sprintf_s(buf, GetText("server.players").data(), NetGame::NumPlayers());
      txt_users = buf;
      break;

    default:
      txt_mode = GetText("server.mode.other");
      break;
  }

  if (lobby_server && lobby_server->GetStatus() == NetServerInfo::PERSISTENT)
    txt_mode += " " + GetText("server.alt.persistent");

  if (paused)
    txt_mode += " " + GetText("server.alt.paused");

  TextOutA(hdc, 4, 4, txt_title.c_str(), txt_title.length());
  TextOutA(hdc, 4, 22, txt_mode.c_str(), txt_mode.length());
  TextOutA(hdc, 4, 40, txt_users.c_str(), txt_users.length());

  Sim* sim = Sim::GetSim();
  if (sim && sim->GetMission())
  {
    Mission* mission = sim->GetMission();
    std::string txt_msn = GetText("server.mission");
    txt_msn += mission->Name();
    TextOutA(hdc, 4, 58, txt_msn.c_str(), txt_msn.length());
  }

  EndPaint(hwnd, &paintstruct);
  return true;
}

DWORD WINAPI StarServerShutdownProc(LPVOID link)
{
  auto stars = static_cast<StarServer*>(link);

  Sleep(3000);

  if (stars)
  {
    stars->Exit();
    return 0;
  }

  return static_cast<DWORD>(E_POINTER);
}

DWORD WINAPI StarServerRestartProc(LPVOID link)
{
  auto stars = static_cast<StarServer*>(link);

  Sleep(3000);

  if (stars)
  {
    char cmdline[256];
    strcpy_s(cmdline, "stars -server");

    STARTUPINFOA s = {};
    s.cb = sizeof(s);

    PROCESS_INFORMATION pi = {};

    CreateProcessA("stars.exe", cmdline, nullptr, nullptr, 0, 0, nullptr, nullptr, &s, &pi);
    stars->Exit();
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
  }

  return static_cast<DWORD>(E_POINTER);
}

void StarServer::Shutdown(bool restart)
{
  DWORD thread_id = 0;

  if (restart)
    CreateThread(nullptr, 4096, StarServerRestartProc, this, 0, &thread_id);
  else
    CreateThread(nullptr, 4096, StarServerShutdownProc, this, 0, &thread_id);
}
