#pragma once

class Campaign;
class CampaignPlan;
class Combatant;
class CombatGroup;
class CombatZone;
class DataLoader;
class Mission;
class Player;
class StarSystem;

class CampaignSaveGame
{
  public:
    
    CampaignSaveGame(Campaign* c = nullptr);
    virtual ~CampaignSaveGame();

    virtual Campaign* GetCampaign() { return campaign; }

    virtual void Load(std::string_view name);
    virtual void Save(std::string_view name);
    static void Delete(const std::string& name);
    static void RemovePlayer(Player* p);

    virtual void LoadAuto();
    virtual void SaveAuto();

    static std::string GetResumeFile();
    static int GetSaveGameList(std::vector<std::string>& save_list);

  private:
    static std::string GetSaveDirectory();
    static std::string GetSaveDirectory(Player* p);
    static void CreateSaveDirectory();

    Campaign* campaign;
};
