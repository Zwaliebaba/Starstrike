#ifndef CombatAction_h
#define CombatAction_h

#include "Geometry.h"
#include "List.h"


class Combatant;
class CombatAction;
class CombatActionReq;

class CombatAction
{
  public:
    
    enum TYPE
    {
      NO_ACTION,
      STRATEGIC_DIRECTIVE,
      ZONE_ASSIGNMENT,
      SYSTEM_ASSIGNMENT,
      MISSION_TEMPLATE,
      COMBAT_EVENT,
      INTEL_EVENT,
      CAMPAIGN_SITUATION,
      CAMPAIGN_ORDERS
    };

    enum STATUS
    {
      PENDING,
      ACTIVE,
      SKIPPED,
      FAILED,
      COMPLETE
    };

    CombatAction(int id, int type, int subtype, int team);
    ~CombatAction();

    int operator ==(const CombatAction& a) const { return id == a.id; }

    bool IsAvailable() const;
    void FireAction();
    void FailAction();
    void AddRequirement(int action, int stat, bool _not = false);
    void AddRequirement(Combatant* c1, Combatant* c2, int comp, int score);
    void AddRequirement(Combatant* c1, int group_type, int group_id, int comp, int score, int intel = 0);
    static int TypeFromName(std::string_view n);
    static int StatusFromName(std::string_view n);

    // accessors/mutators:
    int Identity() const { return id; }
    int Type() const { return type; }
    int Subtype() const { return subtype; }
    int OpposingType() const { return opp_type; }
    int GetIFF() const { return team; }
    int Status() const { return status; }
    int Source() const { return source; }
    Point Location() const { return loc; }
    std::string System() const { return system; }
    std::string Region() const { return region; }
    std::string Filename() const { return text_file; }
    std::string ImageFile() const { return image_file; }
    std::string SceneFile() const { return scene_file; }
    int Count() const { return count; }
    int ExecTime() const { return time; }
    int StartBefore() const { return start_before; }
    int StartAfter() const { return start_after; }
    int MinRank() const { return min_rank; }
    int MaxRank() const { return max_rank; }
    int Delay() const { return delay; }
    int Probability() const { return probability; }
    int AssetType() const { return asset_type; }
    int AssetId() const { return asset_id; }
    std::vector<std::string>& AssetKills() { return asset_kills; }
    int TargetType() const { return target_type; }
    int TargetId() const { return target_id; }
    int TargetIFF() const { return target_iff; }
    std::vector<std::string>& TargetKills() { return target_kills; }
    std::string GetText() const { return text; }

    void SetType(int t) { type = static_cast<char>(t); }
    void SetSubtype(int s) { subtype = static_cast<char>(s); }
    void SetOpposingType(int t) { opp_type = static_cast<char>(t); }
    void SetIFF(int t) { team = static_cast<char>(t); }
    void SetStatus(int s) { status = static_cast<char>(s); }
    void SetSource(int s) { source = s; }
    void SetLocation(const Point& p) { loc = p; }
    void SetSystem(const std::string& sys) { system = sys; }
    void SetRegion(const std::string& rgn) { region = rgn; }
    void SetFilename(const std::string& f) { text_file = f; }
    void SetImageFile(const std::string& f) { image_file = f; }
    void SetSceneFile(const std::string& f) { scene_file = f; }
    void SetCount(int n) { count = static_cast<char>(n); }
    void SetExecTime(int t) { time = t; }
    void SetStartBefore(int s) { start_before = s; }
    void SetStartAfter(int s) { start_after = s; }
    void SetMinRank(int n) { min_rank = static_cast<char>(n); }
    void SetMaxRank(int n) { max_rank = static_cast<char>(n); }
    void SetDelay(int d) { delay = d; }
    void SetProbability(int n) { probability = n; }
    void SetAssetType(int t) { asset_type = t; }
    void SetAssetId(int n) { asset_id = n; }
    void SetTargetType(int t) { target_type = t; }
    void SetTargetId(int n) { target_id = n; }
    void SetTargetIFF(int n) { target_iff = n; }
    void SetText(std::string t) { text = t; }

  private:
    int id;
    char type;
    char subtype;
    char opp_type;
    char team;
    char status;
    char min_rank;
    char max_rank;
    int source;
    Point loc;
    std::string system;
    std::string region;
    std::string text_file;
    std::string image_file;
    std::string scene_file;
    char count;
    int start_before;
    int start_after;
    int delay;
    int probability;
    int rval;
    int time;

    std::string text;
    int asset_type;
    int asset_id;
    std::vector<std::string> asset_kills;
    int target_type;
    int target_id;
    int target_iff;
    std::vector<std::string> target_kills;

    List<CombatActionReq> requirements;
};

class CombatActionReq
{
  public:
    
    enum COMPARISON_OPERATOR
    {
      LT,
      LE,
      GT,
      GE,
      EQ,    // absolute score comparison
      RLT,
      RLE,
      RGT,
      RGE,
      REQ    // delta score comparison
    };

    CombatActionReq(int a, int s, bool n = false)
      : action(a),
        stat(s),
        m_not(n),
        c1(nullptr),
        c2(nullptr),
        comp(0),
        score(0),
        intel(0) {}

    CombatActionReq(Combatant* a1, Combatant* a2, int comparison, int value)
      : action(0),
        stat(0),
        m_not(false),
        c1(a1),
        c2(a2),
        comp(comparison),
        score(value),
        intel(0),
        group_type(0),
        group_id(0) {}

    CombatActionReq(Combatant* a1, int gtype, int gid, int comparison, int value, int intel_level = 0)
      : action(0),
        stat(0),
        m_not(false),
        c1(a1),
        c2(nullptr),
        comp(comparison),
        score(value),
        intel(intel_level),
        group_type(gtype),
        group_id(gid) {}

    static int CompFromName(std::string_view sym);

    int action;
    int stat;
    bool m_not;

    Combatant* c1;
    Combatant* c2;
    int comp;
    int score;
    int intel;
    int group_type;
    int group_id;
};

#endif CombatAction_h
