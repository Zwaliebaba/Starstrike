#include "pch.h"
#include "NetLobbyServer.h"
#include "Campaign.h"
#include "FormatUtil.h"
#include "Game.h"
#include "MachineInfo.h"
#include "Mission.h"
#include "NetAuth.h"
#include "NetBrokerClient.h"
#include "NetChat.h"
#include "NetGame.h"
#include "NetHost.h"
#include "NetLayer.h"
#include "NetMsg.h"
#include "NetPeer.h"
#include "NetPlayer.h"
#include "NetServerConfig.h"
#include "NetUser.h"
#include "NetUtil.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "StarServer.h"
#include "Starshatter.h"

extern const char* g_versionInfo;

static NetLobbyServer* net_lobby_server = nullptr;

NetLobbyServer::NetLobbyServer()
  : announce_time(0),
    m_serverConfig(nullptr),
    m_motdIndex(1)
{
  status = NetServerInfo::LOBBY;
  m_serverName = std::string("Starshatter NetLobbyServer ") + g_versionInfo;
  start_time = NetLayer::GetUTC();

  selected_mission = 0;

  m_serverConfig = NetServerConfig::GetInstance();
  if (m_serverConfig)
  {
    WORD serverPort;
    m_serverName = m_serverConfig->Name();
    serverPort = m_serverConfig->GetLobbyPort();
    server_mission = m_serverConfig->GetMission();

    NetAuth::SetAuthLevel(m_serverConfig->GetAuthLevel());

    m_serverAddr = NetAddr(NetHost().Address().IPAddr(), serverPort);
    link = NEW NetLink(m_serverAddr);
  }

  NetLobbyServer::LoadMOTD();

  StarServer* star_server = StarServer::GetInstance();
  DWORD mission_id = 0;

  // only one mission:
  if (star_server && server_mission.length() > 0)
  {
    auto c = NEW NetCampaignInfo;
    c->id = Campaign::MULTIPLAYER_MISSIONS;
    c->name = "Persistent Multiplayer";
    campaigns.append(c);

    ListIter<Campaign> c_iter = Campaign::GetAllCampaigns();
    while (++c_iter && !mission_id)
    {
      Campaign* campaign = c_iter.value();

      if (campaign->GetCampaignId() == Campaign::MULTIPLAYER_MISSIONS)
      {
        ListIter<MissionInfo> m_iter = campaign->GetMissionList();
        while (++m_iter && !mission_id)
        {
          MissionInfo* m = m_iter.value();

          if (m->script == server_mission)
          {
            c->missions.append(m);
            mission_id = (Campaign::MULTIPLAYER_MISSIONS << NET_CAMPAIGN_SHIFT) + m->m_id;

            SelectMission(mission_id);
            star_server->SetGameMode(StarServer::LOAD_MODE);

            // lock in mission:
            SetStatus(NetServerInfo::PERSISTENT);
          }
        }
      }
    }
  }

  // player host may select mission:
  if (!mission_id)
  {
    campaigns.destroy();

    ListIter<Campaign> c_iter = Campaign::GetAllCampaigns();
    while (++c_iter)
    {
      Campaign* campaign = c_iter.value();

      if (campaign->GetCampaignId() >= Campaign::MULTIPLAYER_MISSIONS)
      {
        auto c = NEW NetCampaignInfo;
        c->id = campaign->GetCampaignId();
        c->name = campaign->Name();
        campaigns.append(c);

        ListIter<MissionInfo> m_iter = campaign->GetMissionList();
        while (++m_iter)
        {
          MissionInfo* m = m_iter.value();
          c->missions.append(m);
        }
      }
    }
  }

  net_lobby_server = this;
}

NetLobbyServer::~NetLobbyServer()
{
  ListIter<NetUser> iter = users;
  while (++iter)
  {
    NetUser* u = iter.value();
    SendData(u, NET_LOBBY_EXIT, std::string());
    ExecFrame();
  }

  Sleep(500);

  unit_map.destroy();
  chat_log.destroy();
  users.destroy();

  if (net_lobby_server == this)
    net_lobby_server = nullptr;
}

NetLobbyServer* NetLobbyServer::GetInstance() { return net_lobby_server; }

void NetLobbyServer::LoadMOTD()
{
  m_motd.clear();

  FILE* f;
  fopen_s(&f, "motd.txt", "r");

  if (f)
  {
    char line[256];

    while (fgets(line, 256, f))
    {
      int n = strlen(line) - 1;

      while (n >= 0 && isspace(line[n]))
        line[n--] = 0;

      m_motd.emplace_back(line);
    }
  }
}

void NetLobbyServer::SendMOTD(NetUser* user)
{
  if (m_motd.size() < 1)
    return;

  char buffer[512];

  for (int i = 0; i < m_motd.size(); i++)
  {
    std::string line = m_motd[i];

    sprintf_s(buffer, "id %d user \" \" msg \"%s\"", m_motdIndex++, line.c_str());

    SendData(user, NET_LOBBY_CHAT, buffer);
  }

  sprintf_s(buffer, "id %d user \" \" msg \" \"", m_motdIndex++);
  SendData(user, NET_LOBBY_CHAT, buffer);
}

void NetLobbyServer::ExecFrame()
{
  NetLobby::ExecFrame();

  if (announce_time == 0 || Game::RealTime() - announce_time > 200000)
  {
    GameOn();
    announce_time = Game::RealTime();
  }

  if (GetStatus() == NetServerInfo::BRIEFING)
  {
    NetGame* net_game = NetGame::GetInstance();

    if (net_game && NetGame::NumPlayers() > 0)
      SetStatus(NetServerInfo::ACTIVE);
  }

  StarServer* star_server = StarServer::GetInstance();
  DWORD mission_id = 0;

  // restart persistent mission?
  if (star_server && star_server->GetGameMode() == StarServer::MENU_MODE && !server_mission.empty())
  {
    NetCampaignInfo* c = campaigns.last();

    if (!c || c->name != "Persistent Multiplayer")
    {
      c = NEW NetCampaignInfo;
      c->id = Campaign::MULTIPLAYER_MISSIONS;
      c->name = "Persistent Multiplayer";
      campaigns.append(c);
    }
    else
      c->missions.clear();

    ListIter<Campaign> c_iter = Campaign::GetAllCampaigns();
    while (++c_iter && !mission_id)
    {
      Campaign* campaign = c_iter.value();

      if (campaign->GetCampaignId() == Campaign::MULTIPLAYER_MISSIONS)
      {
        ListIter<MissionInfo> m_iter = campaign->GetMissionList();
        while (++m_iter && !mission_id)
        {
          MissionInfo* m = m_iter.value();

          if (m->script == server_mission)
          {
            c->missions.append(m);
            mission_id = (Campaign::MULTIPLAYER_MISSIONS << NET_CAMPAIGN_SHIFT) + m->m_id;

            // unlock old mission:
            SetStatus(NetServerInfo::LOBBY);

            SelectMission(mission_id);

            if (star_server->GetGameMode() == StarServer::MENU_MODE)
              star_server->SetGameMode(StarServer::LOAD_MODE);

            // lock in new mission:
            SetStatus(NetServerInfo::PERSISTENT);
          }
        }
      }
    }
  }

  CheckSessions();
}

void NetLobbyServer::SendData(NetUser* dst, int type, std::string msg)
{
  if (link && dst && type > 0 && type < 255)
  {
    if (msg.length())
      link->SendMessage(dst->GetNetID(), static_cast<BYTE>(type), msg.data(), msg.length(), NetMsg::RELIABLE);
    else
      link->SendMessage(dst->GetNetID(), static_cast<BYTE>(type), nullptr, 0, NetMsg::RELIABLE);
  }
}

void NetLobbyServer::CheckSessions()
{
  if (!link)
    return;

  bool dropped = false;

  ListIter<NetUser> u_iter = users;
  while (++u_iter)
  {
    NetUser* u = u_iter.value();
    NetPeer* p = link->FindPeer(u->GetNetID());

    if (p && (NetLayer::GetUTC() - p->LastReceiveTime()) > NET_DISCONNECT_TIME)
    {
      // check game peer for activity:
      NetGame* game = NetGame::GetInstance();
      NetPlayer* player = nullptr;
      NetPeer* p2 = nullptr;

      if (game)
      {
        player = game->FindPlayerByName(u->Name());

        if (player)
        {
          p2 = game->GetPeer(player);

          if (p2 && (NetLayer::GetUTC() - p2->LastReceiveTime()) < NET_DISCONNECT_TIME)
          {
            p->SetLastReceiveTime(p2->LastReceiveTime());
            continue;
          }
        }
        else
          DebugTrace("NetLobbyServer::CheckSessions() Could not find player for '%s'\n", u->Name().data());
      }
      else
        DebugTrace("NetLobbyServer::CheckSessions() Could not find net game for '%s'\n", u->Name().data());

      // announce drop:
      char timestr[64];
      FormatTime(timestr, Game::RealTime() / 1000);
      DebugTrace("NetLobbyServer: Dropped inactive connection '%s' %s\n", u->Name().data(), timestr);

      if (u->IsHost())
      {
        DebugTrace("              User was host - ending net game.\n");
        GameStop();
      }

      u_iter.removeItem();             // first remove user from list
      NetLobby::UnmapUnit(u->Name());  // then unmap unit
      delete u;                        // now it is safe to discard the inactive user

      dropped = true;
    }
  }

  if (dropped)
  {
    SendUsers();
    SendUnits();
  }
}

void NetLobbyServer::GameStart()
{
  if (status < NetServerInfo::ACTIVE)
  {
    SetStatus(NetServerInfo::BRIEFING);

    if (Starshatter::GetInstance())
      Starshatter::GetInstance()->SetGameMode(Starshatter::PREP_MODE);
    else
    {
      StarServer* s = StarServer::GetInstance();
      if (s && s->GetGameMode() == StarServer::MENU_MODE)
        s->SetGameMode(StarServer::LOAD_MODE);
    }
  }
}

void NetLobbyServer::GameStop()
{
  if (GetStatus() != NetServerInfo::PERSISTENT)
  {
    SetStatus(NetServerInfo::LOBBY);

    StarServer* s = StarServer::GetInstance();
    if (s && s->GetGameMode() != StarServer::MENU_MODE)
      s->SetGameMode(StarServer::MENU_MODE);
  }
}

void NetLobbyServer::BanUser(NetUser* user)
{
  if (user && !user->IsHost())
  {
    DebugTrace("NetLobbyServer::BanUser name '%s' addr %d.%d.%d.%d\n", user->Name().data(), user->GetAddress().B1(),
               user->GetAddress().B2(), user->GetAddress().B3(), user->GetAddress().B4());

    SendData(user, NET_LOBBY_EXIT, std::string());

    if (m_serverConfig)
      m_serverConfig->BanUser(user);

    DelUser(user);
  }
}

void NetLobbyServer::AddUser(NetUser* user)
{
  if (m_serverConfig && m_serverConfig->IsUserBanned(user))
  {
    delete user;
    return;
  }

  NetLobby::AddUser(user);
  SendUsers();
}

void NetLobbyServer::DelUser(NetUser* user)
{
  NetLobby::DelUser(user);
  SendUsers();
}

void NetLobbyServer::SendUsers()
{
  std::string content;

  ListIter<NetUser> u_iter = users;
  while (++u_iter)
  {
    NetUser* u = u_iter.value();
    content += u->GetDescription();
  }

  u_iter.reset();

  while (++u_iter)
  {
    NetUser* u = u_iter.value();
    SendData(u, NET_LOBBY_USER_LIST, content);
  }
}

void NetLobbyServer::RequestAuth(NetUser* user)
{
  if (user)
  {
    std::string request = NetAuth::CreateAuthRequest(user);

    if (request.length() > 0)
      SendData(user, NET_LOBBY_AUTH_USER, request);
  }
}

void NetLobbyServer::AddChat(NetUser* user, std::string_view msg, bool route)
{
  NetChatEntry* entry = nullptr;

  if (user && !msg.empty())
  {
    bool msg_ok = false;
    auto p = msg.begin();

    while (p != msg.end() && !msg_ok)
    {
      if (!isspace(*p++))
        msg_ok = true;
    }

    if (msg_ok)
    {
      entry = NEW NetChatEntry(user, msg);

      chat_log.append(entry);

      // forward to all clients:
      if (users.size())
      {
        std::string msg_buf;
        std::string usr_buf;

        // safe quotes uses a static buffer,
        // so make sure to save copies of the
        // results when using more than one in
        // a function call...

        msg_buf = SafeQuotes(msg);
        usr_buf = SafeQuotes(user->Name());

        std::string buffer = std::format("id {} user \"{}\" msg \"{}\"", entry->GetID(), usr_buf.c_str(), msg_buf.c_str());

        ListIter<NetUser> iter = users;
        while (++iter)
        {
          NetUser* u = iter.value();
          SendData(u, NET_LOBBY_CHAT, buffer);
        }

        if (route)
        {
          // send to active game:
          NetUtil::SendChat(0xffff, usr_buf, msg_buf);
        }
      }
    }
  }
}

void NetLobbyServer::ClearChat() { NetLobby::ClearChat(); }

void NetLobbyServer::SaveChat()
{
  FILE* f;
  fopen_s(&f, "chat.txt", "w");
  if (f)
  {
    for (int i = 0; i < chat_log.size(); i++)
    {
      NetChatEntry* c = chat_log[i];
      fprintf(f, "%08x [%s] %s\n", c->GetTime(), c->GetUser().data(), c->GetMessage().data());
    }

    fclose(f);
  }
}

void NetLobbyServer::SelectMission(DWORD id)
{
  if (GetStatus() == NetServerInfo::PERSISTENT)
    return;

  NetLobby::SelectMission(id);

  // inform all users of the selection:
  char buffer[32];
  sprintf_s(buffer, "m_id 0x%08x", selected_mission);

  ListIter<NetUser> iter = users;
  while (++iter)
  {
    NetUser* u = iter.value();
    SendData(u, NET_LOBBY_MISSION_SELECT, buffer);
  }
}

List<NetUnitEntry>& NetLobbyServer::GetUnitMap()
{
  if (!mission)
  {
    unit_map.destroy();
    return unit_map;
  }

  List<NetUnitEntry> units;
  ListIter<MissionElement> iter = mission->GetElements();
  int i = 0;

  Sim* sim = Sim::GetSim();
  if (sim && sim->GetElements().size() > 0)
    iter = sim->GetMissionElements();

  // create new entries for the playable elements in the mission or simulation:
  while (++iter)
  {
    MissionElement* elem = iter.value();

    if (elem->IsPlayable())
    {
      NetUnitEntry* u = nullptr;
      if (elem->Count() == 1)
      {
        u = NEW NetUnitEntry(elem, 0);
        u->SetLives(elem->RespawnCount() + 1);
        u->SetMissionRole(elem->MissionRole());
        u->SetIFF(elem->GetIFF());

        if (elem->GetDesign())
          u->SetDesign(elem->GetDesign()->m_name);

        if (elem->Ships().size() > 0)
        {
          MissionShip* s = elem->Ships()[0];
          u->SetIntegrity(static_cast<int>(s->Integrity()));
        }
        units.append(u);
      }
      else
      {
        for (int i = 0; i < elem->Count(); i++)
        {
          u = NEW NetUnitEntry(elem, i + 1);
          u->SetMissionRole(elem->MissionRole());
          u->SetIFF(elem->GetIFF());

          if (elem->GetDesign())
            u->SetDesign(elem->GetDesign()->m_name);

          if (elem->Ships().size() > i)
          {
            MissionShip* s = elem->Ships()[i];
            u->SetLives(s->Respawns() + 1);
            u->SetIntegrity(static_cast<int>(s->Integrity()));
          }
          units.append(u);
        }
      }
    }
  }

  // match new entries with any existing map entries:
  if (unit_map.size())
  {
    for (i = 0; i < units.size(); i++)
    {
      NetUnitEntry* e_new = units[i];
      NetUnitEntry* e_old = unit_map.find(e_new);

      if (e_old)
      {
        e_new->SetUserName(e_old->GetUserName());
        e_new->SetLock(e_old->GetLocked());
      }
    }
  }

  // rewrite the unit map with the new entries:
  ClearUnitMap();
  for (i = 0; i < units.size(); i++)
    unit_map.append(units[i]);

  return unit_map;
}

void NetLobbyServer::MapUnit(int n, std::string_view user, bool lock)
{
  NetLobby::MapUnit(n, user, lock);

  std::string reply;

  ListIter<NetUnitEntry> map_iter = GetUnitMap();
  while (++map_iter)
  {
    NetUnitEntry* unit = map_iter.value();
    reply += unit->GetDescription();
  }

  ListIter<NetUser> u_iter = users;
  while (++u_iter)
  {
    NetUser* u = u_iter.value();
    SendData(u, NET_LOBBY_UNIT_LIST, reply);
  }
}

void NetLobbyServer::UnmapUnit(std::string_view user)
{
  NetLobby::UnmapUnit(user);

  std::string reply;

  ListIter<NetUnitEntry> map_iter = GetUnitMap();
  while (++map_iter)
  {
    NetUnitEntry* unit = map_iter.value();
    reply += unit->GetDescription();
  }

  ListIter<NetUser> u_iter = users;
  while (++u_iter)
  {
    NetUser* u = u_iter.value();
    SendData(u, NET_LOBBY_UNIT_LIST, reply);
  }
}

void NetLobbyServer::SendUnits()
{
  std::string content;

  ListIter<NetUnitEntry> map_iter = GetUnitMap();
  while (++map_iter)
  {
    NetUnitEntry* unit = map_iter.value();
    content += unit->GetDescription();
  }

  ListIter<NetUser> u_iter = users;
  while (++u_iter)
  {
    NetUser* u = u_iter.value();
    SendData(u, NET_LOBBY_UNIT_LIST, content);
  }
}

std::string NetLobbyServer::Serialize(Mission* m, NetUser* user)
{
  std::string s;

  if (!m || !user)
    return s;

  NetUnitEntry* unit = nullptr;

  ListIter<NetUnitEntry> u_iter = GetUnitMap();
  while (++u_iter && !unit)
  {
    NetUnitEntry* u = u_iter.value();
    if (u->GetUserName() == user->Name())
      unit = u;
  }

  if (unit)
    s = m->Serialize(unit->GetElemName(), unit->GetIndex());

  return s;
}

Mission* NetLobbyServer::GetSelectedMission()
{
  if (mission)
  {
    std::string content = Serialize(mission, GetLocalUser());
    Campaign* c = Campaign::SelectCampaign("Multiplayer Missions");

    if (c)
    {
      c->LoadNetMission(99999, content);
      return c->GetMission(99999);
    }
  }

  return mission;
}

void NetLobbyServer::GameOn()
{
  NetHost host;
  auto type = "Starshatter";
  auto password = "No";
  char address[32];

  strcpy_s(address, "0");

  if (m_serverConfig)
  {
    if (m_serverConfig->GetGameType() == NetServerConfig::NET_GAME_PRIVATE)
      return;

    if (m_serverConfig->GetGameType() == NetServerConfig::NET_GAME_LAN)
    {
      type = "Starshatter-LAN";
      sprintf_s(address, "%d.%d.%d.%d", host.Address().B1(), host.Address().B2(), host.Address().B3(),
                host.Address().B4());
    }
    else
    {
      type = "Starshatter";
      sprintf_s(address, "0.0.0.0");
    }

    if (!m_serverConfig->GetGamePass().empty())
      password = "Yes";
  }

  NetBrokerClient::GameOn(m_serverName, type, address, m_serverAddr.Port(), password);
}

void NetLobbyServer::GameOff() {}

void NetLobbyServer::DoPing(NetPeer* peer, std::string s) {}

void NetLobbyServer::DoServerInfo(NetPeer* peer, std::string s)
{
  if (peer && peer->NetID())
  {
    char buffer[1024];
    WORD gameport = 11101;

    if (m_serverConfig)
      gameport = m_serverConfig->GetGamePort();

    sprintf_s(buffer, "info \"%S\" version \"%s\" mode %d users %d host %s port %d", MachineInfo::GetShortDescription(),
              g_versionInfo, GetStatus(), NumUsers(), HasHost() ? "true" : "false", gameport);

    link->SendMessage(peer->NetID(), NET_LOBBY_SERVER_INFO, buffer, strlen(buffer), NetMsg::RELIABLE);
  }
}

void NetLobbyServer::DoLogin(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  std::string name;
  std::string pass;
  std::string host;
  std::string gamepass;
  std::string signature;
  std::string squadron;
  std::string version;
  int rank = 0;
  int flight_time = 0;
  int missions = 0;
  int kills = 0;
  int losses = 0;

  for (int i = 0; i < params.size(); i++)
  {
    NetLobbyParam* p = params[i];

    int num = 0;
    sscanf_s(p->value.c_str(), "%d", &num);

    if (p->name == "name")
      name = p->value;

    else if (p->name == "pass")
      pass = p->value;

    else if (p->name == "gamepass")
      gamepass = p->value;

    else if (p->name == "host")
      host = p->value;

    else if (p->name == "sig")
      signature = p->value;

    else if (p->name == "squad")
      squadron = p->value;

    else if (p->name == "version")
      version = p->value;

    else if (p->name == "rank")
      rank = num;

    else if (p->name == "time")
      flight_time = num;

    else if (p->name == "miss")
      missions = num;

    else if (p->name == "kill")
      kills = num;

    else if (p->name == "loss")
      losses = num;
  }

  params.destroy();

  // first check the game version:
  if (version != g_versionInfo)
  {
    DebugTrace("NetLobbyServer - user '%s' tried to login with invalid game version '%s'\n", name.data(),
               version.data());

    return;
  }

  // next check the game password:
  if (m_serverConfig && m_serverConfig->GetGamePass().length() > 0)
  {
    if (gamepass != m_serverConfig->GetGamePass())
    {
      DebugTrace("NetLobbyServer - user '%s' tried to login with invalid game password '%s'\n", name.data(),
                 gamepass.data());

      return;
    }
  }

  // now try to log the user in:
  NetUser* pre_existing = FindUserByName(name);

  // is user already logged in?
  if (pre_existing)
  {
    if (pre_existing->Pass() == pass && pre_existing->GetAddress().IPAddr() == peer->Address().IPAddr()) {}
  }

  // otherwise, create a new user:
  else
  {
    auto user = NEW NetUser(name);
    user->SetAddress(peer->Address());
    user->SetNetID(peer->NetID());
    user->SetPass(pass);
    user->SetSignature(signature);
    user->SetSquadron(squadron);
    user->SetRank(rank);
    user->SetFlightTime(flight_time);
    user->SetMissions(missions);
    user->SetKills(kills);
    user->SetLosses(losses);

    if (host == "true" && !HasHost())
      user->SetHost(true);

    AddUser(user);
    RequestAuth(user);
    SendMOTD(user);
  }
}

void NetLobbyServer::DoLogout(NetPeer* peer, std::string msg)
{
  NetUser* user = FindUserByNetID(peer->NetID());

  if (user)
  {
    if (user->IsHost())
      GameStop();

    DelUser(user);
  }
}

void NetLobbyServer::DoUserAuth(NetPeer* _peer, std::string _msg)
{
  NetUser* user = FindUserByNetID(_peer->NetID());

  if (user)
  {
    NetAuth::AuthUser(user, _msg);

    if (!user->IsAuthOK())
    {
      char buffer[256];

      sprintf_s(buffer, "id %d user \"SERVER\" msg \"**********\"", m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);

      sprintf_s(buffer, "id %d user \"SERVER\" msg \"*** Your game configuration does not match the server.\"",
                m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);

      sprintf_s(buffer, "id %d user \"SERVER\" msg \"*** Please verify that you have no mods deployed.\"",
                m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);

      sprintf_s(buffer, "id %d user \"SERVER\" msg \"*** You will not be permitted to join the game with an invalid\"",
                m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);

      sprintf_s(buffer, "id %d user \"SERVER\" msg \"*** configuration.  You may reconnect to this server after you\"",
                m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);

      sprintf_s(buffer, "id %d user \"SERVER\" msg \"*** have corrected your mod configuration.\"", m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);

      sprintf_s(buffer, "id %d user \"SERVER\" msg \"**********\"", m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);

      sprintf_s(buffer, "id %d user \" \" msg \" \"", m_motdIndex++);
      SendData(user, NET_LOBBY_CHAT, buffer);
    }
  }
}

void NetLobbyServer::DoChat(NetPeer* peer, std::string msg)
{
  List<NetLobbyParam> params;
  ParseMsg(msg, params);

  std::string chat_msg;

  for (int i = 0; i < params.size(); i++)
  {
    NetLobbyParam* p = params[i];

    int num = 0;
    sscanf_s(p->value.c_str(), "%d", &num);

    if (p->name == "msg")
      chat_msg = p->value;
  }

  params.destroy();

  NetUser* user = FindUserByNetID(peer->NetID());

  if (user)
  {
    // receive chat from client:
    if (chat_msg.length())
      AddChat(user, chat_msg);

    // request for chat log:
    else
    {
      ListIter<NetChatEntry> iter = chat_log;
      while (++iter)
      {
        NetChatEntry* entry = iter.value();

        char buffer[512];
        char msg_buf[256];
        char usr_buf[256];

        // safe quotes uses a static buffer,
        // so make sure to save copies of the
        // results when using more than one in
        // a function call...

        strcpy_s(msg_buf, SafeQuotes(entry->GetMessage()).c_str());
        strcpy_s(usr_buf, SafeQuotes(entry->GetUser()).c_str());

        sprintf_s(buffer, "id %d user \"%s\" msg \"%s\"", entry->GetID(), usr_buf, msg_buf);

        SendData(user, NET_LOBBY_CHAT, buffer);
      }
    }
  }
}

void NetLobbyServer::DoUserList(NetPeer* peer, std::string msg)
{
  NetUser* user = FindUserByNetID(peer->NetID());

  if (user)
  {
    std::string content;

    if (local_user)
      content += local_user->GetDescription();

    ListIter<NetUser> iter = users;
    while (++iter)
    {
      NetUser* u = iter.value();
      content += u->GetDescription();
    }

    SendData(user, NET_LOBBY_USER_LIST, content);
  }
}

void NetLobbyServer::DoBanUser(NetPeer* peer, std::string msg)
{
  NetUser* user = FindUserByNetID(peer->NetID());

  if (user && user->IsHost() && user->IsAuthOK())
  {
    List<NetLobbyParam> params;
    ParseMsg(msg, params);

    if (params.size() > 0)
    {
      NetLobbyParam* p = params[0];

      if (p->name == "user")
      {
        std::string user_name = p->value;

        NetUser* u = FindUserByName(user_name);
        if (u && !u->IsHost())
          BanUser(u);
      }
    }

    params.destroy();
  }
}

void NetLobbyServer::DoMissionList(NetPeer* peer, std::string msg)
{
  if (NetUser* user = FindUserByNetID(peer->NetID()))
  {
    std::string reply;
    char buffer[4096];

    ListIter<Campaign> c_iter = Campaign::GetAllCampaigns();
    while (++c_iter)
    {
      Campaign* c = c_iter.value();

      if (c->GetCampaignId() >= Campaign::MULTIPLAYER_MISSIONS)
      {
        sprintf_s(buffer, "c_id 0x%08x c_name \"%s\" ", c->GetCampaignId(), SafeQuotes(c->Name()).c_str());

        reply += buffer;
      }
    }

    c_iter.reset();

    while (++c_iter)
    {
      Campaign* c = c_iter.value();

      if (c->GetCampaignId() >= Campaign::MULTIPLAYER_MISSIONS)
      {
        ListIter<MissionInfo> m_iter = c->GetMissionList();
        while (++m_iter)
        {
          MissionInfo* m = m_iter.value();

          size_t mission_id = (c->GetCampaignId() << NET_CAMPAIGN_SHIFT) + m->m_id;

          sprintf_s(buffer, "m_id 0x%08x ", mission_id);
          reply += buffer;

          reply += "m_name \"";
          reply += SafeQuotes(m->m_name);

          // long version of safe quotes:
          int n = 0;
          const char* s = m->description.data();

          while (*s && n < 4090)
          {
            if (*s == '"')
            {
              buffer[n++] = '\'';
              s++;
            }
            else if (*s == '\n')
            {
              buffer[n++] = '\\';
              buffer[n++] = 'n';
              s++;
            }
            else if (*s == '\t')
            {
              buffer[n++] = '\\';
              buffer[n++] = 't';
              s++;
            }
            else
              buffer[n++] = *s++;
          }

          // don't forget the null terminator!
          buffer[n] = 0;

          reply += "\" m_desc \"";
          reply += buffer;

          reply += "\" ";
        }
      }
    }

    SendData(user, NET_LOBBY_MISSION_LIST, reply);

    sprintf_s(buffer, "m_id 0x%08x", selected_mission);
    SendData(user, NET_LOBBY_MISSION_SELECT, buffer);
  }
}

void NetLobbyServer::DoMissionSelect(NetPeer* peer, std::string msg)
{
  if (GetStatus() == NetServerInfo::PERSISTENT)
    return;

  NetUser* user = FindUserByNetID(peer->NetID());

  if (user && user->IsHost() && user->IsAuthOK())
  {
    List<NetLobbyParam> params;
    ParseMsg(msg, params);

    for (int i = 0; i < params.size(); i++)
    {
      NetLobbyParam* p = params[i];

      int num = 0;
      sscanf_s(p->value.c_str(), "0x%x", &num);

      if (p->name == "m_id")
        SelectMission(num);
    }

    params.destroy();
  }
}

void NetLobbyServer::DoMissionData(NetPeer* peer, std::string msg)
{
  NetUser* user = FindUserByNetID(peer->NetID());

  if (user && mission && user->IsAuthOK())
  {
    std::string reply = Serialize(mission, user);
    SendData(user, NET_LOBBY_MISSION_DATA, reply);

    FILE* f = fopen("multi_mission_send.def", "w");
    if (f)
    {
      fwrite(reply.data(), reply.length(), 1, f);
      fclose(f);
    }
  }
}

void NetLobbyServer::DoUnitList(NetPeer* peer, std::string msg)
{
  NetUser* user = FindUserByNetID(peer->NetID());

  if (user && unit_map.size() && user->IsAuthOK())
  {
    std::string reply;

    ListIter<NetUnitEntry> iter = GetUnitMap();
    while (++iter)
    {
      NetUnitEntry* unit = iter.value();
      reply += unit->GetDescription();
    }

    SendData(user, NET_LOBBY_UNIT_LIST, reply);
  }
}

void NetLobbyServer::DoMapUnit(NetPeer* peer, std::string msg)
{
  NetUser* user = FindUserByNetID(peer->NetID());

  if (user && unit_map.size() && user->IsAuthOK())
  {
    List<NetLobbyParam> params;
    ParseMsg(msg, params);

    int id = 0;
    bool lock = false;
    std::string user_name;

    for (int i = 0; i < params.size(); i++)
    {
      NetLobbyParam* p = params[i];

      if (p->name == "id")
        sscanf_s(p->value.c_str(), "%d", &id);

      else if (p->name == "user")
        user_name = p->value;

      else if (p->name == "lock")
        lock = (p->value == "true") ? true : false;
    }

    params.destroy();

    MapUnit(id, user_name, lock);
  }
}

void NetLobbyServer::DoGameStart(NetPeer* peer, std::string msg) { GameStart(); }

void NetLobbyServer::DoGameStop(NetPeer* peer, std::string msg)
{
  NetUser* user = FindUserByNetID(peer->NetID());

  if (user && user->IsHost() && user->IsAuthOK())
    GameStop();
}
