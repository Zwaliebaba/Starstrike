#ifndef NetGame_h
#define NetGame_h

#include "List.h"
#include "NetLink.h"

class Sim;
class Ship;
class Shot;
class NetData;
class NetMsg;
class NetPlayer;

class NetGame
{
  public:
    
    enum
    {
      SHIP,
      SHOT
    };

    NetGame();
    virtual ~NetGame();

    virtual bool IsClient() const { return false; }
    virtual bool IsServer() const { return false; }
    virtual bool IsActive() const { return active; }

    virtual DWORD GetNetID() const { return netid; }
    virtual DWORD GetObjID() const;

    virtual void ExecFrame();
    virtual void Recv();
    virtual void Send();

    virtual void SendData([[maybe_unused]] NetData* data) {}

    virtual NetPlayer* FindPlayerByName(std::string_view name);
    virtual NetPlayer* FindPlayerByNetID(DWORD netid);
    virtual NetPlayer* FindPlayerByObjID(DWORD objid);
    virtual Ship* FindShipByObjID(DWORD objid);
    virtual Shot* FindShotByObjID(DWORD objid);

    virtual NetPeer* GetPeer(NetPlayer* player);

    virtual void Respawn(DWORD objid, Ship* spawn);

    static NetGame* Create();
    static NetGame* GetInstance();
    static bool IsNetGame();
    static bool IsNetGameClient();
    static bool IsNetGameServer();
    static int NumPlayers();

    static DWORD GetNextObjID(int type = SHIP);

  protected:
    virtual void DoJoinRequest([[maybe_unused]] NetMsg* msg) {}
    virtual void DoJoinAnnounce([[maybe_unused]] NetMsg* msg) {}
    virtual void DoQuitRequest([[maybe_unused]] NetMsg* msg) {}
    virtual void DoQuitAnnounce([[maybe_unused]] NetMsg* msg) {}
    virtual void DoGameOver([[maybe_unused]] NetMsg* msg) {}
    virtual void DoDisconnect([[maybe_unused]] NetMsg* msg) {}

    virtual void DoObjLoc([[maybe_unused]] NetMsg* msg) {}
    virtual void DoObjDamage([[maybe_unused]] NetMsg* msg) {}
    virtual void DoObjKill([[maybe_unused]] NetMsg* msg) {}
    virtual void DoObjSpawn([[maybe_unused]] NetMsg* msg) {}
    virtual void DoObjHyper([[maybe_unused]] NetMsg* msg) {}
    virtual void DoObjTarget([[maybe_unused]] NetMsg* msg) {}
    virtual void DoObjEmcon([[maybe_unused]] NetMsg* msg) {}
    virtual void DoSysDamage([[maybe_unused]] NetMsg* msg) {}
    virtual void DoSysStatus([[maybe_unused]] NetMsg* msg) {}

    virtual void DoElemCreate([[maybe_unused]] NetMsg* msg) {}
    virtual void DoElemRequest([[maybe_unused]] NetMsg* msg) {}
    virtual void DoShipLaunch([[maybe_unused]] NetMsg* msg) {}
    virtual void DoNavData([[maybe_unused]] NetMsg* msg) {}
    virtual void DoNavDelete([[maybe_unused]] NetMsg* msg) {}

    virtual void DoWepTrigger([[maybe_unused]] NetMsg* msg) {}
    virtual void DoWepRelease([[maybe_unused]] NetMsg* msg) {}
    virtual void DoWepDestroy([[maybe_unused]] NetMsg* msg) {}

    virtual void DoCommMsg([[maybe_unused]] NetMsg* msg) {}
    virtual void DoChatMsg([[maybe_unused]] NetMsg* msg) {}
    virtual void DoSelfDestruct([[maybe_unused]] NetMsg* msg) {}

    List<NetPlayer> players;
    NetLink* link;

    DWORD objid;
    DWORD netid;
    Ship* local_player;
    std::string player_name;
    std::string player_pass;
    Ship* target;
    Sim* sim;
    bool active;

    DWORD last_send_time;
};

#endif NetGame_h
