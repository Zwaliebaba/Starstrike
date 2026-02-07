#include "pch.h"
#include "CombatAction.h"
#include "Campaign.h"
#include "CombatGroup.h"
#include "Combatant.h"
#include "Player.h"
#include "Random.h"

CombatAction::CombatAction(int n, int typ, int sub, int iff)
  : id(n),
    type(typ),
    subtype(sub),
    opp_type(-1),
    team(iff),
    status(PENDING),
    min_rank(0),
    max_rank(100),
    source(0),
    count(0),
    start_before(static_cast<int>(1e9)),
    start_after(0),
    delay(0),
    probability(100),
    rval(-1),
    time(0),
    asset_type(0),
    asset_id(0),
    target_type(0),
    target_id(0),
    target_iff(0) {}

CombatAction::~CombatAction()
{
  requirements.destroy();
}

bool CombatAction::IsAvailable() const
{
  auto pThis = (CombatAction*)this;

  if (rval < 0)
  {
    pThis->rval = static_cast<int>(Random(0, 100));

    if (rval > probability)
      pThis->status = SKIPPED;
  }

  if (status != PENDING)
    return false;

  if (min_rank > 0 || max_rank < 100)
  {
    Player* player = Player::GetCurrentPlayer();

    if (player->Rank() < min_rank || player->Rank() > max_rank)
      return false;
  }

  Campaign* campaign = Campaign::GetCampaign();
  if (campaign)
  {
    if (campaign->GetTime() < start_after)
      return false;

    if (campaign->GetTime() > start_before)
    {
      pThis->status = FAILED; // too late!
      return false;
    }

    // check requirements against actions in current campaign:
    ListIter<CombatActionReq> iter = pThis->requirements;
    while (++iter)
    {
      CombatActionReq* r = iter.value();
      bool ok = false;

      if (r->action > 0)
      {
        ListIter<CombatAction> action = campaign->GetActions();
        while (++action)
        {
          CombatAction* a = action.value();

          if (a->Identity() == r->action)
          {
            if (r->m_not)
            {
              if (a->Status() == r->stat)
                return false;
            }
            else
            {
              if (a->Status() != r->stat)
                return false;
            }
          }
        }
      }

      // group-based requirement
      else if (r->group_type > 0)
      {
        if (r->c1)
        {
          CombatGroup* group = r->c1->FindGroup(r->group_type, r->group_id);

          if (group)
          {
            int test = 0;
            int comp = 0;

            if (r->intel)
            {
              test = group->IntelLevel();
              comp = r->intel;
            }

            else
            {
              test = group->CalcValue();
              comp = r->score;
            }

            switch (r->comp)
            {
              case CombatActionReq::LT:
                ok = (test < comp);
                break;
              case CombatActionReq::LE:
                ok = (test <= comp);
                break;
              case CombatActionReq::GT:
                ok = (test > comp);
                break;
              case CombatActionReq::GE:
                ok = (test >= comp);
                break;
              case CombatActionReq::EQ:
                ok = (test == comp);
                break;
            }
          }

          if (!ok)
            return false;
        }
      }

      // score-based requirement
      else
      {
        int test = 0;

        if (r->comp <= CombatActionReq::EQ)
        {  // absolute
          if (r->c1)
          {
            int test = r->c1->Score();

            switch (r->comp)
            {
              case CombatActionReq::LT:
                ok = (test < r->score);
                break;
              case CombatActionReq::LE:
                ok = (test <= r->score);
                break;
              case CombatActionReq::GT:
                ok = (test > r->score);
                break;
              case CombatActionReq::GE:
                ok = (test >= r->score);
                break;
              case CombatActionReq::EQ:
                ok = (test == r->score);
                break;
            }
          }
        }

        else
        {                                 // relative
          if (r->c1 && r->c2)
          {
            int test = r->c1->Score() - r->c2->Score();

            switch (r->comp)
            {
              case CombatActionReq::RLT:
                ok = (test < r->score);
                break;
              case CombatActionReq::RLE:
                ok = (test <= r->score);
                break;
              case CombatActionReq::RGT:
                ok = (test > r->score);
                break;
              case CombatActionReq::RGE:
                ok = (test >= r->score);
                break;
              case CombatActionReq::REQ:
                ok = (test == r->score);
                break;
            }
          }
        }

        if (!ok)
          return false;
      }

      if (delay > 0)
      {
        pThis->start_after = static_cast<int>(campaign->GetTime()) + delay;
        pThis->delay = 0;
        return IsAvailable();
      }
    }
  }

  return true;
}

void CombatAction::FireAction()
{
  Campaign* campaign = Campaign::GetCampaign();
  if (campaign)
    time = static_cast<int>(campaign->GetTime());

  if (count >= 1)
    count--;

  if (count < 1)
    status = COMPLETE;
}

void CombatAction::FailAction()
{
  Campaign* campaign = Campaign::GetCampaign();
  if (campaign)
    time = static_cast<int>(campaign->GetTime());

  count = 0;
  status = FAILED;
}

void CombatAction::AddRequirement(int action, int stat, bool _not)
{
  requirements.append(NEW CombatActionReq(action, stat, _not));
}

void CombatAction::AddRequirement(Combatant* c1, Combatant* c2, int comp, int score)
{
  requirements.append(NEW CombatActionReq(c1, c2, comp, score));
}

void CombatAction::AddRequirement(Combatant* c1, int group_type, int group_id, int comp, int score, int intel)
{
  requirements.append(NEW CombatActionReq(c1, group_type, group_id, comp, score, intel));
}

int CombatAction::TypeFromName(std::string_view n)
{
  int type = 0;

  if (n == "NO_ACTION")
    type = NO_ACTION;

  else if (n == "MARKER")
    type = NO_ACTION;

  else if (n == "STRATEGIC_DIRECTIVE")
    type = STRATEGIC_DIRECTIVE;

  else if (n == "STRATEGIC")
    type = STRATEGIC_DIRECTIVE;

  else if (n == "ZONE_ASSIGNMENT")
    type = ZONE_ASSIGNMENT;

  else if (n == "ZONE")
    type = ZONE_ASSIGNMENT;

  else if (n == "SYSTEM_ASSIGNMENT")
    type = SYSTEM_ASSIGNMENT;

  else if (n == "SYSTEM")
    type = SYSTEM_ASSIGNMENT;

  else if (n == "MISSION_TEMPLATE")
    type = MISSION_TEMPLATE;

  else if (n == "MISSION")
    type = MISSION_TEMPLATE;

  else if (n == "COMBAT_EVENT")
    type = COMBAT_EVENT;

  else if (n == "EVENT")
    type = COMBAT_EVENT;

  else if (n == "INTEL_EVENT")
    type = INTEL_EVENT;

  else if (n == "INTEL")
    type = INTEL_EVENT;

  else if (n == "CAMPAIGN_SITUATION")
    type = CAMPAIGN_SITUATION;

  else if (n == "SITREP")
    type = CAMPAIGN_SITUATION;

  else if (n == "CAMPAIGN_ORDERS")
    type = CAMPAIGN_ORDERS;

  else if (n == "ORDERS")
    type = CAMPAIGN_ORDERS;

  return type;
}

int CombatAction::StatusFromName(std::string_view n)
{
  int stat = 0;

  if (n == "PENDING")
    stat = PENDING;

  else if (n == "ACTIVE")
    stat = ACTIVE;

  else if (n == "SKIPPED")
    stat = SKIPPED;

  else if (n == "FAILED")
    stat = FAILED;

  else if (n == "COMPLETE")
    stat = COMPLETE;

  return stat;
}

int CombatActionReq::CompFromName(std::string_view n)
{
  int comp = 0;

  if (n == "LT")
    comp = LT;

  else if (n == "LE")
    comp = LE;

  else if (n == "GT")
    comp = GT;

  else if (n == "GE")
    comp = GE;

  else if (n == "EQ")
    comp = EQ;

  else if (n == "RLT")
    comp = RLT;

  else if (n == "RLE")
    comp = RLE;

  else if (n == "RGT")
    comp = RGT;

  else if (n == "RGE")
    comp = RGE;

  else if (n == "REQ")
    comp = REQ;

  return comp;
}
