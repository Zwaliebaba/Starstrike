#include "pch.h"
#include "NetLobbyClient.h"
#include "Campaign.h"
#include "FormatUtil.h"
#include "NetAuth.h"
#include "NetChat.h"
#include "NetClientConfig.h"
#include "NetHost.h"
#include "NetLayer.h"
#include "NetLink.h"
#include "NetMsg.h"
#include "NetPeer.h"
#include "Player.h"
#include "Starshatter.h"

extern const char* g_versionInfo;

NetLobbyClient::NetLobbyClient()
  : NetLobby(false),
    server_id(0),
    host(false),
    exit_code(0),
    temporary(false)
{
  NetHost me;
  std::string server_name;
  WORD port = 11101;

  ping_req_time = 0;
  chat_req_time = 0;
  user_req_time = 0;
  camp_req_time = 0;
  unit_req_time = 0;
  mods_req_time = 0;

  NetClientConfig* ncc = NetClientConfig::GetInstance();
  if (ncc)
  {
    NetServerInfo* info = ncc->GetSelectedServer();

    if (info)
    {
      server_name = info->hostname;
      addr = info->addr;
      port = info->port;
      gamepass = info->password;
    }
  }

  if (server_name.length() && port > 0)
  {
    DebugTrace("  '%s' is a client of '%s'\n", me.Name(), server_name.data());
    link = NEW NetLink;
    server_id = link->AddPeer(NetAddr(server_name, port));
  }
  else if (port == 0)
    DebugTrace("  '%s' invalid lobby port number %d\n", me.Name(), port);
  else
    DebugTrace("  '%s' is a client without a server\n", me.Name());
}

NetLobbyClient::NetLobbyClient(const NetAddr& server_addr)
  : NetLobby(true),
    server_id(0),
    addr(server_addr),
    host(false),
    exit_code(0),
    temporary(true)
{
  ping_req_time = 0;
  chat_req_time = 0;
  user_req_time = 0;
  camp_req_time = 0;
  unit_req_time = 0;
  mods_req_time = 0;

  if (addr.IPAddr() != 0)
  {
    link = NEW NetLink;
    server_id = link->AddPeer(addr);
  }
}

NetLobbyClient::~NetLobbyClient() { missions.destroy(); }

void NetLobbyClient::SendData(int type, std::string msg)
{
  if (link && server_id && type > 0 && type < 255)
  {
    if (msg.length())
      link->SendMessage(server_id, static_cast<BYTE>(type), msg.data(), msg.length(), NetMsg::RELIABLE);
    else
      link->SendMessage(server_id, static_cast<BYTE>(type), nullptr, 0, NetMsg::RELIABLE);
  }
}

void NetLobbyClient::ExecFrame()
{
  NetLobby::ExecFrame();

  if (!temporary)
  {
    // check health of server:
    NetPeer* s = link->FindPeer(server_id);
    if (s && (NetLayer::GetUTC() - s->LastReceiveTime() > NET_DISCONNECT_TIME))
      exit_code = 2;

    // send keep alive ping:
    if ((NetLayer::GetUTC() - ping_req_time > NET_DISCONNECT_TIME / 3))
    {
      SendData(NET_LOBBY_SERVER_INFO, std::string());
      ping_req_time = NetLayer::GetUTC();
    }
  }
}

bool NetLobbyClient::Login(bool host_req)
{
  Player* player = Player::GetCurrentPlayer();
  if (!player)
    return false;

  host = host_req;

  std::string login = "name \"";
  login += SafeQuotes(player->Name().c_str());
  login += "\" pass \"";
  login += SafeQuotes(player->Password().c_str());
  login += "\" version \"";
  login += g_versionInfo;
  login += "\" ";

  char buffer[256];

  sprintf_s(buffer, "host %s rank %d time %d miss %d kill %d loss %d ", host ? "true" : "false", player->Rank(),
            player->FlightTime(), player->Missions(), player->Kills(), player->Losses());

  login += buffer;

  login += "sig \"";
  login += SafeQuotes(player->Signature().c_str());
  login += "\" squad \"";
  login += SafeQuotes(player->Squadron().c_str());
  login += "\" ";

  if (gamepass.length() > 0)
  {
    login += "gamepass \"";
    login += SafeQuotes(gamepass);
    login += "\" ";
  }

  SendData(NET_LOBBY_LOGIN, login);
  ExecFrame();
  return true;
}

bool NetLobbyClient::Logout()
{
  if (host)
    GameStop();

  SendData(NET_LOBBY_LOGOUT, std::string());
  Sleep(250);
  ExecFrame();
  return true;
}

bool NetLobbyClient::Ping()
{
  std::string no_data;

  SendData(NET_LOBBY_PING, no_data);
  Sleep(100);

  SendData(NET_LOBBY_PING, no_data);
  Sleep(100);

  SendData(NET_LOBBY_PING, no_data);
  Sleep(100);

  SendData(NET_LOBBY_SERVER_INFO, no_data);
  Sleep(700);
  ExecFrame();

  return (server_info.status > NetServerInfo::OFFLINE);
}

void NetLobbyClient::GameStart()
{
  SendData(NET_LOBBY_GAME_START, std::string());
  Sleep(100);

  SetStatus(NetServerInfo::ACTIVE);
  Starshatter::GetInstance()->SetGameMode(Starshatter::PREP_MODE);

  // discard unit map selection data so that
  // it will be refreshed when we return to
  // the lobby after the mission:

  ClearUnitMap();
}

void NetLobbyClient::GameStop()
{
  SendData(NET_LOBBY_GAME_STOP, std::string());
  ExecFrame();
  Sleep(100);

  SetStatus(NetServerInfo::LOBBY);
}

const std::string& NetLobbyClient::GetMachineInfo()
{
  if (server_info.status > NetServerInfo::OFFLINE)
    return server_info.machine_info;

  return NetLobby::GetMachineInfo();
}

int NetLobbyClient::GetStatus() const
{
  if (server_info.status > NetServerInfo::OFFLINE)
    return server_info.status;

  return NetLobby::GetStatus();
}

int NetLobbyClient::NumUsers()
{
  if (server_info.status > NetServerInfo::OFFLINE)
    return server_info.nplayers;

  return NetLobby::NumUsers();
}

bool NetLobbyClient::HasHost()
{
  if (server_info.status > NetServerInfo::OFFLINE)
    return server_info.hosted ? true : false;

  return NetLobby::HasHost();
}

WORD NetLobbyClient::GetGamePort()
{
  if (server_info.status > NetServerInfo::OFFLINE)
    return server_info.gameport;

  return NetLobby::GetGamePort();
}

void NetLobbyClient::AddChat(NetUser* user, std::string_view msg, bool route)
{
  if (msg.empty())
    return;

  char buffer[280];
  sprintf_s(buffer, "msg \"%s\"", SafeQuotes(msg).c_str());

  SendData(NET_LOBBY_CHAT, buffer);
  ExecFrame();
}

List<NetChatEntry>& NetLobbyClient::GetChat()
{
  if (chat_log.size() < 1 && (NetLayer::GetUTC() - chat_req_time > 3))
  {
    SendData(NET_LOBBY_CHAT, std::string());
    chat_req_time = NetLayer::GetUTC();
  }

  return chat_log;
}

List<NetUser>& NetLobbyClient::GetUsers()
{
  if (users.size() < 1 && (NetLayer::GetUTC() - user_req_time > 2))
  {
    SendData(NET_LOBBY_USER_LIST, std::string());
    user_req_time = NetLayer::GetUTC();
  }

  return users;
}

List<NetUnitEntry>& NetLobbyClient::GetUnitMap()
{
  bool request = selected_mission && unit_map.size() < 1 && (NetLayer::GetUTC() - unit_req_time > 2);

  if (selected_mission && GetStatus() == NetServerInfo::ACTIVE && (NetLayer::GetUTC() - unit_req_time > 5))
    request = true;

  if (request)
  {
    SendData(NET_LOBBY_UNIT_LIST, std::string());
    unit_req_time = NetLayer::GetUTC();
  }

  return unit_map;
}

List<NetCampaignInfo>& NetLobbyClient::GetCampaigns()
{
  if (campaigns.size() < 1 && (NetLayer::GetUTC() - camp_req_time > 3))
  {
    SendData(NET_LOBBY_MISSION_LIST, std::string());
    camp_req_time = NetLayer::GetUTC();
  }

  return campaigns;
}

void NetLobbyClient::BanUser(NetUser* user)
{
  char buffer[512];
  sprintf_s(buffer, "user \"%s\"", SafeQuotes(user->Name()).c_str());
  SendData(NET_LOBBY_BAN_USER, buffer);
}

void NetLobbyClient::SelectMission(DWORD id)
{
  char buffer[32];
  sprintf_s(buffer, "m_id 0x%08x", id);
  SendData(NET_LOBBY_MISSION_SELECT, buffer);
}

void NetLobbyClient::MapUnit(int n, std::string_view user, bool lock)
{
  if (user.empty() || user.length() > 250)
    return;

  char buffer[512];
  sprintf_s(buffer, "id %d user \"%s\" lock %s", n, SafeQuotes(user).c_str(), lock ? "true" : "false");
  SendData(NET_LOBBY_MAP_UNIT, buffer);
}

Mission* NetLobbyClient::GetSelectedMission()
{
  mission = nullptr;

  // ask server for mission:
  SendData(NET_LOBBY_MISSION_DATA, std::string());

  // wait for answer:
  int i = 150;
  while (i-- > 0 && !mission)
  {
    Sleep(100);
    ExecFrame();
  }

  return mission;
}

void NetLobbyClient::DoAuthUser(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  int level = NetAuth::NET_AUTH_STANDARD;
  std::string salt;

  for (int i = 0; i < params.size(); i++)
  {
    NetLobbyParam* p = params[i];

    int num = 0;
    sscanf_s(p->value.c_str(), "%d", &num);

    if (p->name == "level")
      level = num;

    else if (p->name == "salt")
      salt = p->value;
  }

  std::string response = NetAuth::CreateAuthResponse(level, salt);
  if (!response.empty())
    SendData(NET_LOBBY_USER_AUTH, response);
}

void NetLobbyClient::DoServerInfo(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  for (int i = 0; i < params.size(); i++)
  {
    NetLobbyParam* p = params[i];

    int num = 0;
    sscanf_s(p->value.c_str(), "%d", &num);

    if (p->name == "info")
      server_info.machine_info = p->value;

    else if (p->name == "version")
      server_info.version = p->value;

    else if (p->name == "mode")
      server_info.status = num;

    else if (p->name == "users")
      server_info.nplayers = num;

    else if (p->name == "host")
      server_info.hosted = (p->value == "true");

    else if (p->name == "port")
      server_info.gameport = static_cast<WORD>(num);
  }

  params.destroy();
}

void NetLobbyClient::DoChat(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  int id = 0;
  std::string user_name;
  std::string chat_msg;

  for (int i = 0; i < params.size(); i++)
  {
    NetLobbyParam* p = params[i];

    int num = 0;
    sscanf_s(p->value.c_str(), "%d", &num);

    if (p->name == "id")
      id = num;

    else if (p->name == "user")
      user_name = p->value;

    else if (p->name == "msg")
      chat_msg = p->value;
  }

  params.destroy();

  // receive chat from server:
  if (id && chat_msg.length())
  {
    auto entry = NEW NetChatEntry(id, user_name, chat_msg);

    if (!chat_log.contains(entry))
      chat_log.insertSort(entry);
    else
      delete entry; // received duplicate
  }
}

void NetLobbyClient::DoUserList(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  users.destroy();

  std::string user_name;
  std::string host_flag;
  std::string signature;
  std::string squadron;
  int rank = 0;
  int flight_time = 0;
  int mission_count = 0;
  int kills = 0;
  int losses = 0;

  for (int i = 0; i < params.size(); i++)
  {
    NetLobbyParam* p = params[i];

    int num = 0;
    sscanf_s(p->value.c_str(), "%d", &num);

    if (p->name == "name")
      user_name = p->value;

    else if (p->name == "sig")
      signature = p->value;

    else if (p->name == "squad")
      squadron = p->value;

    else if (p->name == "rank")
      rank = num;

    else if (p->name == "time")
      flight_time = num;

    else if (p->name == "miss")
      mission_count = num;

    else if (p->name == "kill")
      kills = num;

    else if (p->name == "loss")
      losses = num;

    else if (p->name == "host")
    {
      host_flag = p->value;

      auto u = NEW NetUser(user_name);
      u->SetHost((host_flag == "true") ? true : false);
      u->SetSignature(signature);
      u->SetSquadron(squadron);
      u->SetRank(rank);
      u->SetFlightTime(flight_time);
      u->SetMissions(mission_count);
      u->SetKills(kills);
      u->SetLosses(losses);

      AddUser(u);
    }
  }

  params.destroy();
}

void NetLobbyClient::DoMissionList(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  if (params.size() > 2)
  {
    campaigns.destroy();

    NetCampaignInfo* c = nullptr;
    MissionInfo* m = nullptr;

    for (int i = 0; i < params.size(); i++)
    {
      NetLobbyParam* p = params[i];

      if (p->name == "c_id")
      {
        c = NEW NetCampaignInfo;
        sscanf_s(p->value.c_str(), "0x%x", &c->id);
        campaigns.append(c);

        m = nullptr;
      }

      else if (c && p->name == "c_name")
        c->name = p->value;

      else if (p->name == "m_id")
      {
        int id = 0;
        sscanf_s(p->value.c_str(), "0x%x", &id);

        int m_id = id & NET_MISSION_MASK;
        int c_id = id >> NET_CAMPAIGN_SHIFT;

        for (int i = 0; i < campaigns.size(); i++)
        {
          NetCampaignInfo* c = campaigns[i];
          if (c->id == c_id)
          {
            m = NEW MissionInfo;
            m->m_id = m_id;
            c->missions.append(m);
            missions.append(m);  // for later garbage collection
            break;
          }
        }
      }

      else if (m && p->name == "m_name")
        m->m_name = p->value;

      else if (m && p->name == "m_desc")
        m->description = p->value;
    }
  }

  params.destroy();
}

void NetLobbyClient::DoMissionSelect(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  for (int i = 0; i < params.size(); i++)
  {
    NetLobbyParam* p = params[i];

    int num = 0;
    sscanf_s(p->value.c_str(), "0x%x", &num);

    if (p->name == "m_id")
    {
      if (selected_mission != static_cast<DWORD>(num))
      {
        selected_mission = num;
        ClearUnitMap();
      }
    }
  }

  params.destroy();
}

void NetLobbyClient::DoMissionData(NetPeer* peer, std::string msg)
{
  Campaign* c = Campaign::SelectCampaign("Multiplayer Missions");

  if (c)
  {
    c->LoadNetMission(99999, msg.data());
    mission = c->GetMission(99999);
  }

  if (msg.length())
  {
    FILE* f;
    fopen_s(&f, "multi_mission_recv.def", "w");
    if (f)
    {
      fwrite(msg.data(), msg.length(), 1, f);
      fclose(f);
    }
  }
}

void NetLobbyClient::DoUnitList(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  if (params.size() > 2)
  {
    ClearUnitMap();

    std::string elem_name;
    std::string design;
    std::string user_name;
    int iff = 0;
    int index = 0;
    int lives = 1;
    int hull = 100;
    int role = 0;
    int lock = 0;

    for (int i = 0; i < params.size(); i++)
    {
      NetLobbyParam* p = params[i];

      if (p->name == "name")
        elem_name = p->value;
      else if (p->name == "design")
        design = p->value;
      else if (p->name == "user")
        user_name = p->value;
      else if (p->name == "index")
        sscanf_s(p->value.c_str(), "%d", &index);
      else if (p->name == "iff")
        sscanf_s(p->value.c_str(), "%d", &iff);
      else if (p->name == "lives")
        sscanf_s(p->value.c_str(), "%d", &lives);
      else if (p->name == "hull")
        sscanf_s(p->value.c_str(), "%d", &hull);
      else if (p->name == "role")
        sscanf_s(p->value.c_str(), "%d", &role);
      else if (p->name == "lock")
      {
        sscanf_s(p->value.c_str(), "%d", &lock);

        auto entry = NEW NetUnitEntry(elem_name, design, iff, index);
        entry->SetUserName(user_name);
        entry->SetLives(lives);
        entry->SetIntegrity(hull);
        entry->SetMissionRole(role);
        entry->SetLock(lock ? true : false);

        unit_map.append(entry);
      }
    }
  }

  params.destroy();
}

void NetLobbyClient::DoMapUnit(NetPeer* peer, std::string msg) {}

void NetLobbyClient::DoGameStart(NetPeer* peer, std::string msg) {}

void NetLobbyClient::DoExit(NetPeer* peer, std::string msg) { exit_code = 1; }
