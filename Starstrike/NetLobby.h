#ifndef NetLobby_h
#define NetLobby_h

#include "List.h"
#include "NetLink.h"

#define NET_CAMPAIGN_SHIFT    12
#define NET_MISSION_MASK      0xfff
#define NET_DISCONNECT_TIME   30

#undef GetUserName

class Campaign;
class Mission;
class MissionElement;
class MissionInfo;
class NetCampaignInfo;
class NetChatEntry;
class NetUser;
class NetUnitEntry;
class NetLobbyParam;

class NetLobby
{
  public:
    
    NetLobby(bool temporary = false);
    virtual ~NetLobby();

    [[nodiscard]] virtual bool IsClient() const { return false; }
    [[nodiscard]] virtual bool IsServer() const { return false; }
    [[nodiscard]] virtual bool IsActive() const { return active; }

    virtual void ExecFrame();
    virtual void Recv();
    virtual void Send();
    virtual int GetLastError() const { return 0; }

    virtual NetUser* FindUserByAddr(const NetAddr& addr);
    virtual NetUser* FindUserByName(std::string_view name);
    virtual NetUser* FindUserByNetID(DWORD id);

    virtual void BanUser(NetUser* user);
    virtual void AddUser(NetUser* user);
    virtual void DelUser(NetUser* user);
    virtual bool SetUserHost(NetUser* user, bool host);
    virtual int NumUsers();
    virtual NetUser* GetHost();
    virtual bool HasHost();
    [[nodiscard]] virtual bool IsHost() const { return false; }
    virtual List<NetUser>& GetUsers();

    virtual NetUser* GetLocalUser();
    virtual void SetLocalUser(NetUser* user);

    [[nodiscard]] virtual int GetStatus() const { return status; }
    virtual void SetStatus(int s) { status = s; }

    virtual void AddChat(NetUser* user, std::string_view msg, bool route = true);
    virtual List<NetChatEntry>& GetChat();
    virtual void ClearChat();
    virtual void SaveChat() {}
    virtual DWORD GetStartTime() const { return start_time; }

    virtual List<NetCampaignInfo>& GetCampaigns();

    virtual void AddUnitMap(MissionElement* elem, int index = 0);
    virtual List<NetUnitEntry>& GetUnitMap();
    virtual void ClearUnitMap();
    virtual void MapUnit(int n, std::string_view user, bool lock = false);
    virtual void UnmapUnit(std::string_view user);
    virtual bool IsMapped(std::string_view user);

    virtual Mission* GetSelectedMission() { return mission; }
    virtual DWORD GetSelectedMissionID() const { return selected_mission; }
    virtual void SelectMission(DWORD id);

    virtual const std::string& GetMachineInfo() { return machine_info; }
    virtual WORD GetGamePort() { return 0; }

    // actions:
    virtual bool Ping();
    virtual void GameStart() {}
    virtual void GameStop() {}
    virtual DWORD GetLag();

    // instance management:
    static NetLobby* GetInstance();
    static bool IsNetLobbyClient();
    static bool IsNetLobbyServer();

  protected:
    virtual void DoPing([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoServerInfo([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoServerMods([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoLogin([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoLogout([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoAuthUser([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoUserAuth([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoChat([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoUserList([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoBanUser([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoMissionList([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoMissionSelect([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoMissionData([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoUnitList([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoMapUnit([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoGameStart([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoGameStop([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}
    virtual void DoExit([[maybe_unused]] NetPeer* peer, [[maybe_unused]] std::string msg) {}

    virtual void ParseMsg(std::string msg, List<NetLobbyParam>& params);

    NetLink* link;
    NetUser* local_user;
    List<NetUser> users;
    List<NetChatEntry> chat_log;
    List<NetCampaignInfo> campaigns;
    List<NetUnitEntry> unit_map;
    std::string machine_info;

    bool active;
    DWORD last_send_time;
    DWORD start_time;
    DWORD selected_mission;
    Mission* mission;
    int status;
};

class NetLobbyParam
{
  public:
    
    NetLobbyParam(std::string_view n, std::string_view v)
      : name(n),
        value(v) {}

    int operator ==(const NetLobbyParam& p) const { return name == p.name; }

    std::string name;
    std::string value;
};

class NetUnitEntry
{
  public:
    
    NetUnitEntry(MissionElement* elem, int index = 0);
    NetUnitEntry(std::string_view elem, std::string_view design, int iff, int index = 0);
    ~NetUnitEntry();

    int operator ==(const NetUnitEntry& e) const { return (elem == e.elem) && (index == e.index); }

    std::string GetDescription() const;
    const std::string& GetUserName() const { return user; }
    const std::string& GetElemName() const { return elem; }
    const std::string& GetDesign() const { return design; }
    int GetIFF() const { return iff; }
    int GetIndex() const { return index; }
    int GetLives() const { return lives; }
    int GetIntegrity() const { return hull; }
    int GetMissionRole() const { return role; }
    bool GetLocked() const { return lock; }

    void SetUserName(std::string_view u) { user = u; }
    void SetElemName(std::string_view e) { elem = e; }
    void SetDesign(std::string_view d) { design = d; }
    void SetIFF(int i) { iff = i; }
    void SetIndex(int i) { index = i; }
    void SetLives(int i) { lives = i; }
    void SetIntegrity(int i) { hull = i; }
    void SetMissionRole(int i) { role = i; }
    void SetLock(bool l) { lock = l; }

  private:
    std::string user;
    std::string elem;
    std::string design;
    int iff;
    int index;
    int lives;
    int hull;
    int role;
    bool lock;
};

class NetServerInfo
{
  public:
    
    enum STATUS
    {
      OFFLINE,
      LOBBY,
      BRIEFING,
      ACTIVE,
      DEBRIEFING,
      PERSISTENT
    };

    NetServerInfo();

    std::string name;
    std::string hostname;
    std::string password;
    std::string type;
    NetAddr addr;
    WORD port;
    WORD gameport;
    bool save;

    std::string version;
    std::string machine_info;
    int nplayers;
    int hosted;
    int status;

    DWORD ping_time;
};

class NetCampaignInfo
{
  public:
    
    NetCampaignInfo()
      : id(0) {}

    ~NetCampaignInfo() {}

    int id;
    std::string name;

    List<MissionInfo> missions;
};

enum NET_LOBBY_MESSAGES
{
  NET_LOBBY_PING = 0x10,
  NET_LOBBY_SERVER_INFO,
  NET_LOBBY_SERVER_MODS,
  NET_LOBBY_LOGIN,
  NET_LOBBY_LOGOUT,
  NET_LOBBY_CHAT,
  NET_LOBBY_USER_LIST,
  NET_LOBBY_BAN_USER,
  NET_LOBBY_MISSION_LIST,
  NET_LOBBY_MISSION_SELECT,
  NET_LOBBY_MISSION_DATA,
  NET_LOBBY_UNIT_LIST,
  NET_LOBBY_MAP_UNIT,
  NET_LOBBY_AUTH_USER,
  NET_LOBBY_USER_AUTH,
  NET_LOBBY_GAME_START,
  NET_LOBBY_GAME_STOP,
  NET_LOBBY_EXIT
};

#endif NetLobby_h
