#ifndef NetGameServer_h
#define NetGameServer_h

#include "NetGame.h"
#include "SimObject.h"

class NetChatMsg;

class NetGameServer : public NetGame, public SimObserver
{
  public:
    NetGameServer();
    ~NetGameServer() override;

    bool IsClient() const override { return false; }
    bool IsServer() const override { return true; }

    void ExecFrame() override;
    virtual void CheckSessions();

    void Send() override;
    void SendData(NetData* data) override;
    void Respawn(DWORD objid, Ship* spawn) override;

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

    virtual void RouteChatMsg(NetChatMsg& chat_msg);

  protected:
    void DoJoinRequest(NetMsg* msg) override;
    void DoJoinAnnounce(NetMsg* msg) override;
    void DoQuitRequest(NetMsg* _msg) override;
    void DoQuitAnnounce(NetMsg* msg) override;
    void DoGameOver(NetMsg* msg) override;
    void DoDisconnect(NetMsg* msg) override;

    void DoObjLoc(NetMsg* msg) override;
    void DoObjDamage(NetMsg* msg) override;
    void DoObjKill(NetMsg* msg) override;
    void DoObjSpawn(NetMsg* msg) override;
    void DoObjHyper(NetMsg* msg) override;
    void DoObjTarget(NetMsg* msg) override;
    void DoObjEmcon(NetMsg* msg) override;
    void DoSysDamage(NetMsg* msg) override;
    void DoSysStatus(NetMsg* msg) override;

    void DoElemRequest(NetMsg* msg) override;
    void DoElemCreate(NetMsg* msg) override;
    void DoShipLaunch(NetMsg* msg) override;
    void DoNavData(NetMsg* msg) override;
    void DoNavDelete(NetMsg* msg) override;

    void DoWepTrigger(NetMsg* msg) override;
    void DoWepRelease(NetMsg* msg) override;
    void DoWepDestroy(NetMsg* msg) override;

    void DoCommMsg(NetMsg* msg) override;
    void DoChatMsg(NetMsg* msg) override;
    void DoSelfDestruct(NetMsg* msg) override;

    virtual NetPlayer* FindZombieByObjID(DWORD objid);
    virtual void SendDisconnect(NetPlayer* zombie);

    List<Ship> ships;
    List<NetPlayer> zombies;
};

#endif NetGameServer_h
