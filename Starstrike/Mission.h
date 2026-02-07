#ifndef Mission_h
#define Mission_h

#include "Geometry.h"
#include "List.h"
#include "RLoc.h"
#include "Skin.h"
#include "Term.h"

class MissionElement;
class MissionLoad;
class MissionEvent;
class MissionShip;

class CombatGroup;
class CombatUnit;

class Ship;
class System;
class Element;
class ShipDesign;
class WeaponDesign;
class StarSystem;
class Instruction;

class Mission
{
  public:
    
    enum TYPE
    {
      PATROL,
      SWEEP,
      INTERCEPT,
      AIR_PATROL,
      AIR_SWEEP,
      AIR_INTERCEPT,
      STRIKE,     // ground attack
      ASSAULT,    // starship attack
      DEFEND,
      ESCORT,
      ESCORT_FREIGHT,
      ESCORT_SHUTTLE,
      ESCORT_STRIKE,
      INTEL,
      SCOUT,
      RECON,
      BLOCKADE,
      FLEET,
      BOMBARDMENT,
      FLIGHT_OPS,
      TRANSPORT,
      CARGO,
      TRAINING,
      OTHER
    };

    Mission(int id, std::string_view filename = {}, std::string_view _path = {});
    virtual ~Mission();

    int operator ==(const Mission& m) const { return id == m.id; }

    virtual void Validate();
    virtual bool Load(std::string_view filename = {}, std::string_view path = {});
    virtual bool Save();
    virtual bool ParseMission(std::string_view buffer);
    virtual void SetPlayer(MissionElement* player_element);
    virtual MissionElement* GetPlayer();

    // accessors/mutators:
    int Identity() const { return id; }
    std::string FileName() const { return filename; }
    std::string Name() const { return m_name; }
    std::string Description() const { return desc; }
    std::string Situation() const { return sitrep; }
    std::string Objective() const { return objective; }
    std::string Subtitles() const;
    int Start() const { return start; }
    double Stardate() const { return stardate; }
    int Type() const { return type; }
    std::string TypeName() const { return RoleName(type); }
    int Team() const { return team; }
    bool IsOK() const { return ok; }
    bool IsActive() const { return active; }
    bool IsComplete() const { return complete; }

    StarSystem* GetStarSystem() const { return star_system; }
    List<StarSystem>& GetSystemList() { return system_list; }
    std::string GetRegion() const { return m_region; }

    List<MissionElement>& GetElements() { return elements; }
    virtual MissionElement* FindElement(std::string_view name);
    virtual void AddElement(MissionElement* elem);

    List<MissionEvent>& GetEvents() { return events; }
    MissionEvent* FindEvent(int event_type) const;
    virtual void AddEvent(MissionEvent* event);

    MissionElement* GetTarget() const { return target; }
    MissionElement* GetWard() const { return ward; }

    void SetName(std::string_view n) { m_name = n; }
    void SetDescription(std::string_view d) { desc = d; }
    void SetSituation(std::string_view sit) { sitrep = sit; }
    void SetObjective(std::string_view obj) { objective = obj; }
    void SetStart(int s) { start = s; }
    void SetType(int t) { type = t; }
    void SetTeam(int iff) { team = iff; }
    void SetStarSystem(StarSystem* s);
    void SetRegion(std::string_view rgn) { m_region = rgn; }
    void SetOK(bool a) { ok = a; }
    void SetActive(bool a) { active = a; }
    void SetComplete(bool c) { complete = c; }
    void SetTarget(MissionElement* t) { target = t; }
    void SetWard(MissionElement* w) { ward = w; }

    void ClearSystemList();

    void IncreaseElemPriority(int index);
    void DecreaseElemPriority(int index);
    void IncreaseEventPriority(int index);
    void DecreaseEventPriority(int index);

    static std::string RoleName(int role);
    static int TypeFromName(std::string_view n);

    std::string ErrorMessage() const { return errmsg; }
    void AddError(const std::string& err);

    std::string Serialize(std::string_view player_elem = {}, int player_index = 0);

  protected:
    MissionElement* ParseElement(TermStruct* val);
    MissionEvent* ParseEvent(TermStruct* val);
    MissionShip* ParseShip(TermStruct* val, MissionElement* element);
    Instruction* ParseInstruction(TermStruct* val, MissionElement* element);
    void ParseLoadout(TermStruct* val, MissionElement* element);
    RLoc* ParseRLoc(TermStruct* val);

    int id;
    std::string filename;
    std::string path;
    std::string m_region;
    std::string m_name;
    std::string desc;
    int type;
    int team;
    int start;
    double stardate;
    bool ok;
    bool active;
    bool complete;
    bool degrees;
    std::string objective;
    std::string sitrep;
    std::string errmsg;
    std::string subtitles;
    StarSystem* star_system;
    List<StarSystem> system_list;

    List<MissionElement> elements;
    List<MissionEvent> events;

    MissionElement* target;
    MissionElement* ward;
    MissionElement* current;
};

class MissionElement
{
  friend class Mission;

  public:
    
    MissionElement();
    ~MissionElement();

    int operator ==(const MissionElement& r) const { return m_id == r.m_id; }

    int Identity() const { return m_id; }
    std::string Name() const { return name; }
    std::string Abbreviation() const;
    std::string Carrier() const { return carrier; }
    std::string Commander() const { return commander; }
    std::string Squadron() const { return squadron; }
    std::string Path() const { return path; }
    int ElementID() const { return elem_id; }
    const ShipDesign* GetDesign() const { return m_design; }
    const Skin* GetSkin() const { return skin; }
    int Count() const { return count; }
    int MaintCount() const { return maint_count; }
    int DeadCount() const { return dead_count; }
    int GetIFF() const { return IFF_code; }
    int IntelLevel() const { return intel; }
    int MissionRole() const { return mission_role; }
    int Player() const { return player; }
    std::string RoleName() const;
    Color MarkerColor() const;
    bool IsStarship() const;
    bool IsDropship() const;
    bool IsStatic() const;
    bool IsGroundUnit() const;
    bool IsSquadron() const;
    bool IsCarrier() const;
    bool IsAlert() const { return alert; }
    bool IsPlayable() const { return playable; }
    bool IsRogue() const { return rogue; }
    bool IsInvulnerable() const { return invulnerable; }
    int RespawnCount() const { return respawns; }
    int HoldTime() const { return hold_time; }
    int CommandAI() const { return command_ai; }
    int ZoneLock() const { return zone_lock; }

    std::string Region() const { return rgn_name; }
    Point Location() const;
    RLoc& GetRLoc() { return rloc; }
    double Heading() const { return heading; }

    std::string GetShipName(size_t n) const;
    std::string GetRegistry(size_t n) const;

    List<Instruction>& Objectives() { return objectives; }
    std::vector<std::string>& Instructions() { return instructions; }
    List<Instruction>& NavList() { return navlist; }
    List<MissionLoad>& Loadouts() { return loadouts; }
    List<MissionShip>& Ships() { return ships; }

    void SetName(std::string_view n) { name = n; }
    void SetCarrier(std::string_view c) { carrier = c; }
    void SetCommander(std::string_view c) { commander = c; }
    void SetSquadron(std::string_view s) { squadron = s; }
    void SetPath(std::string_view p) { path = p; }
    void SetElementID(int _id) { elem_id = _id; }
    void SetDesign(const ShipDesign* d) { m_design = d; }
    void SetSkin(const Skin* s) { skin = s; }
    void SetCount(int n) { count = n; }
    void SetMaintCount(int n) { maint_count = n; }
    void SetDeadCount(int n) { dead_count = n; }
    void SetIFF(int iff) { IFF_code = iff; }
    void SetIntelLevel(int i) { intel = i; }
    void SetMissionRole(int r) { mission_role = r; }
    void SetPlayer(int p) { player = p; }
    void SetPlayable(bool p) { playable = p; }
    void SetRogue(bool r) { rogue = r; }
    void SetInvulnerable(bool n) { invulnerable = n; }
    void SetAlert(bool a) { alert = a; }
    void SetCommandAI(int a) { command_ai = a; }
    void SetRegion(std::string_view rgn) { rgn_name = rgn; }
    void SetLocation(const Point& p);
    void SetRLoc(const RLoc& r);
    void SetHeading(double h) { heading = h; }
    void SetRespawnCount(int r) { respawns = r; }
    void SetHoldTime(int t) { hold_time = t; }
    void SetZoneLock(int z) { zone_lock = z; }

    void AddNavPoint(Instruction* pt, Instruction* afterPoint = nullptr);
    void DelNavPoint(Instruction* pt);
    void ClearFlightPlan();
    int GetNavIndex(const Instruction* n);

    void AddObjective(const Instruction* obj) { objectives.append(obj); }
    void AddInstruction(std::string_view i) { instructions.emplace_back(i); }

    CombatGroup* GetCombatGroup() const { return combat_group; }
    void SetCombatGroup(CombatGroup* g) { combat_group = g; }
    CombatUnit* GetCombatUnit() const { return combat_unit; }
    void SetCombatUnit(CombatUnit* u) { combat_unit = u; }

  protected:
    int m_id;
    std::string name;
    std::string carrier;
    std::string commander;
    std::string squadron;
    std::string path;
    int elem_id;
    const ShipDesign* m_design;
    const Skin* skin;
    int count;
    int maint_count;
    int dead_count;
    int IFF_code;
    int mission_role;
    int intel;
    int respawns;
    int hold_time;
    int zone_lock;
    int player;
    int command_ai;
    bool alert;
    bool playable;
    bool rogue;
    bool invulnerable;

    std::string rgn_name;
    RLoc rloc;
    double heading;

    CombatGroup* combat_group;
    CombatUnit* combat_unit;

    List<Instruction> objectives;
    std::vector<std::string> instructions;
    List<Instruction> navlist;
    List<MissionLoad> loadouts;
    List<MissionShip> ships;
};

class MissionLoad
{
  friend class Mission;

  public:
    
    MissionLoad(int ship = -1, std::string_view name = {});
    ~MissionLoad() = default;

    int GetShip() const { return m_ship; }
    void SetShip(int s) { m_ship = s; }

    std::string GetName() const;
    void SetName(std::string_view n) { m_name = n; }

    int* GetStations() { return m_load; }
    int GetStation(int index);
    void SetStation(int index, int selection);

  protected:
    int m_ship;
    std::string m_name;
    int m_load[16];
};

class MissionShip
{
  friend class Mission;

  public:
    
    MissionShip();
    ~MissionShip() = default;

    [[nodiscard]] std::string Name() const { return m_name; }
    [[nodiscard]] std::string RegNum() const { return regnum; }
    [[nodiscard]] std::string Region() const { return region; }
    [[nodiscard]] const Skin* GetSkin() const { return skin; }
    [[nodiscard]] const Point& Location() const { return loc; }
    [[nodiscard]] const Point& Velocity() const { return velocity; }
    [[nodiscard]] int Respawns() const { return respawns; }
    [[nodiscard]] double Heading() const { return heading; }
    [[nodiscard]] double Integrity() const { return integrity; }
    [[nodiscard]] int Decoys() const { return decoys; }
    [[nodiscard]] int Probes() const { return probes; }
    [[nodiscard]] const int* Ammo() const { return ammo; }
    [[nodiscard]] const int* Fuel() const { return fuel; }

    void SetName(std::string_view n) { m_name = n; }
    void SetRegNum(std::string_view n) { regnum = n; }
    void SetRegion(std::string_view n) { region = n; }
    void SetSkin(const Skin* s) { skin = s; }
    void SetLocation(const Point& p) { loc = p; }
    void SetVelocity(const Point& p) { velocity = p; }
    void SetRespawns(int r) { respawns = r; }
    void SetHeading(double h) { heading = h; }
    void SetIntegrity(double n) { integrity = n; }
    void SetDecoys(int d) { decoys = d; }
    void SetProbes(int p) { probes = p; }
    void SetAmmo(const int* a);
    void SetFuel(const int* f);

  protected:
    std::string m_name;
    std::string regnum;
    std::string region;
    const Skin* skin;
    Point loc;
    Point velocity;
    int respawns;
    double heading;
    double integrity;
    int decoys;
    int probes;
    int ammo[16];
    int fuel[4];
};

#endif Mission_h
