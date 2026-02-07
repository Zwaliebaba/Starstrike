#include "pch.h"
#include "NetGame.h"
#include "NetClientConfig.h"
#include "NetData.h"
#include "NetGameClient.h"
#include "NetGameServer.h"
#include "NetLayer.h"
#include "NetMsg.h"
#include "NetPlayer.h"
#include "NetServerConfig.h"
#include "Player.h"
#include "Ship.h"
#include "Sim.h"

constexpr int MAX_NET_FPS = 20;
constexpr int MIN_NET_FRAME = 1000 / MAX_NET_FPS;

constexpr DWORD SHIP_ID_START = 0x0010;
constexpr DWORD SHOT_ID_START = 0x0400;

static NetGame* netgame = nullptr;

static DWORD ship_id_key = SHIP_ID_START;
static DWORD shot_id_key = SHOT_ID_START;

static long start_time = 0;

NetGame::NetGame()
  : link(nullptr),
    objid(0),
    netid(0),
    local_player(nullptr),
    active(true),
    last_send_time(0)
{
  netgame = this;
  sim = Sim::GetSim();

  ship_id_key = SHIP_ID_START;
  shot_id_key = SHOT_ID_START;

  if (sim)
    local_player = sim->GetPlayerShip();

  Player* player = Player::GetCurrentPlayer();
  if (player)
  {
    player_name = player->Name();
    player_pass = player->Password();
  }

  start_time = NetLayer::GetUTC();
}

NetGame::~NetGame()
{
  netgame = nullptr;
  local_player = nullptr;
  players.destroy();

  if (link)
  {
    double delta = fabs(static_cast<double>(NetLayer::GetUTC() - start_time));
    double bandwidth = 10.0 * (link->GetBytesSent() + link->GetBytesRecv()) / delta;
    double recvrate = link->GetPacketsRecv() / delta;

    DebugTrace("NetGame Stats\n-------------\n");
    DebugTrace("  packets sent %d\n", link->GetPacketsSent());
    DebugTrace("  packets recv %d\n", link->GetPacketsRecv());
    DebugTrace("  bytes sent   %d\n", link->GetBytesSent());
    DebugTrace("  bytes recv   %d\n", link->GetBytesRecv());
    DebugTrace("  retries      %d\n", link->GetRetries());
    DebugTrace("  drops        %d\n", link->GetDrops());
    DebugTrace("  avg lag      %d msec\n", link->GetLag());
    DebugTrace("  time         %d sec\n", static_cast<int>(delta));
    DebugTrace("  bandwidth    %d bps\n", static_cast<int>(bandwidth));
    DebugTrace("  packet rate  %d pps in\n\n", static_cast<int>(recvrate));

    delete link;
  }
}

NetGame* NetGame::Create()
{
  if (!netgame)
  {
    if (NetServerConfig::GetInstance())
      netgame = NEW NetGameServer;

    else if (NetClientConfig::GetInstance() && NetClientConfig::GetInstance()->GetSelectedServer())
      netgame = NEW NetGameClient;
  }

  return netgame;
}

NetGame* NetGame::GetInstance() { return netgame; }

DWORD NetGame::GetObjID() const
{
  if (local_player)
    return local_player->GetObjID();

  return 0;
}

DWORD NetGame::GetNextObjID(int type)
{
  if (type == SHIP)
  {
    if (ship_id_key >= SHOT_ID_START)
      ship_id_key = SHIP_ID_START;

    return ship_id_key++;
  }
  if (type == SHOT)
  {
    if (shot_id_key >= 0xFFFE)
      shot_id_key = SHOT_ID_START;

    return shot_id_key++;
  }

  return 0;
}

void NetGame::ExecFrame()
{
  Send();
  Recv();
}

void NetGame::Recv()
{
  NetMsg* msg = link->GetMessage();

  while (msg)
  {
    if (active)
    {
      switch (msg->Type())
      {
        case NET_JOIN_REQUEST:
          DoJoinRequest(msg);
          break;
        case NET_JOIN_ANNOUNCE:
          DoJoinAnnounce(msg);
          break;
        case NET_QUIT_REQUEST:
          DoQuitRequest(msg);
          break;
        case NET_QUIT_ANNOUNCE:
          DoQuitAnnounce(msg);
          break;
        case NET_GAME_OVER:
          DoGameOver(msg);
          break;
        case NET_DISCONNECT:
          DoDisconnect(msg);
          break;

        case NET_OBJ_LOC:
          DoObjLoc(msg);
          break;
        case NET_OBJ_DAMAGE:
          DoObjDamage(msg);
          break;
        case NET_OBJ_KILL:
          DoObjKill(msg);
          break;
        case NET_OBJ_SPAWN:
          DoObjSpawn(msg);
          break;
        case NET_OBJ_HYPER:
          DoObjHyper(msg);
          break;
        case NET_OBJ_TARGET:
          DoObjTarget(msg);
          break;
        case NET_OBJ_EMCON:
          DoObjEmcon(msg);
          break;
        case NET_SYS_DAMAGE:
          DoSysDamage(msg);
          break;
        case NET_SYS_STATUS:
          DoSysStatus(msg);
          break;

        case NET_ELEM_CREATE:
          DoElemCreate(msg);
          break;
        case NET_ELEM_REQUEST:
          DoElemRequest(msg);
          break;
        case NET_SHIP_LAUNCH:
          DoShipLaunch(msg);
          break;
        case NET_NAV_DATA:
          DoNavData(msg);
          break;
        case NET_NAV_DELETE:
          DoNavDelete(msg);
          break;

        case NET_WEP_TRIGGER:
          DoWepTrigger(msg);
          break;
        case NET_WEP_RELEASE:
          DoWepRelease(msg);
          break;
        case NET_WEP_DESTROY:
          DoWepDestroy(msg);
          break;

        case NET_COMM_MESSAGE:
          DoCommMsg(msg);
          break;
        case NET_CHAT_MESSAGE:
          DoChatMsg(msg);
          break;
        case NET_SELF_DESTRUCT:
          DoSelfDestruct(msg);
          break;
      }
    }

    delete msg;
    msg = link->GetMessage();
  }
}

void NetGame::Send() {}

int NetGame::NumPlayers()
{
  if (netgame)
  {
    int num_players = netgame->players.size();

    if (netgame->local_player)
      num_players++;

    return num_players;
  }

  return 0;
}

NetPlayer* NetGame::FindPlayerByName(std::string_view name)
{
  for (int i = 0; i < players.size(); i++)
  {
    NetPlayer* p = players[i];

    if (p->Name() == name)
      return p;
  }

  return nullptr;
}

NetPlayer* NetGame::FindPlayerByNetID(DWORD netid)
{
  for (int i = 0; i < players.size(); i++)
  {
    NetPlayer* p = players[i];

    if (p->GetNetID() == netid)
      return p;
  }

  return nullptr;
}

NetPlayer* NetGame::FindPlayerByObjID(DWORD objid)
{
  for (int i = 0; i < players.size(); i++)
  {
    NetPlayer* p = players[i];

    if (p->GetObjID() == objid)
      return p;
  }

  return nullptr;
}

Ship* NetGame::FindShipByObjID(DWORD objid)
{
  if (sim)
    return sim->FindShipByObjID(objid);

  return nullptr;
}

Shot* NetGame::FindShotByObjID(DWORD objid)
{
  if (sim)
    return sim->FindShotByObjID(objid);

  return nullptr;
}

NetPeer* NetGame::GetPeer(NetPlayer* player)
{
  if (player && link)
    return link->FindPeer(player->GetNetID());

  return nullptr;
}

void NetGame::Respawn(DWORD objid, Ship* spawn) {}

bool NetGame::IsNetGame() { return netgame != nullptr; }

bool NetGame::IsNetGameClient()
{
  if (netgame)
    return netgame->IsClient();
  return false;
}

bool NetGame::IsNetGameServer()
{
  if (netgame)
    return netgame->IsServer();
  return false;
}
