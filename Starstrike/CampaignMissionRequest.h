#pragma once

#include "Geometry.h"

class Campaign;
class CombatGroup;
class CombatUnit;
class CombatZone;
class Mission;
class MissionElement;
class MissionInfo;

class CampaignMissionRequest
{
  public:
    CampaignMissionRequest(Campaign* c, int type, int start, CombatGroup* primary, CombatGroup* tgt = nullptr);

    Campaign* GetCampaign() { return campaign; }
    int Type() { return type; }
    int OpposingType() { return opp_type; }
    int StartTime() { return start; }
    CombatGroup* GetPrimaryGroup() { return primary_group; }
    CombatGroup* GetSecondaryGroup() { return secondary_group; }
    CombatGroup* GetObjective() { return objective; }

    bool IsLocSpecified() { return use_loc; }
    std::string RegionName() { return region; }
    Point Location() { return location; }
    std::string Script() { return script; }

    void SetType(int t) { type = t; }
    void SetOpposingType(int t) { opp_type = t; }
    void SetStartTime(int s) { start = s; }
    void SetPrimaryGroup(CombatGroup* g) { primary_group = g; }
    void SetSecondaryGroup(CombatGroup* g) { secondary_group = g; }
    void SetObjective(CombatGroup* g) { objective = g; }

    void SetRegionName(std::string_view rgn)
    {
      region = rgn;
      use_loc = true;
    }

    void SetLocation(const Point& loc)
    {
      location = loc;
      use_loc = true;
    }

    void SetScript(std::string_view s) { script = s; }

  private:
    Campaign* campaign;

    int type;                     // type of mission
    int opp_type;                 // opposing mission type
    int start;                    // start time
    CombatGroup* primary_group;   // player's group
    CombatGroup* secondary_group; // optional support group
    CombatGroup* objective;       // target or ward

    bool use_loc; // use the specified location
    std::string region;
    Point location;
    std::string script;
};
