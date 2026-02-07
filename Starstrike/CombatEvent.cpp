#include "pch.h"
#include "CombatEvent.h"
#include "Campaign.h"
#include "CombatGroup.h"
#include "DataLoader.h"
#include "FormatUtil.h"
#include "Player.h"

CombatEvent::CombatEvent(Campaign* c, int typ, int tim, int tem, int src, std::string rgn)
  : campaign(c),
    type(typ),
    time(tim),
    team(tem),
    points(0),
    source(src),
    visited(false),
    region(std::move(rgn)) {}

std::string CombatEvent::SourceName() const { return SourceName(source); }

std::string CombatEvent::TypeName() const { return TypeName(type); }

std::string CombatEvent::SourceName(int n)
{
  switch (n)
  {
    case FORCOM:
      return "FORCOM";
    case TACNET:
      return "TACNET";
    case INTEL:
      return "SECURE";
    case MAIL:
      return "Mail";
    case NEWS:
      return "News";
  }

  return "Unknown";
}

int CombatEvent::SourceFromName(std::string n)
{
  for (int i = FORCOM; i <= NEWS; i++)
  {
    if (n == SourceName(i))
      return i;
  }

  return -1;
}

std::string CombatEvent::TypeName(int n)
{
  switch (n)
  {
    case ATTACK:
      return "ATTACK";
    case DEFEND:
      return "DEFEND";
    case MOVE_TO:
      return "MOVE_TO";
    case CAPTURE:
      return "CAPTURE";
    case STRATEGY:
      return "STRATEGY";
    case STORY:
      return "STORY";
    case CAMPAIGN_START:
      return "CAMPAIGN_START";
    case CAMPAIGN_END:
      return "CAMPAIGN_END";
    case CAMPAIGN_FAIL:
      return "CAMPAIGN_FAIL";
  }

  return "Unknown";
}

int CombatEvent::TypeFromName(std::string n)
{
  for (int i = ATTACK; i <= CAMPAIGN_FAIL; i++)
  {
    if (n == TypeName(i))
      return i;
  }

  return -1;
}

void CombatEvent::Load()
{
  DataLoader* loader = DataLoader::GetLoader();

  if (!campaign || !loader)
    return;

  loader->SetDataPath(campaign->Path());

  if (!file.empty())
  {BYTE* block = nullptr;

    loader->LoadBuffer(file, block, true);
    info = (const char*)block;
    loader->ReleaseBuffer(block);

    if (info.contains('$'))
    {
      Player* player = Player::GetCurrentPlayer();
      CombatGroup* group = campaign->GetPlayerGroup();

      if (player)
      {
        info = FormatTextReplace(info, "$NAME", player->Name());
        info = FormatTextReplace(info, "$RANK", Player::RankName(player->Rank()));
      }

      if (group)
        info = FormatTextReplace(info, "$GROUP", group->GetDescription());

      char timestr[32];
      FormatDayTime(timestr, campaign->GetTime(), true);
      info = FormatTextReplace(info, "$TIME", timestr);
    }
  }

  if (type < CAMPAIGN_END && !image_file.empty())
    loader->LoadBitmap(image_file, image);

  loader->SetDataPath();
}
