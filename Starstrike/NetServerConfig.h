#ifndef NetServerConfig_h
#define NetServerConfig_h

#include "Game.h"
#include "NetUser.h"


class NetServerConfig
{
  public:
    
    NetServerConfig();
    ~NetServerConfig();

    enum GAME_TYPE
    {
      NET_GAME_LAN,
      NET_GAME_PRIVATE,
      NET_GAME_PUBLIC
    };

    std::string Name() const { return name; }
    std::string GetAdminName() const { return admin_name; }
    std::string GetAdminPass() const { return admin_pass; }
    std::string GetGamePass() const { return game_pass; }
    std::string GetMission() const { return mission; }
    WORD GetAdminPort() const { return admin_port; }
    WORD GetLobbyPort() const { return lobby_port; }
    WORD GetGamePort() const { return game_port; }
    int GetPoolsize() const { return poolsize; }
    int GetSessionTimeout() const { return session_timeout; }
    int GetGameType() const { return game_type; }
    int GetAuthLevel() const { return auth_level; }

    void SetName(std::string_view s) { name = Clean(s); }
    void SetAdminName(std::string_view s) { admin_name = Clean(s); }
    void SetAdminPass(std::string_view s) { admin_pass = Clean(s); }
    void SetGamePass(std::string_view s) { game_pass = Clean(s); }
    void SetMission(std::string_view s) { mission = Clean(s); }
    void SetGameType(int t) { game_type = t; }
    void SetAdminPort(WORD p) { admin_port = p; }
    void SetLobbyPort(WORD p) { lobby_port = p; }
    void SetGamePort(WORD p) { game_port = p; }
    void SetPoolsize(int s) { poolsize = s; }
    void SetSessionTimeout(int t) { session_timeout = t; }
    void SetAuthLevel(int n) { auth_level = n; }

    void Load();
    void Save();

    bool IsUserBanned(const NetUser* user);
    void BanUser(NetUser* user);

    static void Initialize();
    static void Close();
    static NetServerConfig* GetInstance() { return instance; }

  private:
    void LoadBanList();
    std::string Clean(std::string_view s);

    std::string name;
    std::string admin_name;
    std::string admin_pass;
    std::string game_pass;
    std::string mission;

    WORD admin_port;
    WORD lobby_port;
    WORD game_port;
    int poolsize;
    int session_timeout;
    int game_type;
    int auth_level;

    List<NetAddr> banned_addrs;
    std::vector<std::string> banned_names;

    static NetServerConfig* instance;
};

#endif NetServerConfig_h
