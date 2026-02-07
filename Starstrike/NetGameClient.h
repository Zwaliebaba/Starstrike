#ifndef NetGameClient_h
#define NetGameClient_h

#include "NetGame.h"
#include "SimObject.h"

class NetJoinAnnounce;

class NetGameClient : public NetGame, public SimObserver
{
  public:
    NetGameClient();
    ~NetGameClient() override;

    bool IsClient() const override { return true; }
    bool IsServer() const override { return false; }

    void ExecFrame() override;
    void Send() override;
    void SendData(NetData* data) override;
    void Respawn(DWORD objid, Ship* spawn) override;

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

  protected:
    void DoJoinRequest(NetMsg* msg) override;
    void DoJoinAnnounce(NetMsg* msg) override;
    void DoQuitRequest(NetMsg* msg) override;
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

    virtual void SendJoinRequest();

    virtual bool DoJoinBacklog(NetJoinAnnounce* join_ann);

    DWORD server_id;
    DWORD join_req_time;
    List<NetJoinAnnounce> join_backlog;
};

#endif NetGameClient_h
