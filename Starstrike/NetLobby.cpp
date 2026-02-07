#include "pch.h"
#include "NetLobby.h"
#include "Campaign.h"
#include "Mission.h"
#include "NetChat.h"
#include "NetMsg.h"
#include "NetUser.h"
#include "Reader.h"
#include "ShipDesign.h"
#include "Token.h"

constexpr int MAX_NET_FPS = 50;
constexpr int MIN_NET_FRAME = 1000 / MAX_NET_FPS;

constexpr DWORD SHIP_ID_START = 0x0010;
constexpr DWORD SHOT_ID_START = 0x0400;

static NetLobby* instance = nullptr;

NetLobby::NetLobby(bool temporary)
  : link(nullptr),
    local_user(nullptr),
    active(true),
    last_send_time(0),
    selected_mission(0),
    mission(nullptr),
    status(0)
{
  if (!temporary)
    instance = this;
}

NetLobby::~NetLobby()
{
  if (instance == this)
    instance = nullptr;

  unit_map.destroy();
  users.destroy();
  campaigns.destroy();
  chat_log.destroy();

  if (local_user)
  {
    delete local_user;
    local_user = nullptr;
  }

  if (link)
  {
    delete link;
    link = nullptr;
  }

  mission = nullptr;
}

NetLobby* NetLobby::GetInstance() { return instance; }

void NetLobby::ExecFrame()
{
  Send();
  Recv();
}

void NetLobby::Recv()
{
  NetMsg* msg = link->GetMessage();

  while (msg)
  {
    if (active)
    {
      NetPeer* peer = link->FindPeer(msg->NetID());

      static char buffer[256];
      char* text = nullptr;

      if (msg->Length() > 2)
      {
        if (msg->Length() < 250)
        {
          ZeroMemory(buffer, sizeof(buffer));
          CopyMemory(buffer, msg->Data()+2, msg->Length()-2);
          text = buffer;
        }
        else if (msg->Length() < 1024 * 1024)
        {
          text = NEW char[msg->Length()];
          ZeroMemory(text, msg->Length());
          CopyMemory(text, msg->Data()+2, msg->Length()-2);
        }
      }

      switch (msg->Type())
      {
        case NET_LOBBY_PING:
          DoPing(peer, text);
          break;
        case NET_LOBBY_SERVER_INFO:
          DoServerInfo(peer, text);
          break;
        case NET_LOBBY_SERVER_MODS:
          DoServerMods(peer, text);
          break;
        case NET_LOBBY_LOGIN:
          DoLogin(peer, text);
          break;
        case NET_LOBBY_LOGOUT:
          DoLogout(peer, text);
          break;

        case NET_LOBBY_AUTH_USER:
          DoAuthUser(peer, text);
          break;
        case NET_LOBBY_USER_AUTH:
          DoUserAuth(peer, text);
          break;

        case NET_LOBBY_CHAT:
          DoChat(peer, text);
          break;
        case NET_LOBBY_USER_LIST:
          DoUserList(peer, text);
          break;
        case NET_LOBBY_BAN_USER:
          DoBanUser(peer, text);
          break;
        case NET_LOBBY_MISSION_LIST:
          DoMissionList(peer, text);
          break;
        case NET_LOBBY_MISSION_SELECT:
          DoMissionSelect(peer, text);
          break;
        case NET_LOBBY_MISSION_DATA:
          DoMissionData(peer, text);
          break;
        case NET_LOBBY_UNIT_LIST:
          DoUnitList(peer, text);
          break;
        case NET_LOBBY_MAP_UNIT:
          DoMapUnit(peer, text);
          break;
        case NET_LOBBY_GAME_START:
          DoGameStart(peer, text);
          break;
        case NET_LOBBY_GAME_STOP:
          DoGameStop(peer, text);
          break;
        case NET_LOBBY_EXIT:
          DoExit(peer, text);
          break;
      }

      if (text && text != buffer)
        delete [] text;
    }

    delete msg;
    msg = link->GetMessage();
  }
}

void NetLobby::Send() {}

DWORD NetLobby::GetLag()
{
  if (link)
    return link->GetLag();

  return 0;
}

void NetLobby::SetLocalUser(NetUser* user)
{
  if (user != local_user)
  {
    if (local_user)
    {
      delete local_user;
      local_user = nullptr;
    }

    if (user)
    {
      local_user = user;
      local_user->SetHost(true);
    }
  }
}

void NetLobby::BanUser(NetUser* user) {}

void NetLobby::AddUser(NetUser* user)
{
  if (user)
  {
    if (user != local_user && !users.contains(user))
    {
      users.append(user);
      DebugTrace("NetLobby User Logged In  - name: '%s' id: %d host: %d\n", user->Name().data(), user->GetNetID(),
                 user->IsHost());
    }
  }
}

void NetLobby::DelUser(NetUser* user)
{
  if (user)
  {
    if (user == local_user)
      local_user = nullptr;

    else
      users.remove(user);

    UnmapUnit(user->Name());

    DebugTrace("NetLobby User Logged Out - name: '%s' id: %d host: %d\n", user->Name().data(), user->GetNetID(),
               user->IsHost());

    user->SetHost(false);
    delete user;
  }
}

int NetLobby::NumUsers()
{
  int num = 0;

  if (local_user)
    num = 1;

  num += users.size();

  return num;
}

NetUser* NetLobby::GetHost()
{
  for (int i = 0; i < users.size(); i++)
  {
    NetUser* u = users[i];

    if (u->IsHost())
      return u;
  }

  return nullptr;
}

bool NetLobby::HasHost()
{
  bool host = false;

  if (local_user && local_user->IsHost())
    host = true;

  for (int i = 0; i < users.size() && !host; i++)
  {
    if (users[i]->IsHost())
      host = true;
  }

  if (status > NetServerInfo::LOBBY)
    host = true;

  return host;
}

bool NetLobby::SetUserHost(NetUser* user, bool host)
{
  bool ok = false;

  if (user && users.contains(user))
  {
    if (host && !HasHost())
    {
      user->SetHost(true);
      ok = true;
    }
    else if (!host)
    {
      user->SetHost(false);
      ok = true;
    }
  }

  return ok;
}

NetUser* NetLobby::GetLocalUser() { return local_user; }

List<NetUser>& NetLobby::GetUsers() { return users; }

NetUser* NetLobby::FindUserByAddr(const NetAddr& addr)
{
  for (int i = 0; i < users.size(); i++)
  {
    NetUser* u = users[i];
    if (u->GetAddress().IPAddr() == addr.IPAddr())
      return u;
  }

  return nullptr;
}

NetUser* NetLobby::FindUserByName(std::string_view name)
{
  if (local_user && local_user->Name() == name)
    return local_user;

  for (int i = 0; i < users.size(); i++)
  {
    NetUser* u = users[i];
    if (u->Name() == name)
      return u;
  }

  return nullptr;
}

NetUser* NetLobby::FindUserByNetID(DWORD id)
{
  for (int i = 0; i < users.size(); i++)
  {
    NetUser* u = users[i];
    if (u->GetNetID() == id)
      return u;
  }

  return nullptr;
}

void NetLobby::ParseMsg(std::string msg, List<NetLobbyParam>& params)
{
  params.destroy();

  BlockReader reader(msg);
  Scanner lexer(&reader);

  Token name = lexer.Get();

  while (name.type() == Token::AlphaIdent)
  {
    Token value = lexer.Get();
    if (value.type() != Token::EOT)
    {
      std::string val = value.symbol();

      if (val[0] == '"' && val[val.length() - 1] == '"')
        val = val.substr(1, val.length() - 2);

      auto param = NEW NetLobbyParam(name.symbol(), val);
      params.append(param);

      name = lexer.Get();
    }

    else
      name = Token(Token::EOT);
  }
}

bool NetLobby::Ping()
{
  Sleep(500);
  return false;
}

void NetLobby::AddChat(NetUser* user, std::string_view msg, bool route)
{
  if (user && !msg.empty())
    chat_log.append(NEW NetChatEntry(user, msg));
}

List<NetChatEntry>& NetLobby::GetChat() { return chat_log; }

void NetLobby::ClearChat() { chat_log.destroy(); }

List<NetCampaignInfo>& NetLobby::GetCampaigns() { return campaigns; }

void NetLobby::AddUnitMap(MissionElement* elem, int index)
{
  if (elem)
    unit_map.append(NEW NetUnitEntry(elem, index));
}

List<NetUnitEntry>& NetLobby::GetUnitMap() { return unit_map; }

void NetLobby::ClearUnitMap() { unit_map.destroy(); }

void NetLobby::MapUnit(int n, std::string_view user, bool lock)
{
  if (n >= 0 && n < unit_map.size())
  {
    NetUnitEntry* unit = unit_map[n];

    if (!lock && unit->GetLocked())
      return;

    if (!user.empty())
    {
      for (int i = 0; i < unit_map.size(); i++)
      {
        NetUnitEntry* u = unit_map[i];
        if (u->GetUserName() == user)
        {
          if (!lock && u->GetLocked())
            return;
          u->SetUserName("");
        }
      }
    }

    unit->SetUserName(user);
    unit->SetLock(lock);
  }
}

void NetLobby::UnmapUnit(std::string_view user)
{
  if (!user.empty())
  {
    for (int i = 0; i < unit_map.size(); i++)
    {
      NetUnitEntry* u = unit_map[i];
      if (u->GetUserName() == user)
      {
        u->SetUserName("");
        u->SetLock(false);
        return;
      }
    }
  }
}

bool NetLobby::IsMapped(std::string_view user)
{
  if (!user.empty())
  {
    for (int i = 0; i < unit_map.size(); i++)
    {
      NetUnitEntry* u = unit_map[i];
      if (u->GetUserName() == user)
        return true;
    }
  }

  return false;
}

void NetLobby::SelectMission(DWORD id)
{
  if (selected_mission != id)
  {
    selected_mission = id;
    ClearUnitMap();

    int mission_id = selected_mission;
    int campaign_id = -1;
    Campaign* campaign = nullptr;
    mission = nullptr;

    campaign_id = mission_id >> NET_CAMPAIGN_SHIFT;
    mission_id = mission_id & NET_MISSION_MASK;

    ListIter<Campaign> c_iter = Campaign::GetAllCampaigns();
    while (++c_iter)
    {
      campaign = c_iter.value();

      if (campaign->GetCampaignId() == campaign_id)
      {
        mission = campaign->GetMission(mission_id);
        break;
      }
    }

    if (campaign && mission)
    {
      Campaign::SelectCampaign(campaign->Name());
      campaign->SetMissionId(mission_id);

      ListIter<MissionElement> iter = mission->GetElements();
      while (++iter)
      {
        MissionElement* elem = iter.value();

        if (elem->IsPlayable())
        {
          if (elem->Count() == 1)
            AddUnitMap(elem);
          else
          {
            for (int i = 0; i < elem->Count(); i++)
              AddUnitMap(elem, i + 1);
          }
        }
      }
    }

    else
    {
      selected_mission = 0;
      mission = nullptr;
    }
  }
}

bool NetLobby::IsNetLobbyClient()
{
  if (instance)
    return instance->IsClient();
  return false;
}

bool NetLobby::IsNetLobbyServer()
{
  if (instance)
    return instance->IsServer();
  return false;
}

NetUnitEntry::NetUnitEntry(MissionElement* e, int n)
  : index(n),
    lives(1),
    hull(100),
    role(0),
    lock(false)
{
  if (e)
  {
    elem = e->Name();
    iff = e->GetIFF();

    if (e->GetDesign())
      design = e->GetDesign()->m_name;
  }
}

NetUnitEntry::NetUnitEntry(std::string_view e, std::string_view d, int i, int n)
  : elem(e),
    design(d),
    iff(i),
    index(n),
    lives(1),
    hull(100),
    role(0),
    lock(false) {}

NetUnitEntry::~NetUnitEntry() {}

std::string NetUnitEntry::GetDescription() const
{
  if (!elem.empty())
  {
    static char buffer[1024];

    sprintf_s(buffer, "name \"%s\" index %d design \"%s\" iff %d user \"%s\" lives %d hull %d role %d lock %d ",
              elem.data(), index, design.data(), iff, user.data(), lives, hull, role, lock);

    return buffer;
  }

  return "name \"Not Found\" ";
}

NetServerInfo::NetServerInfo()
  : port(0),
    gameport(0),
    save(false),
    nplayers(0),
    hosted(0),
    status(0),
    ping_time(0) {}
