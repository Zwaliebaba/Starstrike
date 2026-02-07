#include "pch.h"
#include "CampaignPlanAssignment.h"
#include "Campaign.h"
#include "CombatAssignment.h"
#include "CombatGroup.h"
#include "CombatZone.h"
#include "Combatant.h"
#include "Mission.h"

void CampaignPlanAssignment::ExecFrame()
{
  if (m_campaign && m_campaign->IsActive())
  {
    // once every few minutes is plenty:
    if (Campaign::Stardate() - m_execTime < 300)
      return;

    ListIter<Combatant> iter = m_campaign->GetCombatants();
    while (++iter)
      ProcessCombatant(iter.value());

    m_execTime = Campaign::Stardate();
  }
}

void CampaignPlanAssignment::ProcessCombatant(Combatant* c)
{
  CombatGroup* force = c->GetForce();
  if (force)
  {
    force->CalcValue();
    force->ClearAssignments();
  }

  ListIter<CombatZone> zone = m_campaign->GetZones();
  while (++zone)
    ProcessZone(c, zone.value());
}

void CampaignPlanAssignment::BuildZoneList(CombatGroup* g, CombatZone* zone, List<CombatGroup>& groups)
{
  if (!g)
    return;

  if (g->GetAssignedZone() == zone)
    groups.append(g);

  ListIter<CombatGroup> iter = g->GetComponents();
  while (++iter)
    BuildZoneList(iter.value(), zone, groups);
}

void CampaignPlanAssignment::BuildAssetList(const int* pref, List<CombatGroup>& groups, List<CombatGroup>& assets)
{
  if (!pref)
    return;

  while (*pref)
  {
    ListIter<CombatGroup> g = groups;
    while (++g)
    {
      if (g->Type() == *pref && g->CountUnits() > 0)
        assets.append(g.value());
    }

    pref++;
  }
}

void CampaignPlanAssignment::ProcessZone(Combatant* c, CombatZone* zone)
{
  List<CombatGroup> groups;
  BuildZoneList(c->GetForce(), zone, groups);

  ZoneForce* force = zone->FindForce(c->GetIFF());

  // defensive assignments:
  ListIter<CombatGroup> def = force->GetDefendList();
  while (++def)
  {
    List<CombatGroup> assets;
    BuildAssetList(CombatGroup::PreferredDefender(def->Type()), groups, assets);

    ListIter<CombatGroup> g = assets;
    while (++g)
    {
      auto a = NEW CombatAssignment(Mission::DEFEND, def.value(), g.value());

      if (a)
        g->GetAssignments().append(a);
    }
  }

  // offensive assignments:
  ListIter<CombatGroup> tgt = force->GetTargetList();
  while (++tgt)
  {
    CombatGroup* target = tgt.value();

    List<CombatGroup> assets;
    BuildAssetList(CombatGroup::PreferredAttacker(tgt->Type()), groups, assets);

    ListIter<CombatGroup> g = assets;
    while (++g)
    {
      CombatGroup* asset = g.value();
      int mtype = Mission::ASSAULT;

      if (target->IsStrikeTarget())
        mtype = Mission::STRIKE;

      else if (target->IsFighterGroup())
        mtype = Mission::SWEEP;

      else if (target->Type() == CombatGroup::LCA_SQUADRON)
        mtype = Mission::INTERCEPT;

      auto a = NEW CombatAssignment(mtype, target, asset);

      if (a)
        g->GetAssignments().append(a);
    }
  }
}
