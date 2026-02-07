#pragma once

#include "CampaignPlan.h"

class CombatAction;
class CombatAssignment;
class CombatEvent;
class CombatGroup;
class CombatUnit;
class CombatZone;

class CampaignPlanEvent : public CampaignPlan
{
public:
  CampaignPlanEvent(Campaign *c);
  virtual ~CampaignPlanEvent();

  // operations:
  virtual void ExecFrame();
  virtual void SetLockout(int seconds);

  virtual bool ExecScriptedEvents();
  virtual bool ExecStatisticalEvents();

protected:
  virtual void ProsecuteKills(CombatAction *action);

  virtual CombatAssignment *
  ChooseAssignment(CombatGroup *c);
  virtual bool CreateEvent(CombatAssignment *a);

  virtual CombatEvent *CreateEventDefend(CombatAssignment *a);
  virtual CombatEvent *CreateEventFighterAssault(CombatAssignment *a);
  virtual CombatEvent *CreateEventFighterStrike(CombatAssignment *a);
  virtual CombatEvent *CreateEventFighterSweep(CombatAssignment *a);
  virtual CombatEvent *CreateEventStarship(CombatAssignment *a);

  virtual bool IsFriendlyAssignment(CombatAssignment *a);
  virtual bool Success(CombatAssignment *a);
  virtual std::string GetTeamName(CombatGroup *g);

  // attributes:
  int event_time;
};
