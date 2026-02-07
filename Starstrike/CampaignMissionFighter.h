#pragma once

#include "Geometry.h"

class Campaign;
class CampaignMissionRequest;
class CombatGroup;
class CombatUnit;
class CombatZone;
class Mission;
class MissionElement;
class MissionInfo;
class MissionTemplate;

class CampaignMissionFighter
{
  public:
    CampaignMissionFighter(Campaign* c);
    virtual ~CampaignMissionFighter();

    virtual void CreateMission(CampaignMissionRequest* request);

  protected:
    virtual Mission* GenerateMission(int id);
    virtual void SelectType();
    virtual void SelectRegion();
    virtual void GenerateStandardElements();
    virtual void GenerateMissionElements();
    virtual void CreateElements(CombatGroup* g);
    virtual void CreateSquadron(CombatGroup* g);
    virtual void CreatePlayer(CombatGroup* g);

    virtual void CreatePatrols();
    virtual void CreateWards();
    virtual void CreateWardFreight();
    virtual void CreateWardShuttle();
    virtual void CreateWardStrike();

    virtual void CreateEscorts();

    virtual void CreateTargets();
    virtual void CreateTargetsPatrol();
    virtual void CreateTargetsSweep();
    virtual void CreateTargetsIntercept();
    virtual void CreateTargetsFreightEscort();
    virtual void CreateTargetsShuttleEscort();
    virtual void CreateTargetsStrikeEscort();
    virtual void CreateTargetsStrike();
    virtual void CreateTargetsAssault();
    virtual int CreateRandomTarget(std::string_view rgn, Point base_loc);

    virtual bool IsGroundObjective(CombatGroup* obj);

    virtual void PlanetaryInsertion(MissionElement* elem);
    virtual void OrbitalInsertion(MissionElement* elem);

    virtual MissionElement* CreateSingleElement(CombatGroup* g, CombatUnit* u);
    virtual MissionElement* CreateFighterPackage(CombatGroup* squadron, int count, int role);

    virtual CombatGroup* FindSquadron(int iff, int type);
    virtual CombatUnit* FindCarrier(CombatGroup* g);

    virtual void DefineMissionObjectives();
    virtual MissionInfo* DescribeMission();
    virtual void Exit();

    Campaign* m_campaign = {};
    CampaignMissionRequest* request = {};
    MissionInfo* mission_info = {};

    CombatGroup* m_squadron = {};
    CombatGroup* strike_group = {};
    CombatGroup* strike_target = {};
    Mission* mission = {};
    MissionElement* player_elem = {};
    MissionElement* m_carrierElem = {};
    MissionElement* ward = {};
    MissionElement* prime_target = {};
    MissionElement* escort = {};
    std::string air_region;
    std::string orb_region;
    bool airborne;
    bool airbase;
    int ownside;
    int enemy;
    int mission_type;
};
