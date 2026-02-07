#pragma once

#include "CampaignPlan.h"
#include "List.h"

class CombatGroup;
class CombatUnit;
class CombatZone;

class CampaignPlanAssignment : public CampaignPlan
{
  public:
    CampaignPlanAssignment(Campaign* c)
      : CampaignPlan(c) {}

    ~CampaignPlanAssignment() override {}

    // operations:
    virtual void ExecFrame();

  protected:
    virtual void ProcessCombatant(Combatant* c);
    virtual void ProcessZone(Combatant* c, CombatZone* zone);
    virtual void BuildZoneList(CombatGroup* g, CombatZone* zone, List<CombatGroup>& list);
    virtual void BuildAssetList(const int* pref, List<CombatGroup>& avail, List<CombatGroup>& assets);
};
