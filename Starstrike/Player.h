#pragma once

#include "List.h"

class Player;
class Bitmap;
class ShipStats;
class AwardInfo;
class Sound;

class Player
{
  public:
    
    Player(std::string_view name);
    virtual ~Player();

    int operator ==(const Player& u) const { return name == u.name; }

    int Identity() const { return uid; }
    std::string Name() const { return name; }
    std::string Password() const { return pass; }
    std::string Squadron() const { return squadron; }
    std::string Signature() const { return signature; }
    std::string ChatMacro(int n) const;
    int CreateDate() const { return create_date; }
    int Rank() const;
    int Medal(int n) const;
    int Points() const { return points; }
    int Medals() const { return medals; }
    int FlightTime() const { return flight_time; }
    int Missions() const { return missions; }
    int Kills() const { return kills; }
    int Losses() const { return losses; }
    int Campaigns() const { return campaigns; }
    int Trained() const { return trained; }

    int FlightModel() const { return flight_model; }
    int FlyingStart() const { return flying_start; }
    int LandingModel() const { return landing_model; }
    int AILevel() const { return ai_level; }
    int HUDMode() const { return hud_mode; }
    int HUDColor() const { return hud_color; }
    int FriendlyFire() const { return ff_level; }
    int GridMode() const { return grid; }
    int Gunsight() const { return gunsight; }

    bool ShowAward() const { return award != nullptr; }
    std::string AwardName() const;
    std::string AwardDesc() const;
    Bitmap* AwardImage() const;
    Sound* AwardSound() const;

    bool CanCommand(int ship_class);

    void SetName(std::string_view n);
    void SetPassword(std::string_view p);
    void SetSquadron(std::string_view s);
    void SetSignature(std::string_view s);
    void SetChatMacro(int n, std::string_view m);
    void SetCreateDate(int d);
    void SetRank(int r);
    void SetPoints(int p);
    void SetMedals(int m);
    void SetCampaigns(int n);
    void SetTrained(int n);
    void SetFlightTime(int t);
    void SetMissions(int m);
    void SetKills(int k);
    void SetLosses(int l);

    void AddFlightTime(int t);
    void AddPoints(int p);
    void AddMedal(int m);
    void AddMissions(int m);
    void AddKills(int k);
    void AddLosses(int l);

    bool HasTrained(int n) const;
    bool HasCompletedCampaign(int id) const;
    void SetCampaignComplete(int id);

    void SetFlightModel(int n);
    void SetFlyingStart(int n);
    void SetLandingModel(int n);
    void SetAILevel(int n);
    void SetHUDMode(int n);
    void SetHUDColor(int n);
    void SetFriendlyFire(int n);
    void SetGridMode(int n);
    void SetGunsight(int n);

    void ClearShowAward();

    std::string EncodeStats();
    void DecodeStats(std::string_view stats);

    int GetMissionPoints(ShipStats* stats, DWORD start_time);
    void ProcessStats(ShipStats* stats, DWORD start_time);
    bool EarnedAward(AwardInfo* a, ShipStats* s);

    static std::string RankName(int rank);
    static std::string RankAbrv(int rank);
    static int RankFromName(std::string_view name);
    static Bitmap* RankInsignia(int rank, int size);
    static std::string RankDescription(int rank);
    static std::string MedalName(int medal);
    static Bitmap* MedalInsignia(int medal, int size);
    static std::string MedalDescription(int medal);
    static int CommandRankRequired(int ship_class);

    static List<Player>& GetRoster();
    static Player* GetCurrentPlayer();
    static void SelectPlayer(Player* p);
    static Player* Create(std::string_view name);
    static void Destroy(Player* p);
    static Player* Find(std::string_view name);
    static void Initialize();
    static void Close();
    static void Load();
    static void Save();
    static bool ConfigExists();
    static void LoadAwardTables();

  protected:
    Player();

    void CreateUniqueID();

    int uid;
    std::string name;
    std::string pass;
    std::string squadron;
    std::string signature;
    std::string chat_macros[10];
    int mfd[4];

    // stats:
    int create_date;
    int points;
    int medals;        // bitmap of earned medals
    int flight_time;
    int missions;
    int kills;
    int losses;
    int campaigns;     // bitmap of completed campaigns
    int trained;       // id of highest training mission completed

    // gameplay options:
    int flight_model;
    int flying_start;
    int landing_model;
    int ai_level;
    int hud_mode;
    int hud_color;
    int ff_level;
    int grid;
    int gunsight;

    // transient:
    AwardInfo* award;
};
