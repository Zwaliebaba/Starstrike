#pragma once

#include "CampaignPlan.h"
#include "List.h"

class CampaignPlanMovement : public CampaignPlan
{
public:
  CampaignPlanMovement(Campaign *c) : CampaignPlan(c) {}
  virtual ~CampaignPlanMovement() {}

  // operations:
  virtual void ExecFrame();

protected:
  void MoveUnit(CombatUnit *u);

  List<CombatUnit> all_units;
};
