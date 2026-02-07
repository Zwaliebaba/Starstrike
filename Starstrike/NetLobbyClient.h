#ifndef NetLobbyClient_h
#define NetLobbyClient_h

#include "NetLobby.h"

class NetLobbyClient : public NetLobby
{
  public:
    NetLobbyClient();
    NetLobbyClient(const NetAddr& server_addr);
    ~NetLobbyClient() override;

    int operator ==(const NetLobbyClient& c) const { return this == &c; }

    void ExecFrame() override;
    virtual bool Login(bool host = false);
    virtual bool Logout();
    int GetLastError() const override { return exit_code; }

    // actions:
    bool Ping() override;
    void GameStart() override;
    void GameStop() override;

    void BanUser(NetUser* user) override;

    void AddChat(NetUser* user, std::string_view msg, bool route = true) override;
    List<NetChatEntry>& GetChat() override;

    NetAddr GetServerAddr() const { return addr; }
    bool IsHost() const override { return host; }
    bool IsClient() const override { return true; }

    List<NetUser>& GetUsers() override;
    List<NetCampaignInfo>& GetCampaigns() override;
    List<NetUnitEntry>& GetUnitMap() override;
    void MapUnit(int n, std::string_view user, bool lock = false) override;
    void SelectMission(DWORD id) override;
    Mission* GetSelectedMission() override;

    // overrides for ping support:
    const std::string& GetMachineInfo() override;
    int GetStatus() const override;
    int NumUsers() override;
    bool HasHost() override;
    WORD GetGamePort() override;

  protected:
    virtual void SendData(int type, std::string msg);
    void DoServerInfo(NetPeer* peer, std::string msg) override;
    void DoAuthUser(NetPeer* peer, std::string msg) override;
    void DoChat(NetPeer* peer, std::string msg) override;
    void DoUserList(NetPeer* peer, std::string msg) override;
    void DoMissionList(NetPeer* peer, std::string msg) override;
    void DoMissionSelect(NetPeer* peer, std::string msg) override;
    void DoMissionData(NetPeer* peer, std::string msg) override;
    void DoUnitList(NetPeer* peer, std::string msg) override;
    void DoMapUnit(NetPeer* peer, std::string msg) override;
    void DoGameStart(NetPeer* peer, std::string msg) override;
    void DoExit(NetPeer* peer, std::string msg) override;

    DWORD server_id;
    NetAddr addr;
    bool host;
    std::string gamepass;
    int exit_code;

    NetServerInfo server_info;
    List<MissionInfo> missions;

    bool temporary;
    DWORD ping_req_time;
    DWORD chat_req_time;
    DWORD user_req_time;
    DWORD camp_req_time;
    DWORD unit_req_time;
    DWORD mods_req_time;
};

#endif NetLobbyClient_h
