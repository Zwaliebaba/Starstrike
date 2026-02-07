#ifndef NetData_h
#define NetData_h

#include "Geometry.h"

// UNRELIABLE: 0x01 - 0x0F

constexpr BYTE NET_PING = 0x01;
constexpr BYTE NET_PONG = 0x02;
constexpr BYTE NET_OBJ_LOC = 0x03;

// RELIABLE:   0x10 - 0x7F

constexpr BYTE NET_JOIN_REQUEST = 0x10;
constexpr BYTE NET_JOIN_ANNOUNCE = 0x11;
constexpr BYTE NET_QUIT_REQUEST = 0x12;
constexpr BYTE NET_QUIT_ANNOUNCE = 0x13;
constexpr BYTE NET_KICK_REQUEST = 0x14;
constexpr BYTE NET_KICK_ANNOUNCE = 0x15;
constexpr BYTE NET_GAME_OVER = 0x16;
constexpr BYTE NET_COMM_MESSAGE = 0x17;
constexpr BYTE NET_CHAT_MESSAGE = 0x18;
constexpr BYTE NET_DISCONNECT = 0x19;

constexpr BYTE NET_OBJ_DAMAGE = 0x20;
constexpr BYTE NET_OBJ_KILL = 0x21;
constexpr BYTE NET_OBJ_SPAWN = 0x22;
constexpr BYTE NET_OBJ_HYPER = 0x23;
constexpr BYTE NET_OBJ_TARGET = 0x24;
constexpr BYTE NET_OBJ_EMCON = 0x25;
constexpr BYTE NET_SYS_DAMAGE = 0x26;
constexpr BYTE NET_SYS_STATUS = 0x27;

constexpr BYTE NET_ELEM_CREATE = 0x28;
constexpr BYTE NET_SHIP_LAUNCH = 0x29;
constexpr BYTE NET_NAV_DATA = 0x2A;
constexpr BYTE NET_NAV_DELETE = 0x2B;
constexpr BYTE NET_ELEM_REQUEST = 0x2C;

constexpr BYTE NET_WEP_TRIGGER = 0x30;
constexpr BYTE NET_WEP_RELEASE = 0x31;
constexpr BYTE NET_WEP_DESTROY = 0x32;

constexpr BYTE NET_SELF_DESTRUCT = 0x3F;

class Ship;

class NetData
{
  public:
    NetData() {}
    virtual ~NetData() {}

    virtual int Type() const { return 0; }
    virtual int Length() const { return 0; }
    virtual BYTE* Pack() { return nullptr; }
    virtual bool Unpack(const BYTE* data) { return false; }

    virtual DWORD GetObjID() const { return 0; }
    virtual void SetObjID(DWORD o) {}
};

class NetObjLoc : public NetData
{
  public:
    NetObjLoc()
      : objid(0),
        throttle(false),
        augmenter(false),
        shield(0) {}

    NetObjLoc(DWORD oid, const Point& pos, const Point& orient, const Point& vel)
      : objid(oid),
        location(pos),
        velocity(vel),
        euler(orient),
        throttle(false),
        augmenter(false),
        gear(false),
        shield(0) {}

    enum
    {
      TYPE = NET_OBJ_LOC,
      SIZE = 24
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD id) override { objid = id; }

    Point GetLocation() const { return location; }
    Point GetVelocity() const { return velocity; }
    Point GetOrientation() const { return euler; }
    bool GetThrottle() const { return throttle; }
    bool GetAugmenter() const { return augmenter; }
    bool GetGearDown() const { return gear; }
    int GetShield() const { return shield; }

    void SetLocation(const Point& loc) { location = loc; }
    void SetVelocity(const Point& v) { velocity = v; }
    void SetOrientation(const Point& o) { euler = o; }
    void SetThrottle(bool t) { throttle = t; }
    void SetAugmenter(bool a) { augmenter = a; }
    void SetGearDown(bool g) { gear = g; }
    void SetShield(int s) { shield = s; }

  private:
    DWORD objid;
    Point location;
    Point velocity;
    Point euler;
    bool throttle;
    bool augmenter;
    bool gear;
    int shield;

    BYTE data[SIZE];
};

class NetJoinRequest : public NetData
{
  public:
    NetJoinRequest()
      : index(0) {}

    enum
    {
      TYPE = NET_JOIN_REQUEST,
      SIZE = 128
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    std::string GetName() const { return name; }
    std::string GetPassword() const { return pass; }
    std::string GetSerialNumber() const { return serno; }
    std::string GetElement() const { return elem; }
    int GetIndex() const { return index; }

    void SetName(std::string_view n) { name = n; }
    void SetPassword(std::string_view p) { pass = p; }
    void SetSerialNumber(std::string_view s) { serno = s; }
    void SetElement(std::string_view n) { elem = n; }
    void SetIndex(int n) { index = n; }

  private:
    std::string name;    // callsign
    std::string pass;    // password
    std::string serno;   // box cdkey
    std::string elem;    // element to join
    int index;   // one-based index

    BYTE data[SIZE];
};

class NetJoinAnnounce : public NetData
{
  public:
    NetJoinAnnounce();

    enum
    {
      TYPE = NET_JOIN_ANNOUNCE,
      SIZE = 200
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    std::string GetName() const { return name; }
    std::string GetElement() const { return elem; }
    std::string GetRegion() const { return region; }
    const Point& GetLocation() const { return loc; }
    const Point& GetVelocity() const { return velocity; }
    int GetIndex() const { return index; }
    double GetIntegrity() const { return integrity; }
    int GetRespawns() const { return respawns; }
    int GetDecoys() const { return decoys; }
    int GetProbes() const { return probes; }
    int GetFuel() const { return fuel; }
    int GetShield() const { return shield; }
    const int* GetAmmo() const { return ammo; }

    void SetShip(Ship* s);

    void SetName(std::string_view n) { name = n; }
    void SetElement(std::string_view n) { elem = n; }
    void SetRegion(std::string_view r) { region = r; }
    void SetLocation(const Point& l) { loc = l; }
    void SetVelocity(const Point& v) { velocity = v; }
    void SetIndex(int n) { index = n; }
    void SetIntegrity(double n) { integrity = static_cast<float>(n); }
    void SetRespawns(int n) { respawns = n; }
    void SetDecoys(int n) { decoys = n; }
    void SetProbes(int n) { probes = n; }
    void SetFuel(int n) { fuel = n; }
    void SetShield(int n) { shield = n; }
    void SetAmmo(const int* a);

    virtual DWORD GetNetID() const { return nid; }
    virtual void SetNetID(DWORD n) { nid = n; }

  private:
    std::string name;       // callsign
    std::string elem;       // element to join
    std::string region;     // region ship is in
    Point loc;        // location of ship
    Point velocity;   // velocity of ship
    int index;      // one-based index
    float integrity;  // hull integrity
    int respawns;
    int decoys;
    int probes;
    int fuel;
    int shield;
    int ammo[16];
    DWORD objid;
    DWORD nid;        // not sent over network

    BYTE data[SIZE];
};

class NetQuitAnnounce : public NetData
{
  public:
    NetQuitAnnounce()
      : objid(0),
        disconnected(false) {}

    enum
    {
      TYPE = NET_QUIT_ANNOUNCE,
      SIZE = 5
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }
    virtual bool GetDisconnected() const { return disconnected; }
    virtual void SetDisconnected(bool d) { disconnected = d; }

  private:
    DWORD objid;
    bool disconnected;

    BYTE data[SIZE];
};

class NetDisconnect : public NetData
{
  public:
    NetDisconnect() {}

    enum
    {
      TYPE = NET_DISCONNECT,
      SIZE = 2
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

  private:
    BYTE data[SIZE];
};

class NetObjDamage : public NetData
{
  public:
    NetObjDamage()
      : objid(0),
        damage(0),
        shotid(0) {}

    enum
    {
      TYPE = NET_OBJ_DAMAGE,
      SIZE = 12
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    virtual DWORD GetShotID() const { return shotid; }
    virtual void SetShotID(DWORD o) { shotid = o; }

    float GetDamage() const { return damage; }
    void SetDamage(float d) { damage = d; }

  private:
    DWORD objid;
    float damage;
    DWORD shotid;

    BYTE data[SIZE];
};

class NetObjKill : public NetData
{
  public:
    NetObjKill()
      : objid(0),
        kill_id(0),
        killtype(0),
        respawn(false),
        deck(0) {}

    enum
    {
      TYPE      = NET_OBJ_KILL,
      SIZE      = 24,
      KILL_MISC = 0,
      KILL_PRIMARY,
      KILL_SECONDARY,
      KILL_COLLISION,
      KILL_CRASH,
      KILL_DOCK
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }
    virtual DWORD GetKillerID() const { return kill_id; }
    virtual void SetKillerID(DWORD o) { kill_id = o; }
    virtual int GetKillType() const { return killtype; }
    virtual void SetKillType(int t) { killtype = t; }
    virtual bool GetRespawn() const { return respawn; }
    virtual void SetRespawn(bool r) { respawn = r; }
    virtual Point GetRespawnLoc() const { return loc; }
    virtual void SetRespawnLoc(const Point& p) { loc = p; }
    virtual int GetFlightDeck() const { return deck; }
    virtual void SetFlightDeck(int n) { deck = n; }

  private:
    DWORD objid;
    DWORD kill_id;
    int killtype;
    bool respawn;
    Point loc;
    int deck;

    BYTE data[SIZE];
};

class NetObjHyper : public NetData
{
  public:
    NetObjHyper()
      : objid(0),
        fc_src(0),
        fc_dst(0),
        transtype(0) {}

    enum
    {
      TYPE = NET_OBJ_HYPER,
      SIZE = 56
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD id) override { objid = id; }

    const Point& GetLocation() const { return location; }
    std::string GetRegion() const { return region; }
    DWORD GetFarcaster1() const { return fc_src; }
    DWORD GetFarcaster2() const { return fc_dst; }
    int GetTransitionType() const { return transtype; }

    void SetLocation(const Point& loc) { location = loc; }
    void SetRegion(std::string_view rgn) { region = rgn; }
    void SetFarcaster1(DWORD f) { fc_src = f; }
    void SetFarcaster2(DWORD f) { fc_dst = f; }
    void SetTransitionType(int t) { transtype = t; }

  private:
    DWORD objid;
    Point location;
    std::string region;
    DWORD fc_src;
    DWORD fc_dst;
    int transtype;

    BYTE data[SIZE];
};

class NetObjTarget : public NetData
{
  public:
    NetObjTarget()
      : objid(0),
        tgtid(0),
        sysix(0) {}

    enum
    {
      TYPE = NET_OBJ_TARGET,
      SIZE = 7
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    DWORD GetTgtID() const { return tgtid; }
    void SetTgtID(DWORD o) { tgtid = o; }
    int GetSubtarget() const { return sysix; }
    void SetSubtarget(int n) { sysix = n; }

  private:
    DWORD objid;
    DWORD tgtid;
    int sysix;

    BYTE data[SIZE];
};

class NetObjEmcon : public NetData
{
  public:
    NetObjEmcon()
      : objid(0),
        emcon(0) {}

    enum
    {
      TYPE = NET_OBJ_EMCON,
      SIZE = 5
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    int GetEMCON() const { return emcon; }
    void SetEMCON(int n) { emcon = n; }

  private:
    DWORD objid;
    int emcon;

    BYTE data[SIZE];
};

class NetSysDamage : public NetData
{
  public:
    NetSysDamage()
      : objid(0),
        sysix(-1),
        dmgtype(0),
        damage(0) {}

    enum
    {
      TYPE = NET_SYS_DAMAGE,
      SIZE = 12
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    int GetSystem() const { return sysix; }
    void SetSystem(int n) { sysix = n; }
    BYTE GetDamageType() const { return dmgtype; }
    void SetDamageType(BYTE t) { dmgtype = t; }
    double GetDamage() const { return damage; }
    void SetDamage(double d) { damage = d; }

  private:
    DWORD objid;
    int sysix;
    BYTE dmgtype;
    double damage;

    BYTE data[SIZE];
};

class NetSysStatus : public NetData
{
  public:
    NetSysStatus()
      : objid(0),
        sysix(-1),
        status(0),
        power(0),
        reactor(0),
        avail(1) {}

    enum
    {
      TYPE = NET_SYS_STATUS,
      SIZE = 12
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    int GetSystem() const { return sysix; }
    void SetSystem(int n) { sysix = n; }
    BYTE GetStatus() const { return status; }
    void SetStatus(BYTE s) { status = s; }
    int GetPower() const { return power; }
    void SetPower(int n) { power = n; }
    int GetReactor() const { return reactor; }
    void SetReactor(int n) { reactor = n; }
    double GetAvailability() const { return avail; }
    void SetAvailablility(double a) { avail = a; }

  private:
    DWORD objid;
    int sysix;
    int status;
    int power;
    int reactor;
    double avail;

    BYTE data[SIZE];
};

class NetWepTrigger : public NetData
{
  public:
    NetWepTrigger()
      : objid(0),
        tgtid(0),
        sysix(-1),
        index(0),
        count(0),
        decoy(false),
        probe(false) {}

    enum
    {
      TYPE = NET_WEP_TRIGGER,
      SIZE = 10
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    DWORD GetTgtID() const { return tgtid; }
    void SetTgtID(DWORD o) { tgtid = o; }
    int GetSubtarget() const { return sysix; }
    void SetSubtarget(int n) { sysix = n; }
    int GetIndex() const { return index; }
    void SetIndex(int n) { index = n; }
    int GetCount() const { return count; }
    void SetCount(int n) { count = n; }
    bool GetDecoy() const { return decoy; }
    void SetDecoy(bool d) { decoy = d; }
    bool GetProbe() const { return probe; }
    void SetProbe(bool p) { probe = p; }

  private:
    DWORD objid;
    DWORD tgtid;
    int sysix;
    int index;
    int count;
    bool decoy;
    bool probe;

    BYTE data[SIZE];
};

class NetWepRelease : public NetData
{
  public:
    NetWepRelease()
      : objid(0),
        tgtid(0),
        wepid(0),
        sysix(-1),
        index(0),
        decoy(false),
        probe(false) {}

    enum
    {
      TYPE = NET_WEP_RELEASE,
      SIZE = 11
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    DWORD GetTgtID() const { return tgtid; }
    void SetTgtID(DWORD o) { tgtid = o; }
    int GetSubtarget() const { return sysix; }
    void SetSubtarget(int n) { sysix = n; }
    DWORD GetWepID() const { return wepid; }
    void SetWepID(DWORD o) { wepid = o; }
    int GetIndex() const { return index; }
    void SetIndex(int n) { index = n; }
    bool GetDecoy() const { return decoy; }
    void SetDecoy(bool d) { decoy = d; }
    bool GetProbe() const { return probe; }
    void SetProbe(bool p) { probe = p; }

  private:
    DWORD objid;
    DWORD tgtid;
    DWORD wepid;
    int sysix;
    int index;
    bool decoy;
    bool probe;

    BYTE data[SIZE];
};

class NetWepDestroy : public NetData
{
  public:
    NetWepDestroy()
      : objid(0) {}

    enum
    {
      TYPE = NET_WEP_DESTROY,
      SIZE = 4
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

  private:
    DWORD objid;

    BYTE data[SIZE];
};

class RadioMessage;

class NetCommMsg : public NetData
{
  public:
    NetCommMsg()
      : objid(0),
        radio_message(nullptr),
        length(0) {}

    ~NetCommMsg() override;

    enum
    {
      TYPE = NET_COMM_MESSAGE,
      SIZE = 200
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return length; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    RadioMessage* GetRadioMessage() { return radio_message; }
    void SetRadioMessage(RadioMessage* m);

  private:
    DWORD objid;
    RadioMessage* radio_message;

    int length;
    BYTE data[SIZE];
};

class NetChatMsg : public NetData
{
  public:
    NetChatMsg()
      : dstid(0),
        length(0) {}

    enum
    {
      TYPE     = NET_CHAT_MESSAGE,
      SIZE     = 210,
      MAX_CHAT = 160,
      HDR_LEN  = 4,
      NAME_LEN = 32
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return length; }

    virtual DWORD GetDstID() const { return dstid; }
    virtual void SetDstID(DWORD d) { dstid = d; }
    std::string GetName() const { return name; }
    void SetName(std::string_view m) { name = m; }
    std::string GetText() const { return text; }
    void SetText(std::string_view m) { text = m; }

  private:
    DWORD dstid;
    std::string name;
    std::string text;

    int length;
    BYTE data[SIZE];
};

class NetElemRequest : public NetData
{
  public:
    NetElemRequest();

    enum
    {
      TYPE     = NET_ELEM_REQUEST,
      SIZE     = 64,
      NAME_LEN = 32
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    std::string GetName() const { return name; }
    void SetName(std::string_view m) { name = m; }

  private:
    std::string name;

    BYTE data[SIZE];
};

class NetElemCreate : public NetData
{
  public:
    NetElemCreate();

    enum
    {
      TYPE     = NET_ELEM_CREATE,
      SIZE     = 192,
      NAME_LEN = 32
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    std::string GetName() const { return name; }
    void SetName(std::string_view m) { name = m; }
    std::string GetCommander() const { return commander; }
    void SetCommander(std::string_view m) { commander = m; }
    std::string GetObjective() const { return objective; }
    void SetObjective(std::string_view m) { objective = m; }
    std::string GetCarrier() const { return carrier; }
    void SetCarrier(std::string_view m) { carrier = m; }

    int GetIFF() const { return iff; }
    void SetIFF(int n) { iff = n; }
    int GetType() const { return type; }
    void SetType(int n) { type = n; }
    int GetIntel() const { return intel; }
    void SetIntel(int n) { intel = n; }
    int GetObjCode() const { return obj_code; }
    void SetObjCode(int n) { obj_code = n; }
    int GetSquadron() const { return squadron; }
    void SetSquadron(int n) { squadron = n; }

    int* GetLoadout() { return load; }
    void SetLoadout(int* n);
    int* GetSlots() { return slots; }
    void SetSlots(int* n);

    bool GetAlert() const { return alert; }
    void SetAlert(bool a) { alert = a; }

    bool GetInFlight() const { return in_flight; }
    void SetInFlight(bool f) { in_flight = f; }

  private:
    std::string name;
    int iff;
    int type;
    int intel;
    int obj_code;
    int squadron;

    std::string commander;
    std::string objective;
    std::string carrier;

    int load[16];
    int slots[4];
    bool alert;
    bool in_flight;

    BYTE data[SIZE];
};

class NetShipLaunch : public NetData
{
  public:
    NetShipLaunch()
      : objid(0),
        squadron(0),
        slot(0) {}

    enum
    {
      TYPE = NET_SHIP_LAUNCH,
      SIZE = 16
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    virtual int GetSquadron() const { return squadron; }
    virtual int GetSlot() const { return slot; }

    void SetObjID(DWORD o) override { objid = o; }
    virtual void SetSquadron(int s) { squadron = s; }
    virtual void SetSlot(int s) { slot = s; }

  private:
    DWORD objid;
    int squadron;
    int slot;

    BYTE data[SIZE];
};

class Instruction;

class NetNavData : public NetData
{
  public:
    NetNavData();
    ~NetNavData() override;

    enum
    {
      TYPE     = NET_NAV_DATA,
      SIZE     = 144,
      NAME_LEN = 32
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    bool IsAdd() const { return create; }
    bool IsEdit() const { return !create; }
    std::string GetElem() const { return elem; }
    int GetIndex() const { return index; }
    Instruction* GetNavPoint() const { return navpoint; }

    void SetObjID(DWORD o) override { objid = o; }
    void SetAdd(bool b) { create = b; }
    void SetElem(std::string_view e) { elem = e; }
    void SetIndex(int n) { index = n; }
    void SetNavPoint(Instruction* n);

  private:
    DWORD objid;
    bool create;
    std::string elem;
    int index;
    Instruction* navpoint;

    BYTE data[SIZE];
};

class NetNavDelete : public NetData
{
  public:
    NetNavDelete()
      : objid(0),
        index(0) {}

    enum
    {
      TYPE = NET_NAV_DELETE,
      SIZE = 40
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    std::string GetElem() const { return elem; }
    int GetIndex() const { return index; }

    void SetObjID(DWORD o) override { objid = o; }
    void SetElem(std::string_view e) { elem = e; }
    void SetIndex(int n) { index = n; }

  private:
    DWORD objid;
    std::string elem;
    int index;

    BYTE data[SIZE];
};

class NetSelfDestruct : public NetData
{
  public:
    NetSelfDestruct()
      : objid(0),
        damage(0) {}

    enum
    {
      TYPE = NET_SELF_DESTRUCT,
      SIZE = 8
    };

    BYTE* Pack() override;
    bool Unpack(const BYTE* data) override;
    int Type() const override { return TYPE; }
    int Length() const override { return SIZE; }

    DWORD GetObjID() const override { return objid; }
    void SetObjID(DWORD o) override { objid = o; }

    float GetDamage() const { return damage; }
    void SetDamage(float d) { damage = d; }

  private:
    DWORD objid;
    float damage;

    BYTE data[SIZE];
};

#endif NetData_h
