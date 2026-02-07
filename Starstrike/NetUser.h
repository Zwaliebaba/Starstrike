#ifndef NetUser_h
#define NetUser_h

#include "Color.h"
#include "NetAddr.h"

class Player;

class NetUser
{
  public:
    NetUser(std::string_view name);
    NetUser(const Player* player);
    virtual ~NetUser();

    int operator ==(const NetUser& u) const { return this == &u; }

    std::string Name() const { return name; }
    std::string Pass() const { return pass; }
    const NetAddr& GetAddress() const { return addr; }
    Color GetColor() const { return color; }
    std::string GetSessionID() const { return session_id; }
    DWORD GetNetID() const { return netid; }
    bool IsHost() const { return host; }

    int AuthLevel() const { return auth_level; }
    int AuthState() const { return auth_state; }
    std::string Salt() const { return salt; }
    bool IsAuthOK() const;

    std::string Squadron() const { return squadron; }
    std::string Signature() const { return signature; }
    int Rank() const { return rank; }
    int FlightTime() const { return flight_time; }
    int Missions() const { return missions; }
    int Kills() const { return kills; }
    int Losses() const { return losses; }

    void SetName(std::string_view n) { name = n; }
    void SetPass(std::string_view p) { pass = p; }
    void SetAddress(const NetAddr& a) { addr = a; }

    void SetColor(Color c) { color = c; }
    void SetNetID(DWORD id) { netid = id; }
    void SetSessionID(std::string s) { session_id = s; }
    void SetHost(bool h) { host = h; }

    void SetAuthLevel(int n) { auth_level = n; }
    void SetAuthState(int n) { auth_state = n; }
    void SetSalt(std::string_view s) { salt = s; }

    void SetSquadron(std::string_view s) { squadron = s; }
    void SetSignature(std::string_view s) { signature = s; }
    void SetRank(int n) { rank = n; }
    void SetFlightTime(int n) { flight_time = n; }
    void SetMissions(int n) { missions = n; }
    void SetKills(int n) { kills = n; }
    void SetLosses(int n) { losses = n; }

    std::string GetDescription();

  protected:
    std::string name;
    std::string pass;
    std::string session_id;
    NetAddr addr;
    DWORD netid;
    Color color;
    bool host;

    // authentication:
    int auth_state;
    int auth_level;
    std::string salt;

    // stats:
    std::string squadron;
    std::string signature;
    int rank;
    int flight_time;
    int missions;
    int kills;
    int losses;
};

#endif NetUser_h
