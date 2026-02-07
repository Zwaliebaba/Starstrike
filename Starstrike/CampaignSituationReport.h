#pragma once

class Campaign;
class CombatGroup;
class CombatUnit;
class CombatZone;
class Mission;
class MissionElement;

class CampaignSituationReport
{
  public:
    CampaignSituationReport(Campaign* c, Mission* m);
    virtual ~CampaignSituationReport();

    virtual void GenerateSituationReport();

  protected:
    virtual void GlobalSituation();
    virtual void MissionSituation();
    virtual MissionElement* FindEscort(MissionElement* elem);
    virtual std::string GetThreatInfo();

    Campaign* campaign;
    Mission* mission;
    std::string sitrep;
};
