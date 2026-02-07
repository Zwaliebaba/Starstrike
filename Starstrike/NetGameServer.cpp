#include "pch.h"
#include "NetGameServer.h"
#include "Element.h"
#include "FlightDeck.h"
#include "Game.h"
#include "HUDView.h"
#include "Hangar.h"
#include "Instruction.h"
#include "NetData.h"
#include "NetHost.h"
#include "NetLayer.h"
#include "NetLobbyServer.h"
#include "NetMsg.h"
#include "NetPeer.h"
#include "NetPlayer.h"
#include "NetServerConfig.h"
#include "NetUser.h"
#include "NetUtil.h"
#include "RadioMessage.h"
#include "Shield.h"
#include "Ship.h"
#include "Sim.h"
#include "SimEvent.h"
#include "StarServer.h"

constexpr int MAX_NET_FPS = 20;
constexpr int MIN_NET_FRAME = 1000 / MAX_NET_FPS;

NetGameServer::NetGameServer()
{
  DebugTrace("Constructing NetGameServer\n");

  WORD server_port = 11101;

  if (NetServerConfig::GetInstance())
    server_port = NetServerConfig::GetInstance()->GetGamePort();

  NetAddr server(NetHost().Address().IPAddr(), server_port);

  link = NEW NetLink(server);

  ListIter<SimRegion> rgn_iter = sim->GetRegions();
  while (++rgn_iter)
  {
    SimRegion* rgn = rgn_iter.value();

    ListIter<Ship> iter = rgn->Ships();
    while (++iter)
    {
      Ship* s = iter.value();
      s->SetObjID(GetNextObjID());
      Observe(s);
      ships.append(s);
    }
  }

  if (local_player)
  {
    Observe(local_player);
    objid = local_player->GetObjID();
  }
}

NetGameServer::~NetGameServer()
{
  ListIter<NetPlayer> player = players;
  while (++player)
  {
    NetPlayer* p = player.value();

    if (p->GetShip())
      p->GetShip()->SetRespawnCount(0);

    link->SendMessage(p->GetNetID(), NET_GAME_OVER, nullptr, 0, NetMsg::RELIABLE);
  }

  Sleep(500);

  ListIter<Ship> iter = ships;
  while (++iter)
  {
    Ship* s = iter.value();
    s->SetRespawnCount(0);
  }

  zombies.destroy();
  ships.clear();
}

void NetGameServer::ExecFrame()
{
  NetGame::ExecFrame();
  CheckSessions();

  ListIter<SimRegion> rgn_iter = sim->GetRegions();
  while (++rgn_iter)
  {
    SimRegion* rgn = rgn_iter.value();

    ListIter<Ship> iter = rgn->Ships();
    while (++iter)
    {
      Ship* s = iter.value();

      if (s->GetObjID() == 0)
      {
        s->SetObjID(GetNextObjID());
        Observe(s);
        ships.append(s);

        NetJoinAnnounce join_ann;
        join_ann.SetShip(s);
        join_ann.SetName("Server A.I. Ship");
        SendData(&join_ann);
      }
    }
  }

  static DWORD time_mark = 0;

  if (!time_mark)
    time_mark = Game::RealTime();
  else if (Game::RealTime() - time_mark > 60000)
  {
    time_mark = Game::RealTime();

    if (link && players.size() > 0)
    {
      DebugTrace("Server Stats\n-------------\n");
      DebugTrace("  packets sent %d\n", link->GetPacketsSent());
      DebugTrace("  packets recv %d\n", link->GetPacketsRecv());
      DebugTrace("  bytes sent   %d\n", link->GetBytesSent());
      DebugTrace("  bytes recv   %d\n", link->GetBytesRecv());
      DebugTrace("  retries      %d\n", link->GetRetries());
      DebugTrace("  drops        %d\n", link->GetDrops());
      DebugTrace("  avg lag      %d msec\n", link->GetLag());
    }
  }
}

void NetGameServer::CheckSessions()
{
  if (!link)
    return;

  ListIter<NetPlayer> iter = players;
  while (++iter)
  {
    NetPlayer* player = iter.value();
    NetPeer* peer = link->FindPeer(player->GetNetID());

    if (peer && (NetLayer::GetUTC() - peer->LastReceiveTime()) > NET_DISCONNECT_TIME)
    {
      // announce drop:
      NetPlayer* zombie = iter.removeItem();
      HUDView::Message(Game::GetText("NetGameServer.remote-discon", zombie->Name()).c_str());

      // tell everyone else:
      NetQuitAnnounce quit_ann;
      quit_ann.SetObjID(zombie->GetObjID());
      quit_ann.SetDisconnected(true);
      SendData(&quit_ann);

      // return remote ship to ship pool:
      Ship* s = zombie->GetShip();
      if (s)
      {
        Observe(s);
        ships.append(s);
        s->SetNetworkControl(nullptr);
        zombie->SetShip(nullptr);

        NetJoinAnnounce join_ann;
        join_ann.SetShip(s);
        join_ann.SetName("Server A.I. Ship");
        SendData(&join_ann);
      }

      zombies.append(zombie);
    }
  }
}

NetPlayer* NetGameServer::FindZombieByObjID(DWORD objid)
{
  for (int i = 0; i < zombies.size(); i++)
  {
    NetPlayer* p = zombies[i];

    if (p->GetObjID() == objid)
      return p;
  }

  return nullptr;
}

void NetGameServer::DoJoinRequest(NetMsg* msg)
{
  if (!msg)
    return;

  bool unpause = players.isEmpty();

  NetJoinRequest join_req;
  if (join_req.Unpack(msg->Data()))
  {
    HUDView::Message(Game::GetText("NetGameServer::join-request", join_req.GetName(), join_req.GetElement(),
                                   join_req.GetIndex()).c_str());

    DWORD nid = msg->NetID();
    std::string name = join_req.GetName();
    std::string serno = join_req.GetSerialNumber();
    std::string elem_name = join_req.GetElement();
    int index = join_req.GetIndex();
    Ship* ship = nullptr;
    Sim* sim = Sim::GetSim();

    if (sim)
    {
      Element* element = sim->FindElement(elem_name);

      if (element)
        ship = element->GetShip(index);
    }

    if (!ship)
    {
      DebugTrace("  JOIN DENIED: could not locate ship for remote player\n");
      return;
    }

    if (!ship->GetObjID())
    {
      DebugTrace("  JOIN DENIED: remote player requested ship with objid = 0\n");
      return;
    }

    NetLobbyServer* lobby = NetLobbyServer::GetInstance();

    if (lobby)
    {
      NetUser* user = lobby->FindUserByName(name);

      if (!user)
        user = lobby->FindUserByNetID(nid);

      if (!user)
      {
        DebugTrace("  JOIN DENIED: remote player '%s' not found in lobby\n", name.data());
        return;
      }
      if (!user->IsAuthOK())
      {
        DebugTrace("  JOIN DENIED: remote player '%s' not authenticated\n", name.data());
        return;
      }
    }

    NetPlayer* remote_player = FindPlayerByNetID(nid);
    if (remote_player && remote_player->GetShip() != ship)
    {
      DebugTrace("  disconnecting remote player from ship '%s'\n", ship->Name());
      players.remove(remote_player);
      delete remote_player;
      remote_player = nullptr;
    }

    if (!remote_player)
    {
      Ignore(ship);
      ships.remove(ship);

      remote_player = NEW NetPlayer(nid);
      remote_player->SetName(name);
      remote_player->SetSerialNumber(serno);
      remote_player->SetObjID(ship->GetObjID());
      remote_player->SetShip(ship);

      HUDView::Message(Game::GetText("NetGameServer::join-announce").data());
      DebugTrace("remote player name = %s\n", name.data());
      DebugTrace("              obj  = %d\n", ship->GetObjID());
      DebugTrace("              ship = %s\n", ship->Name());

      remote_player->SetObjID(ship->GetObjID());

      // tell the new player about the server:
      if (local_player)
      {
        NetJoinAnnounce join_ann;
        join_ann.SetShip(local_player);
        join_ann.SetName(player_name);
        link->SendMessage(remote_player->GetNetID(), join_ann.Pack(), NetJoinAnnounce::SIZE, NetMsg::RELIABLE);
      }

      // tell the new player about the existing remote players:
      ListIter<NetPlayer> iter = players;
      while (++iter)
      {
        Ship* s = iter->GetShip();

        if (s)
        {
          NetJoinAnnounce join_ann;
          join_ann.SetShip(s);
          join_ann.SetName(iter->Name());
          join_ann.SetObjID(iter->GetObjID());
          link->SendMessage(remote_player->GetNetID(), join_ann.Pack(), NetJoinAnnounce::SIZE, NetMsg::RELIABLE);
        }
      }

      // tell the new player about the A.I. controlled ships:
      ListIter<Ship> ai_iter = ships;
      while (++ai_iter)
      {
        Ship* s = ai_iter.value();
        if (s != local_player)
        {
          NetJoinAnnounce join_ann;
          join_ann.SetShip(s);
          join_ann.SetName("Server A.I. Ship");
          link->SendMessage(remote_player->GetNetID(), join_ann.Pack(), NetJoinAnnounce::SIZE, NetMsg::RELIABLE);
        }
      }

      // make the new player an "existing" remote player:
      players.append(remote_player);

      // tell existing players about the new player:
      // NOTE, this also provides the net id to the new player!
      iter.reset();
      while (++iter)
      {
        Ship* s = remote_player->GetShip();

        NetJoinAnnounce join_ann;
        join_ann.SetShip(s);
        join_ann.SetName(remote_player->Name());
        join_ann.SetObjID(remote_player->GetObjID());
        link->SendMessage(iter->GetNetID(), join_ann.Pack(), NetJoinAnnounce::SIZE, NetMsg::RELIABLE);
      }

      if (unpause)
      {
        StarServer* s = StarServer::GetInstance();
        if (s)
          s->Pause(false);
      }
    }
  }
}

void NetGameServer::DoJoinAnnounce(NetMsg* msg)
{
  if (!msg)
    return;

  NetJoinAnnounce join_ann;
  if (join_ann.Unpack(msg->Data()))
    DebugTrace("Server received Join Announce from '%s'\n", join_ann.GetName());
}

void NetGameServer::DoQuitRequest(NetMsg* _msg)
{
  if (!_msg)
    return;

  NetPlayer* player = FindPlayerByNetID(_msg->NetID());

  if (player)
  {
    NetPlayer* zombie = players.remove(player);
    HUDView::Message(Game::GetText("NetGameServer.remote-quit", zombie->Name()).c_str());

    // tell everyone else:
    NetQuitAnnounce quit_ann;
    quit_ann.SetObjID(zombie->GetObjID());
    SendData(&quit_ann);

    // return remote ship to ship pool:
    Ship* s = zombie->GetShip();
    if (s)
    {
      Observe(s);
      ships.append(s);
      s->SetNetworkControl(nullptr);
      zombie->SetShip(nullptr);

      NetJoinAnnounce join_ann;
      join_ann.SetShip(s);
      join_ann.SetName("Server A.I. Ship");
      SendData(&join_ann);
    }

    zombies.append(zombie);
  }
  else
    DebugTrace("Quit Request from unknown player NetID: %08X\n", _msg->NetID());
}

void NetGameServer::DoQuitAnnounce(NetMsg* msg)
{
  if (!msg)
    return;
  DebugTrace("Server received Quit Announce from NetID: %08x\n", msg->NetID());
}

void NetGameServer::DoGameOver(NetMsg* msg)
{
  if (!msg)
    return;
  DebugTrace("Server received Game Over from NetID: %08x\n", msg->NetID());
}

void NetGameServer::DoDisconnect(NetMsg* msg)
{
  if (!msg)
    return;
  DebugTrace("Server received Disconnect from NetID: %08x\n", msg->NetID());
}

void NetGameServer::DoObjLoc(NetMsg* msg)
{
  if (!msg)
    return;

  NetObjLoc obj_loc;
  obj_loc.Unpack(msg->Data());

  NetPlayer* player = FindPlayerByObjID(obj_loc.GetObjID());
  if (player && player->GetShip())
    player->DoObjLoc(&obj_loc);

  else
  {
    player = FindZombieByObjID(obj_loc.GetObjID());

    if (player)
      SendDisconnect(player);
  }
}

void NetGameServer::DoObjDamage(NetMsg* msg)
{
  if (!msg)
    return;
  DebugTrace("Server received OBJ DAMAGE from NetID: %08x (ignored)\n", msg->NetID());
}

void NetGameServer::DoObjKill(NetMsg* msg)
{
  if (!msg)
    return;

  NetObjKill obj_kill;
  obj_kill.Unpack(msg->Data());

  if (obj_kill.GetKillType() != NetObjKill::KILL_DOCK)
  {
    DebugTrace("Server received OBJ KILL from NetID: %08x (ignored)\n", msg->NetID());
    return;
  }

  Ship* ship = FindShipByObjID(obj_kill.GetObjID());
  if (ship)
  {
    Ship* killer = FindShipByObjID(obj_kill.GetKillerID());
    std::string killer_name = Game::GetText("NetGameServer.unknown");

    if (killer)
      killer_name = killer->Name();

    ShipStats* killee = ShipStats::Find(ship->Name());
    if (killee)
      killee->AddEvent(SimEvent::DOCK, killer_name);

    FlightDeck* deck = killer->GetFlightDeck(obj_kill.GetFlightDeck());
    sim->NetDockShip(ship, killer, deck);
  }
}

void NetGameServer::DoObjSpawn(NetMsg* msg)
{
  if (!msg)
    return;
  DebugTrace("Server received OBJ SPAWN from NetID: %08x (ignored)\n", msg->NetID());
}

void NetGameServer::DoObjHyper(NetMsg* msg)
{
  if (!msg)
    return;
  DebugTrace("Server received OBJ HYPER from NetID: %d\n", msg->NetID());

  NetObjHyper obj_hyper;
  obj_hyper.Unpack(msg->Data());

  NetPlayer* player = FindPlayerByObjID(obj_hyper.GetObjID());
  if (player && player->GetShip())
  {
    if (player->DoObjHyper(&obj_hyper))
      SendData(&obj_hyper);
  }
  else
  {
    player = FindZombieByObjID(obj_hyper.GetObjID());

    if (player)
      SendDisconnect(player);
  }
}

void NetGameServer::DoObjTarget(NetMsg* msg)
{
  if (!msg)
    return;

  NetObjTarget obj_target;
  obj_target.Unpack(msg->Data());

  NetPlayer* player = FindPlayerByObjID(obj_target.GetObjID());
  if (player)
    player->DoObjTarget(&obj_target);
  else
  {
    player = FindZombieByObjID(obj_target.GetObjID());

    if (player)
      SendDisconnect(player);
  }
}

void NetGameServer::DoObjEmcon(NetMsg* msg)
{
  if (!msg)
    return;

  NetObjEmcon obj_emcon;
  obj_emcon.Unpack(msg->Data());

  NetPlayer* player = FindPlayerByObjID(obj_emcon.GetObjID());
  if (player)
    player->DoObjEmcon(&obj_emcon);
  else
  {
    player = FindZombieByObjID(obj_emcon.GetObjID());

    if (player)
      SendDisconnect(player);
  }
}

void NetGameServer::DoSysDamage(NetMsg* msg)
{
  if (!msg)
    return;

  NetSysDamage sys_damage;
  sys_damage.Unpack(msg->Data());

  NetPlayer* player = FindZombieByObjID(sys_damage.GetObjID());

  if (player)
    SendDisconnect(player);
}

void NetGameServer::DoSysStatus(NetMsg* msg)
{
  if (!msg)
    return;

  NetSysStatus sys_status;
  sys_status.Unpack(msg->Data());

  Ship* ship = FindShipByObjID(sys_status.GetObjID());
  NetPlayer* player = FindPlayerByNetID(msg->NetID());

  if (ship)
  {
    if (!player || ship->GetObjID() != player->GetObjID())
    {
      /**
  DebugTrace("NetGameServer::DoSysStatus - received request for ship '%s' from wrong player %s\n",
      ship->Name(), player ? player->Name() : "null");
  **/

      return;
    }

    player->DoSysStatus(&sys_status);

    // rebroadcast:
    System* sys = ship->GetSystem(sys_status.GetSystem());
    NetUtil::SendSysStatus(ship, sys);
  }

  else
  {
    player = FindZombieByObjID(sys_status.GetObjID());

    if (player)
      SendDisconnect(player);
  }
}

void NetGameServer::DoElemRequest(NetMsg* msg)
{
  if (!msg)
    return;

  NetElemRequest elem_request;
  elem_request.Unpack(msg->Data());

  Sim* sim = Sim::GetSim();
  Element* elem = sim->FindElement(elem_request.GetName());

  if (elem)
  {
    int squadron = -1;
    int slots[] = {-1, -1, -1, -1};
    Ship* carrier = elem->GetCarrier();

    if (carrier && carrier->GetHangar())
    {
      Hangar* hangar = carrier->GetHangar();

      for (int i = 0; i < 4; i++)
        hangar->FindSquadronAndSlot(elem->GetShip(i + 1), squadron, slots[i]);
    }

    NetUtil::SendElemCreate(elem, squadron, slots, false, true);
  }
}

void NetGameServer::DoElemCreate(NetMsg* msg)
{
  if (!msg)
    return;

  NetElemCreate elem_create;
  elem_create.Unpack(msg->Data());

  Sim* sim = Sim::GetSim();
  Element* elem = sim->CreateElement(elem_create.GetName(), elem_create.GetIFF(), elem_create.GetType());

  int* load = elem_create.GetLoadout();
  int* slots = elem_create.GetSlots();
  int squadron = elem_create.GetSquadron();
  int code = elem_create.GetObjCode();
  std::string target = elem_create.GetObjective();
  bool alert = elem_create.GetAlert();

  elem->SetIntelLevel(elem_create.GetIntel());
  elem->SetLoadout(load);

  if (code > Instruction::RTB || target.length() > 0)
  {
    auto obj = NEW Instruction(code, target);
    elem->AddObjective(obj);
  }

  Ship* carrier = sim->FindShip(elem_create.GetCarrier());
  if (carrier)
  {
    elem->SetCarrier(carrier);

    Hangar* hangar = carrier->GetHangar();
    if (hangar)
    {
      std::string squadron_name = hangar->SquadronName(squadron);
      elem->SetSquadron(squadron_name);

      FlightDeck* deck = nullptr;
      int queue = 1000;

      for (int i = 0; i < carrier->NumFlightDecks(); i++)
      {
        FlightDeck* d = carrier->GetFlightDeck(i);

        if (d && d->IsLaunchDeck())
        {
          int dq = hangar->PreflightQueue(d);

          if (dq < queue)
          {
            queue = dq;
            deck = d;
          }
        }
      }

      for (int i = 0; i < 4; i++)
      {
        int slot = slots[i];
        if (slot > -1)
          hangar->GotoAlert(squadron, slot, deck, elem, load, !alert);
      }
    }
  }

  NetUtil::SendElemCreate(elem, elem_create.GetSquadron(), elem_create.GetSlots(), elem_create.GetAlert());
}

void NetGameServer::DoShipLaunch(NetMsg* msg)
{
  if (!msg)
    return;

  NetShipLaunch ship_launch;
  ship_launch.Unpack(msg->Data());

  Sim* sim = Sim::GetSim();
  int squadron = ship_launch.GetSquadron();
  int slot = ship_launch.GetSlot();

  NetPlayer* player = FindPlayerByObjID(ship_launch.GetObjID());
  if (player)
  {
    Ship* carrier = player->GetShip();

    if (carrier)
    {
      Hangar* hangar = carrier->GetHangar();

      if (hangar)
        hangar->Launch(squadron, slot);

      NetUtil::SendShipLaunch(carrier, squadron, slot);
    }
  }
}

void NetGameServer::DoNavData(NetMsg* msg)
{
  if (!msg)
    return;

  NetNavData nav_data;
  nav_data.Unpack(msg->Data());

  Element* elem = sim->FindElement(nav_data.GetElem());

  if (elem)
  {
    if (nav_data.IsAdd())
    {
      auto navpt = NEW Instruction(*nav_data.GetNavPoint());
      Instruction* after = nullptr;
      int index = nav_data.GetIndex();

      if (index >= 0 && index < elem->GetFlightPlan().size())
        after = elem->GetFlightPlan().at(index);

      elem->AddNavPoint(navpt, after, false);
    }

    else
    {
      Instruction* navpt = nav_data.GetNavPoint();
      Instruction* exist = nullptr;
      int index = nav_data.GetIndex();

      if (navpt && index >= 0 && index < elem->GetFlightPlan().size())
      {
        exist = elem->GetFlightPlan().at(index);
        *exist = *navpt;
      }
    }

    SendData(&nav_data);
  }
}

void NetGameServer::DoNavDelete(NetMsg* msg)
{
  if (!msg)
    return;

  NetNavDelete nav_delete;
  nav_delete.Unpack(msg->Data());

  Element* elem = sim->FindElement(nav_delete.GetElem());

  if (elem)
  {
    int index = nav_delete.GetIndex();

    if (index < 0)
      elem->ClearFlightPlan(false);

    else if (index < elem->FlightPlanLength())
    {
      Instruction* npt = elem->GetFlightPlan().at(index);
      elem->DelNavPoint(npt, false);
    }

    SendData(&nav_delete);
  }
}

void NetGameServer::DoWepTrigger(NetMsg* msg)
{
  if (!msg)
    return;

  NetWepTrigger trigger;
  trigger.Unpack(msg->Data());

  NetPlayer* player = FindPlayerByObjID(trigger.GetObjID());
  if (player)
    player->DoWepTrigger(&trigger);
  else
  {
    player = FindZombieByObjID(trigger.GetObjID());

    if (player)
      SendDisconnect(player);
  }
}

void NetGameServer::DoWepRelease(NetMsg* msg)
{
  if (!msg)
    return;

  NetWepRelease release;
  release.Unpack(msg->Data());

  NetPlayer* player = FindPlayerByObjID(release.GetObjID());
  if (player)
    player->DoWepRelease(&release);
  else
  {
    player = FindZombieByObjID(release.GetObjID());

    if (player)
      SendDisconnect(player);
  }

  DebugTrace("WEP RELEASE on server? objid = %d\n", release.GetObjID());
}

void NetGameServer::DoWepDestroy(NetMsg* msg) {}

void NetGameServer::DoCommMsg(NetMsg* msg)
{
  if (!msg)
    return;

  NetCommMsg comm_msg;
  comm_msg.Unpack(msg->Data());

  RadioMessage* radio_msg = comm_msg.GetRadioMessage();

  NetPlayer* player = FindPlayerByObjID(comm_msg.GetObjID());
  if (player && radio_msg)
  {
    player->DoCommMessage(&comm_msg);

    int channel = comm_msg.GetRadioMessage()->Channel();

    ListIter<NetPlayer> dst = players;
    while (++dst)
    {
      NetPlayer* remote_player = dst.value();
      if (remote_player->GetNetID() && (channel == 0 || channel == remote_player->GetIFF()))
      {
        BYTE* data = comm_msg.Pack();
        int size = comm_msg.Length();

        link->SendMessage(remote_player->GetNetID(), data, size, NetMsg::RELIABLE);
      }
    }
  }
  else
  {
    player = FindZombieByObjID(comm_msg.GetObjID());

    if (player)
      SendDisconnect(player);
  }
}

void NetGameServer::DoChatMsg(NetMsg* msg)
{
  if (!msg)
    return;

  NetChatMsg chat_msg;
  chat_msg.Unpack(msg->Data());

  RouteChatMsg(chat_msg);
}

void NetGameServer::RouteChatMsg(NetChatMsg& chat_msg)
{
  DWORD dst_id = chat_msg.GetDstID();

  // broadcast or team:
  if (dst_id == 0xffff || dst_id <= 10)
  {
    BYTE* data = chat_msg.Pack();
    int size = chat_msg.Length();

    ListIter<NetPlayer> dst = players;
    while (++dst)
    {
      NetPlayer* remote_player = dst.value();
      if (remote_player->GetNetID() && chat_msg.GetName() != remote_player->Name())
      {
        if (dst_id == 0xffff || dst_id == 0 || remote_player->GetIFF() == static_cast<int>(dst_id) - 1)
          link->SendMessage(remote_player->GetNetID(), data, size);
      }
    }

    if (local_player && (dst_id == 0xffff || dst_id == 0 || local_player->GetIFF() == static_cast<int>(dst_id) - 1))
    {
      std::string name = chat_msg.GetName();
      if (name.length() < 1)
        name = Game::GetText("NetGameServer.chat.unknown");

      // don't echo general messages from the local player.
      // they are already displayed by the chat entry code
      // in starshatter.cpp

      if (name != player_name)
        HUDView::Message("%s> %s", name.data(), chat_msg.GetText().data());
    }
  }

  // direct to local player:
  else if (local_player && local_player->GetObjID() == dst_id)
  {
    std::string name = chat_msg.GetName();
    if (name.length() < 1)
      name = Game::GetText("NetGameServer.chat.unknown");

    HUDView::Message("%s> %s", name.data(), chat_msg.GetText().data());
  }

  // ship-to-ship, but not to local player:
  else if (!local_player || local_player->GetObjID() != dst_id)
  {
    NetPlayer* remote_player = FindPlayerByObjID(dst_id);
    if (remote_player && remote_player->GetNetID())
    {
      BYTE* data = chat_msg.Pack();
      int size = chat_msg.Length();
      link->SendMessage(remote_player->GetNetID(), data, size);
    }

    // record message in server log:
    DebugTrace("%s> %s\n", chat_msg.GetName().data(), chat_msg.GetText().data());
  }

  if (dst_id == 0xffff)
    return;

  // record message in chat log:
  NetLobbyServer* lobby = NetLobbyServer::GetInstance();

  if (!lobby)
    return;

  NetUser* user = lobby->FindUserByName(chat_msg.GetName());

  if (user)
    lobby->AddChat(user, chat_msg.GetText(), false);   // don't re-route
}

const char* FormatGameTime();

void NetGameServer::DoSelfDestruct(NetMsg* msg)
{
  if (!msg)
    return;

  NetSelfDestruct self_destruct;
  self_destruct.Unpack(msg->Data());

  Ship* ship = FindShipByObjID(self_destruct.GetObjID());
  NetPlayer* player = FindPlayerByNetID(msg->NetID());

  if (ship)
  {
    if (!player || ship->GetObjID() != player->GetObjID())
    {
      DebugTrace("NetGameServer::DoSelfDestruct - received request for ship '%s' from wrong player %s\n", ship->Name(),
                 player ? player->Name() : "null");

      return;
    }

    ship->InflictNetDamage(self_destruct.GetDamage());

    SendData(&self_destruct);

    int ship_destroyed = (!ship->InTransition() && ship->Integrity() < 1.0f);

    // then delete the ship:
    if (ship_destroyed)
    {
      NetUtil::SendObjKill(ship, nullptr, NetObjKill::KILL_MISC);
      DebugTrace("    %s Self Destruct (%s)\n", ship->Name(), FormatGameTime());

      ShipStats* killee = ShipStats::Find(ship->Name());
      if (killee)
        killee->AddEvent(SimEvent::DESTROYED, ship->Name());

      ship->DeathSpiral();
    }
  }
}

void NetGameServer::Send()
{
  if (players.isEmpty())
    return;

  DWORD time = Game::GameTime();

  // don't flood the network...
  if (time - last_send_time < MIN_NET_FRAME)
    return;

  last_send_time = time;

  // tell the remote players where *we* are:
  if (local_player && !local_player->IsNetObserver() && objid)
  {
    double r, p, y;
    local_player->Cam().Orientation().ComputeEulerAngles(r, p, y);

    NetObjLoc obj_loc;
    obj_loc.SetObjID(objid);
    obj_loc.SetLocation(local_player->Location());
    obj_loc.SetVelocity(local_player->Velocity());
    obj_loc.SetOrientation(Point(r, p, y));
    obj_loc.SetThrottle(local_player->Throttle() > 10);
    obj_loc.SetAugmenter(local_player->Augmenter());
    obj_loc.SetGearDown(local_player->IsGearDown());

    Shield* shield = local_player->GetShield();
    if (shield)
      obj_loc.SetShield(static_cast<int>(shield->GetPowerLevel()));
    else
      obj_loc.SetShield(0);

    SendData(&obj_loc);
  }

  // tell each remote player where all the others are:
  ListIter<NetPlayer> src = players;
  while (++src)
  {
    NetPlayer* player = src.value();

    Ship* player_ship = player->GetShip();

    if (player_ship)
    {
      double r, p, y;
      player_ship->Cam().Orientation().ComputeEulerAngles(r, p, y);

      NetObjLoc obj_loc;
      obj_loc.SetObjID(player->GetObjID());
      obj_loc.SetLocation(player_ship->Location());
      obj_loc.SetVelocity(player_ship->Velocity());
      obj_loc.SetOrientation(Point(r, p, y));
      obj_loc.SetThrottle(player_ship->Throttle() > 10);
      obj_loc.SetAugmenter(player_ship->Augmenter());
      obj_loc.SetGearDown(player_ship->IsGearDown());

      Shield* shield = player_ship->GetShield();
      if (shield)
        obj_loc.SetShield(static_cast<int>(shield->GetPowerLevel()));
      else
        obj_loc.SetShield(0);

      BYTE* obj_loc_data = obj_loc.Pack();

      ListIter<NetPlayer> dst = players;
      while (++dst)
      {
        NetPlayer* remote_player = dst.value();
        if (remote_player->GetNetID() && remote_player != player)
          link->SendMessage(remote_player->GetNetID(), obj_loc_data, NetObjLoc::SIZE);
      }
    }
  }

  // tell each remote player where all the A.I. ships are:
  ListIter<Ship> ai_iter = ships;
  while (++ai_iter)
  {
    Ship* s = ai_iter.value();

    if (s && !s->IsStatic())
    {
      double r, p, y;
      s->Cam().Orientation().ComputeEulerAngles(r, p, y);

      NetObjLoc obj_loc;
      obj_loc.SetObjID(s->GetObjID());
      obj_loc.SetLocation(s->Location());
      obj_loc.SetVelocity(s->Velocity());
      obj_loc.SetOrientation(Point(r, p, y));
      obj_loc.SetThrottle(s->Throttle() > 10);
      obj_loc.SetAugmenter(s->Augmenter());
      obj_loc.SetGearDown(s->IsGearDown());

      Shield* shield = s->GetShield();
      if (shield)
        obj_loc.SetShield(static_cast<int>(shield->GetPowerLevel()));
      else
        obj_loc.SetShield(0);

      SendData(&obj_loc);
    }
  }
}

void NetGameServer::SendData(NetData* net_data)
{
  if (net_data)
  {
    BYTE* data = net_data->Pack();
    int size = net_data->Length();
    BYTE flags = 0;
    bool all = true;  // include player with objid in net_data?
    DWORD oid = net_data->GetObjID();

    BYTE msg_type = net_data->Type();

    if (msg_type >= 0x10)
      flags |= NetMsg::RELIABLE;

    if (msg_type == NET_WEP_TRIGGER || msg_type == NET_COMM_MESSAGE || msg_type == NET_CHAT_MESSAGE || msg_type ==
      NET_OBJ_HYPER || msg_type == NET_ELEM_CREATE || msg_type == NET_NAV_DATA || msg_type == NET_NAV_DELETE)
      all = false;

    ListIter<NetPlayer> dst = players;
    while (++dst)
    {
      NetPlayer* remote_player = dst.value();
      if (remote_player->GetNetID() && (all || oid != remote_player->GetObjID()))
        link->SendMessage(remote_player->GetNetID(), data, size, flags);
    }
  }
}

void NetGameServer::SendDisconnect(NetPlayer* zombie)
{
  if (zombie)
  {
    NetDisconnect disconnect;
    BYTE* data = disconnect.Pack();
    int size = disconnect.Length();
    BYTE flags = NetMsg::RELIABLE;

    if (zombie->GetNetID())
      link->SendMessage(zombie->GetNetID(), data, size, flags);
  }
}

bool NetGameServer::Update(SimObject* obj)
{
  if (obj->Type() == SimObject::SIM_SHIP)
  {
    auto s = static_cast<Ship*>(obj);
    if (local_player == s)
      local_player = nullptr;

    if (ships.contains(s))
      ships.remove(s);
  }

  return SimObserver::Update(obj);
}

std::string NetGameServer::GetObserverName() const { return "NetGameServer"; }

void NetGameServer::Respawn(DWORD oid, Ship* spawn)
{
  if (!oid || !spawn)
    return;

  DebugTrace("NetGameServer::Respawn(%d, %s)\n", oid, spawn->Name());
  spawn->SetObjID(oid);
  Observe(spawn);

  NetPlayer* p = FindPlayerByObjID(oid);
  if (p)
    p->SetShip(spawn);
  else
    ships.append(spawn);

  if (objid == oid)
  {
    DebugTrace("  RESPAWN LOCAL PLAYER\n\n");
    local_player = spawn;
  }
}
