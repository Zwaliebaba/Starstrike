#ifndef NetLobbyServer_h
#define NetLobbyServer_h

#include "NetLobby.h"
#include "NetServerConfig.h"

class NetLobbyServer : public NetLobby
{
  public:
    NetLobbyServer();
    ~NetLobbyServer() override;

    int operator ==(const NetLobbyServer& s) const { return this == &s; }

    void ExecFrame() override;
    bool IsHost() const override { return true; }
    bool IsServer() const override { return true; }

    void BanUser(NetUser* user) override;
    void AddUser(NetUser* user) override;
    void DelUser(NetUser* user) override;
    virtual void SendUsers();
    virtual void RequestAuth(NetUser* user);

    void AddChat(NetUser* user, std::string_view msg, bool route = true) override;
    void ClearChat() override;
    void SaveChat() override;

    List<NetUnitEntry>& GetUnitMap() override;
    void MapUnit(int n, std::string_view user, bool lock = false) override;
    void UnmapUnit(std::string_view user) override;
    virtual void SendUnits();

    void SelectMission(DWORD id) override;
    virtual std::string Serialize(Mission* m, NetUser* u = nullptr);
    Mission* GetSelectedMission() override;

    virtual std::string GetServerName() const { return m_serverName; }
    virtual void SetServerName(std::string_view s) { m_serverName = s; }
    virtual std::string GetServerMission() const { return server_mission; }
    virtual void SetServerMission(std::string_view script) { server_mission = script; }

    void GameStart() override;
    void GameStop() override;

    virtual void GameOn();
    virtual void GameOff();

    // singleton locator:
    static NetLobbyServer* GetInstance();

  protected:
    virtual void CheckSessions();

    virtual void SendData(NetUser* dst, int type, std::string msg);
    void DoPing(NetPeer* peer, std::string msg) override;
    void DoServerInfo(NetPeer* peer, std::string msg) override;
    void DoLogin(NetPeer* peer, std::string msg) override;
    void DoLogout(NetPeer* peer, std::string msg) override;
    void DoUserAuth(NetPeer* _peer, std::string _msg) override;
    void DoChat(NetPeer* peer, std::string msg) override;
    void DoUserList(NetPeer* peer, std::string msg) override;
    void DoBanUser(NetPeer* peer, std::string msg) override;
    void DoMissionList(NetPeer* peer, std::string msg) override;
    void DoMissionSelect(NetPeer* peer, std::string msg) override;
    void DoMissionData(NetPeer* peer, std::string msg) override;
    void DoUnitList(NetPeer* peer, std::string msg) override;
    void DoMapUnit(NetPeer* peer, std::string msg) override;
    void DoGameStart(NetPeer* peer, std::string msg) override;
    void DoGameStop(NetPeer* peer, std::string msg) override;

    virtual void LoadMOTD();
    virtual void SendMOTD(NetUser* user);

    std::string m_serverName;
    NetAddr m_serverAddr;
    DWORD announce_time;
    NetServerConfig* m_serverConfig;
    std::string server_mission;
    int m_motdIndex;

    std::vector<std::string> m_motd;
};

#endif NetLobbyServer_h
