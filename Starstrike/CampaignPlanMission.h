#pragma once

#include "CampaignPlan.h"

class CampaignMissionRequest;
class CombatGroup;
class CombatUnit;
class CombatZone;

class CampaignPlanMission : public CampaignPlan
{
public:
  CampaignPlanMission(Campaign *c) : CampaignPlan(c), start(0), slot(0) {}
  virtual ~CampaignPlanMission() {}

  // operations:
  virtual void ExecFrame();

protected:
  virtual void SelectStartTime();
  virtual CampaignMissionRequest *PlanCampaignMission();
  virtual CampaignMissionRequest *PlanStrategicMission();
  virtual CampaignMissionRequest *PlanRandomStarshipMission();
  virtual CampaignMissionRequest *PlanRandomFighterMission();
  virtual CampaignMissionRequest *PlanStarshipMission();
  virtual CampaignMissionRequest *PlanFighterMission();

  CombatGroup *player_group;
  int start;
  int slot;
};
