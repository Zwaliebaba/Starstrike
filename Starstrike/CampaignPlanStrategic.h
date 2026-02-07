#pragma once

#include "CampaignPlan.h"
#include "List.h"

class CampaignPlanStrategic : public CampaignPlan
{
public:
  CampaignPlanStrategic(Campaign *c) : CampaignPlan(c) {}
  virtual ~CampaignPlanStrategic() {}

  // operations:
  virtual void ExecFrame();

protected:
  void PlaceGroup(CombatGroup *g);

  void ScoreCombatant(Combatant *c);

  void ScoreDefensible(Combatant *c);
  void ScoreDefend(Combatant *c, CombatGroup *g);

  void ScoreTargets(Combatant *c, Combatant *t);
  void ScoreTarget(Combatant *c, CombatGroup *g);

  void ScoreNeeds(Combatant *c);

  // zone alocation:
  void BuildGroupList(CombatGroup *g, List<CombatGroup> &groups);
  void AssignZones(Combatant *c);
  void ResolveZoneMovement(CombatGroup *g);
};
