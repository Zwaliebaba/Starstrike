#pragma once

#include "Bitmap.h"
#include "List.h"
#include "Term.h"

class CampaignPlan;
class Combatant;
class CombatAction;
class CombatEvent;
class CombatGroup;
class CombatUnit;
class CombatZone;
class DataLoader;
class Mission;
class MissionTemplate;
class StarSystem;

class MissionInfo
{
  public:
    
    MissionInfo();
    ~MissionInfo();

    int operator ==(const MissionInfo& m) const { return m_id == m.m_id; }
    int operator <(const MissionInfo& m) const { return m_id < m.m_id; }
    int operator <=(const MissionInfo& m) const { return m_id <= m.m_id; }

    bool IsAvailable();

    size_t m_id;
    std::string m_name;
    std::string player_info;
    std::string description;
    std::string system;
    std::string region;
    std::string script;
    int start;
    int type;

    int min_rank;
    int max_rank;
    int action_id;
    int action_status;
    int exec_once;
    int start_before;
    int start_after;

    Mission* mission;
};

class TemplateList
{
  public:
    
    TemplateList();
    ~TemplateList();

    int mission_type;
    int group_type;
    int index;
    List<MissionInfo> missions;
};

class Campaign
{
  public:
    
    enum CONSTANTS
    {
      TRAINING_CAMPAIGN = 1,
      DYNAMIC_CAMPAIGN,
      SINGLE_MISSIONS = 1000,
      MULTIPLAYER_MISSIONS,
      NUM_IMAGES = 6
    };

    enum STATUS
    {
      CAMPAIGN_INIT,
      CAMPAIGN_ACTIVE,
      CAMPAIGN_SUCCESS,
      CAMPAIGN_FAILED
    };

    Campaign(int id, std::string_view name = {});
    Campaign(int id, std::string_view name, std::string_view path);
    virtual ~Campaign();

    int operator ==(const Campaign& s) const { return m_name == s.m_name; }
    int operator <(const Campaign& s) const { return campaign_id < s.campaign_id; }

    // operations:
    virtual void Load();
    virtual void Prep();
    virtual void Start();
    virtual void ExecFrame();
    virtual void Unload();

    virtual void Clear();
    virtual void CommitExpiredActions();
    virtual void LockoutEvents(int seconds);
    virtual void CheckPlayerGroup();
    void CreatePlanners();

    // accessors:
    std::string Name() const { return m_name; }
    std::string Description() const { return description; }
    std::string Path() const { return m_path; }

    std::string Situation() const { return situation; }
    std::string Orders() const { return orders; }

    void SetSituation(std::string_view s) { situation = s; }
    void SetOrders(std::string_view o) { orders = o; }

    int GetPlayerTeamScore();
    List<MissionInfo>& GetMissionList() { return missions; }
    List<Combatant>& GetCombatants() { return combatants; }
    List<CombatZone>& GetZones() { return zones; }
    List<StarSystem>& GetSystemList() { return systems; }
    List<CombatAction>& GetActions() { return actions; }
    List<CombatEvent>& GetEvents() { return events; }
    CombatEvent* GetLastEvent();

    CombatAction* FindAction(int id);

    int CountNewEvents() const;

    int GetPlayerIFF();
    CombatGroup* GetPlayerGroup() { return player_group; }
    void SetPlayerGroup(CombatGroup* pg);
    CombatUnit* GetPlayerUnit() { return player_unit; }
    void SetPlayerUnit(CombatUnit* pu);

    Combatant* GetCombatant(std::string_view name);
    CombatGroup* FindGroup(int iff, int type, int id);
    CombatGroup* FindGroup(int iff, int type, CombatGroup* near_group = nullptr);
    CombatGroup* FindStrikeTarget(int iff, CombatGroup* strike_group);

    StarSystem* GetSystem(std::string_view sys);
    CombatZone* GetZone(std::string_view rgn);
    MissionInfo* CreateNewMission();
    void DeleteMission(int id);
    Mission* GetMission();
    Mission* GetMission(int id);
    Mission* GetMissionByFile(std::string_view filename);
    MissionInfo* GetMissionInfo(size_t id);
    MissionInfo* FindMissionTemplate(int msn_type, CombatGroup* player_group);
    void ReloadMission(int id);
    void LoadNetMission(int id, std::string_view net_mission);
    void StartMission();
    void RollbackMission();

    void SetCampaignId(int id);
    int GetCampaignId() const { return campaign_id; }
    void SetMissionId(int id);
    int GetMissionId() const { return mission_id; }
    Bitmap* GetImage(int n) { return &m_image[n]; }
    double GetTime() const { return time; }
    double GetStartTime() const { return startTime; }
    void SetStartTime(double t) { startTime = t; }
    double GetLoadTime() const { return loadTime; }
    void SetLoadTime(double t) { loadTime = t; }
    double GetUpdateTime() const { return updateTime; }
    void SetUpdateTime(double t) { updateTime = t; }

    bool InCutscene() const;
    bool IsDynamic() const;
    bool IsTraining() const;
    bool IsScripted() const;
    bool IsSequential() const;
    bool IsSaveGame() const { return loaded_from_savegame; }
    void SetSaveGame(bool s) { loaded_from_savegame = s; }

    bool IsActive() const { return status == CAMPAIGN_ACTIVE; }
    bool IsComplete() const { return status == CAMPAIGN_SUCCESS; }
    bool IsFailed() const { return status == CAMPAIGN_FAILED; }
    void SetStatus(int s);
    int GetStatus() const { return status; }

    int GetAllCombatUnits(int iff, List<CombatUnit>& units);

    static void Initialize();
    static void Close();
    static Campaign* GetCampaign();
    static List<Campaign>& GetAllCampaigns();
    static int GetLastCampaignId();
    static Campaign* SelectCampaign(std::string_view name);

    static double Stardate();

  protected:
    void LoadCampaign(DataLoader* loader, bool full = false);
    void LoadTemplateList(DataLoader* loader);
    void LoadMissionList(DataLoader* loader);
    void ParseGroup(TermStruct* val, CombatGroup* force, CombatGroup* clone, std::string_view filename);
    void ParseAction(TermStruct* val, std::string_view filename);
    CombatGroup* CloneOver(CombatGroup* force, CombatGroup* clone, CombatGroup* group);
    void SelectDefaultPlayerGroup(CombatGroup* g, int type);
    TemplateList* GetTemplateList(int msn_type, int grp_type);

    // attributes:
    int campaign_id;
    int status;
    std::string m_filename;
    std::string m_path;
    std::string m_name;
    std::string description;
    std::string situation;
    std::string orders;
    Bitmap m_image[NUM_IMAGES];

    bool scripted;
    bool sequential;
    bool loaded_from_savegame;

    List<Combatant> combatants;
    List<StarSystem> systems;
    List<CombatZone> zones;
    List<CampaignPlan> planners;
    List<MissionInfo> missions;
    List<TemplateList> templates;
    List<CombatAction> actions;
    List<CombatEvent> events;
    CombatGroup* player_group;
    CombatUnit* player_unit;

    int mission_id;
    Mission* mission;
    Mission* net_mission;

    double time;
    double loadTime;
    double startTime;
    double updateTime;
    int lockout;
};
