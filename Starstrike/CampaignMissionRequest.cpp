#include "pch.h"
#include "CampaignMissionRequest.h"
#include "CombatGroup.h"

CampaignMissionRequest::CampaignMissionRequest(Campaign* c, int t, int s, CombatGroup* p, CombatGroup* tgt)
  : campaign(c),
    type(t),
    opp_type(-1),
    start(s),
    primary_group(p),
    secondary_group(nullptr),
    objective(tgt),
    use_loc(false) {}
