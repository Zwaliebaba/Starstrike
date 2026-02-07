#pragma once

class Campaign;
class Combatant;
class CombatGroup;
class CombatUnit;

class CampaignPlan
{
public:
  CampaignPlan(Campaign *c) : m_campaign(c), m_execTime(-1e6) {}
  virtual ~CampaignPlan() = default;

  int operator==(const CampaignPlan &p) const { return this == &p; }

  // operations:
  virtual void ExecFrame() {}
  virtual void SetLockout(int seconds) {}

protected:
  Campaign *m_campaign = {};
  double m_execTime;
};
