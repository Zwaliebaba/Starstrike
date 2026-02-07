#include "pch.h"
#include "CampaignSaveGame.h"
#include "Campaign.h"
#include "CombatAction.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "Combatant.h"
#include "DataLoader.h"
#include "FormatUtil.h"
#include "ParseUtil.h"
#include "Player.h"
#include "Reader.h"

static auto SAVE_DIR = "SaveGame";

CampaignSaveGame::CampaignSaveGame(Campaign* c)
  : campaign(c) {}

CampaignSaveGame::~CampaignSaveGame() {}

std::string CampaignSaveGame::GetSaveDirectory() { return GetSaveDirectory(Player::GetCurrentPlayer()); }

std::string CampaignSaveGame::GetSaveDirectory(Player* player)
{
  if (player)
  {
    char save_dir[32];
    sprintf_s(save_dir, "%s\\%02d", SAVE_DIR, player->Identity());

    return save_dir;
  }

  return SAVE_DIR;
}

void CampaignSaveGame::CreateSaveDirectory()
{
  HANDLE hDir = CreateFileA(SAVE_DIR, 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);

  if (hDir == INVALID_HANDLE_VALUE)
    CreateDirectoryA(SAVE_DIR, nullptr);
  else
    CloseHandle(hDir);

  hDir = CreateFileA(GetSaveDirectory().c_str(), 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);

  if (hDir == INVALID_HANDLE_VALUE)
    CreateDirectoryA(GetSaveDirectory().c_str(), nullptr);
  else
    CloseHandle(hDir);
}

static char g_multiline[4096];

static std::string FormatMultiLine(std::string_view _s)
{
  int i = 4095;
  char* p = g_multiline;
  std::string_view::const_iterator s = _s.begin();

  while (s != _s.end() && i > 0)
  {
    if (*s == '\n')
    {
      *p++ = '\\';
      *p++ = 'n';
      ++s;
      i -= 2;
    }
    else if (*s == '"')
    {
      *p++ = '\\';
      *p++ = '"';
      ++s;
      i -= 2;
    }
    else
    {
      *p++ = *s++;
      i--;
    }
  }

  *p = 0;
  return g_multiline;
}

static std::string  ParseMultiLine(std::string _s)
{
  int i = 4095;
  char* p = g_multiline;
  std::string::iterator s = _s.begin();

  while (s != _s.end() && i > 0)
  {
    if (*s == '\\')
    {
      ++s;
      if (*s == 'n')
      {
        *p++ = '\n';
        ++s;
        i--;
      }
      else if (*s == '"')
      {
        *p++ = '"';
        ++s;
        i--;
      }
      else
      {
        *p++ = *s++;
        i--;
      }
    }
    else
    {
      *p++ = *s++;
      i--;
    }
  }

  *p = 0;
  return g_multiline;
}

void CampaignSaveGame::Load(std::string_view filename)
{
  DebugTrace("-------------------------\nLOADING SAVEGAME ({}).\n", filename);
  campaign = nullptr;

  if (filename.empty())
    return;

  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath(GetSaveDirectory() + "\\");

  BYTE* block;
  loader->LoadBuffer(filename, block, true);

  Sleep(10);

  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse save game '{}'\n", filename);
    loader->SetDataPath();
    return;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "SAVEGAME")
  {
    DebugTrace("ERROR: invalid save game file '{}'\n", filename);
    term->print(10);
    loader->SetDataPath();
    delete term;
    return;
  }

  int grp_iff = 0;
  int grp_type = 0;
  int grp_id = 0;
  int status = 0;
  double baseTime = 0;
  double time = 0;
  std::string unit;
  std::string sitrep;
  std::string orders;

  do
  {
    Sleep(5);
    delete term;
    term = nullptr;
    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
      {
        if (def->name()->value() == "campaign")
        {
          std::string cname;
          int cid = 0;

          if (def->term())
          {
            if (def->term()->isText())
              cname = def->term()->isText()->value();
            else if (def->term()->isNumber())
              cid = static_cast<int>(def->term()->isNumber()->value());
          }

          if (!campaign)
          {
            List<Campaign>& list = Campaign::GetAllCampaigns();

            for (int i = 0; i < list.size() && !campaign; i++)
            {
              Campaign* c = list.at(i);

              if (cname == c->Name() || cid == c->GetCampaignId())
              {
                campaign = c;
                campaign->Load();
                campaign->Prep(); // restore campaign to pristine state

                loader->SetDataPath(GetSaveDirectory() + "\\");
              }
            }
          }
        }

        else if (def->name()->value() == "grp_iff")
          GetDefNumber(grp_iff, def, filename);

        else if (def->name()->value() == "grp_type")
          GetDefNumber(grp_type, def, filename);

        else if (def->name()->value() == "grp_id")
          GetDefNumber(grp_id, def, filename);

        else if (def->name()->value() == "unit")
          GetDefText(unit, def, filename);

        else if (def->name()->value() == "status")
          GetDefNumber(status, def, filename);

        else if (def->name()->value() == "basetime")
          GetDefNumber(baseTime, def, filename);

        else if (def->name()->value() == "time")
          GetDefNumber(time, def, filename);

        else if (def->name()->value() == "sitrep")
          GetDefText(sitrep, def, filename);

        else if (def->name()->value() == "orders")
          GetDefText(orders, def, filename);

        else if (def->name()->value() == "combatant")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: combatant struct missing in '{}/{}'\n", loader->GetDataPath(), filename);
          else
          {
            TermStruct* val = def->term()->isStruct();

            std::string name;
            int iff = 0;
            int score = 0;

            for (int i = 0; i < val->elements()->size(); i++)
            {
              TermDef* pdef = val->elements()->at(i)->isDef();
              if (pdef)
              {
                if (pdef->name()->value() == "name")
                  GetDefText(name, pdef, filename);

                else if (pdef->name()->value() == "iff")
                  GetDefNumber(iff, pdef, filename);

                else if (pdef->name()->value() == "score")
                  GetDefNumber(score, pdef, filename);
              }
            }

            if (campaign && name[0])
            {
              Combatant* combatant = campaign->GetCombatant(name);

              if (combatant)
              {
                CombatGroup::MergeOrderOfBattle(block, filename, iff, combatant, campaign);
                combatant->SetScore(score);
              }
              else
                DebugTrace("WARNING: could not find combatant '{}' in campaign.\n", name);
            }
          }
        }
        else if (def->name()->value() == "event")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: event struct missing in '{}/{}'\n", loader->GetDataPath(), filename);
          else
          {
            TermStruct* val = def->term()->isStruct();

            std::string type;
            std::string source;
            std::string region;
            std::string title;
            std::string file;
            std::string image;
            std::string scene;
            std::string info;
            int time = 0;
            int team = 0;
            int points = 0;

            for (int i = 0; i < val->elements()->size(); i++)
            {
              TermDef* pdef = val->elements()->at(i)->isDef();
              if (pdef)
              {
                if (pdef->name()->value() == "type")
                  GetDefText(type, pdef, filename);

                else if (pdef->name()->value() == "source")
                  GetDefText(source, pdef, filename);

                else if (pdef->name()->value() == "region")
                  GetDefText(region, pdef, filename);

                else if (pdef->name()->value() == "title")
                  GetDefText(title, pdef, filename);

                else if (pdef->name()->value() == "file")
                  GetDefText(file, pdef, filename);

                else if (pdef->name()->value() == "image")
                  GetDefText(image, pdef, filename);

                else if (pdef->name()->value() == "scene")
                  GetDefText(scene, pdef, filename);

                else if (pdef->name()->value() == "info")
                  GetDefText(info, pdef, filename);

                else if (pdef->name()->value() == "time")
                  GetDefNumber(time, pdef, filename);

                else if (pdef->name()->value() == "team")
                  GetDefNumber(team, pdef, filename);

                else if (pdef->name()->value() == "points")
                  GetDefNumber(points, pdef, filename);
              }
            }

            if (campaign && type[0])
            {
              loader->SetDataPath(campaign->Path());

              auto event = NEW CombatEvent(campaign, CombatEvent::TypeFromName(type), time, team,
                                           CombatEvent::SourceFromName(source), region);

              if (event)
              {
                event->SetTitle(title);
                event->SetFilename(file);
                event->SetImageFile(image);
                event->SetSceneFile(scene);
                event->Load();

                if (info[0])
                  event->SetInformation(ParseMultiLine(info));

                event->SetVisited(true);
                campaign->GetEvents().append(event);
              }
            }
          }
        }
        else if (def->name()->value() == "action")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: action struct missing in '%s/%s'\n", loader->GetDataPath(), filename);
          else
          {
            TermStruct* val = def->term()->isStruct();

            int id = -1;
            int stat = 0;
            int count = 0;
            int after = 0;

            for (int i = 0; i < val->elements()->size(); i++)
            {
              TermDef* pdef = val->elements()->at(i)->isDef();
              if (pdef)
              {
                if (pdef->name()->value() == "id")
                  GetDefNumber(id, pdef, filename);

                else if (pdef->name()->value() == "stat")
                  GetDefNumber(stat, pdef, filename);

                else if (pdef->name()->value() == "count")
                  GetDefNumber(count, pdef, filename);

                else if (pdef->name()->value().contains("after"))
                  GetDefNumber(after, pdef, filename);
              }
            }

            if (campaign && id >= 0)
            {
              ListIter<CombatAction> a_iter = campaign->GetActions();
              while (++a_iter)
              {
                CombatAction* a = a_iter.value();
                if (a->Identity() == id)
                {
                  a->SetStatus(stat);

                  if (count)
                    a->SetCount(count);

                  if (after)
                    a->SetStartAfter(after);

                  break;
                }
              }
            }
          }
        }
      }
    }
  }
  while (term);

  if (term)
  {
    delete term;
    term = nullptr;
  }

  if (campaign)
  {
    campaign->SetSaveGame(true);

    List<Campaign>& list = Campaign::GetAllCampaigns();

    if (status < Campaign::CAMPAIGN_SUCCESS)
    {
      campaign->SetStatus(status);
      if (sitrep.length())
        campaign->SetSituation(sitrep);
      if (orders.length())
        campaign->SetOrders(orders);
      campaign->SetStartTime(baseTime);
      campaign->SetLoadTime(baseTime + time);
      campaign->LockoutEvents(3600);
      campaign->Start();

      if (grp_type >= CombatGroup::FLEET && grp_type <= CombatGroup::PRIVATE)
      {
        CombatGroup* player_group = campaign->FindGroup(grp_iff, grp_type, grp_id);
        if (player_group)
        {
          CombatUnit* player_unit = nullptr;

          if (unit.length())
            player_unit = player_group->FindUnit(unit);

          if (player_unit)
            campaign->SetPlayerUnit(player_unit);
          else
            campaign->SetPlayerGroup(player_group);
        }
      }
    }

    // failed - restart current campaign:
    else if (status == Campaign::CAMPAIGN_FAILED)
    {
      DebugTrace("CampaignSaveGame: Loading FAILED campaign, restarting '{}'\n", campaign->Name());

      campaign->Load();
      campaign->Prep(); // restore campaign to pristine state
      campaign->SetSaveGame(false);

      loader->SetDataPath(GetSaveDirectory() + "\\");
    }

    // start next campaign:
    else if (status == Campaign::CAMPAIGN_SUCCESS)
    {
      DebugTrace("CampaignSaveGame: Loading COMPLETED campaign '{}', searching for next campaign...\n",
                 campaign->Name());

      bool found = false;

      for (int i = 0; i < list.size() && !found; i++)
      {
        Campaign* c = list.at(i);

        if (c->GetCampaignId() == campaign->GetCampaignId() + 1)
        {
          campaign = c;
          campaign->Load();
          campaign->Prep(); // restore campaign to pristine state

          DebugTrace("Advanced to campaign {} '{}'\n", campaign->GetCampaignId(), campaign->Name());

          loader->SetDataPath(GetSaveDirectory() + "\\");
          found = true;
        }
      }

      // if no next campaign found, start over from the beginning:
      for (int i = 0; i < list.size() && !found; i++)
      {
        Campaign* c = list.at(i);

        if (c->IsDynamic())
        {
          campaign = c;
          campaign->Load();
          campaign->Prep(); // restore campaign to pristine state

          DebugTrace("Completed full series, restarting at {} '{}'\n", campaign->GetCampaignId(), campaign->Name());

          loader->SetDataPath(GetSaveDirectory() + "\\");
          found = true;
        }
      }
    }
  }

  loader->ReleaseBuffer(block);
  loader->SetDataPath();
  DebugTrace("SAVEGAME LOADED ({}).\n\n", filename);
}

void CampaignSaveGame::Save(std::string_view name)
{
  if (!campaign)
    return;

  CreateSaveDirectory();

  std::string s = GetSaveDirectory() + std::string("\\") + std::string(name);

  FILE* f;
  fopen_s(&f, s.c_str(), "w");

  if (f)
  {
    char timestr[32];
    FormatDayTime(timestr, campaign->GetTime());

    CombatGroup* player_group = campaign->GetPlayerGroup();
    CombatUnit* player_unit = campaign->GetPlayerUnit();

    fprintf(f, "SAVEGAME\n\n");
    fprintf(f, "campaign: \"%s\"\n\n", campaign->Name().c_str());
    fprintf(f, "grp_iff:  %d\n", player_group->GetIFF());
    fprintf(f, "grp_type: %d\n", player_group->Type());
    fprintf(f, "grp_id:   %d\n", player_group->GetID());
    if (player_unit)
      fprintf(f, "unit:     \"%s\"\n", player_unit->Name().data());

    fprintf(f, "status:   %d\n", campaign->GetStatus());
    fprintf(f, "basetime: %f\n", campaign->GetStartTime());
    fprintf(f, "time:     %f // %s\n\n", campaign->GetTime(), timestr);

    fprintf(f, "sitrep:   \"%s\"\n", campaign->Situation().c_str());
    fprintf(f, "orders:   \"%s\"\n\n", campaign->Orders().c_str());

    ListIter<Combatant> c_iter = campaign->GetCombatants();
    while (++c_iter)
    {
      Combatant* c = c_iter.value();

      fprintf(f, "combatant: {");
      fprintf(f, " name:\"%s\",", c->Name().c_str());
      fprintf(f, " iff:%d,", c->GetIFF());
      fprintf(f, " score:%d,", c->Score());
      fprintf(f, " }\n");
    }

    fprintf(f, "\n");

    ListIter<CombatAction> a_iter = campaign->GetActions();
    while (++a_iter)
    {
      CombatAction* a = a_iter.value();
      fprintf(f, "action: { id:%4d, stat:%d", a->Identity(), a->Status());

      if (a->Status() == CombatAction::PENDING)
      {
        if (a->Count())
          fprintf(f, ", count:%d", a->Count());

        if (a->StartAfter())
          fprintf(f, ", after:%d", a->StartAfter());
      }

      fprintf(f, " }\n");
    }

    fprintf(f, "\n");

    ListIter<CombatEvent> e_iter = campaign->GetEvents();
    while (++e_iter)
    {
      CombatEvent* e = e_iter.value();

      fprintf(f, "event: {");
      fprintf(f, " type:%-18s,", e->TypeName().c_str());
      fprintf(f, " time:0x%08x,", e->Time());
      fprintf(f, " team:%d,", e->GetIFF());
      fprintf(f, " points:%d,", e->Points());
      fprintf(f, " source:\"%s\",", e->SourceName().c_str());
      fprintf(f, " region:\"%s\",", e->Region().c_str());
      fprintf(f, " title:\"%s\",", e->Title().c_str());
      if (!e->Filename().empty())
        fprintf(f, " file:\"%s\",", e->Filename().c_str());
      if (!e->ImageFile().empty())
        fprintf(f, " image:\"%s\",", e->ImageFile().c_str());
      if (!e->SceneFile().empty())
        fprintf(f, " scene:\"%s\",", e->SceneFile().c_str());
      if (e->Filename().empty())
        fprintf(f, " info:\"%s\"", FormatMultiLine(e->Information()).c_str());
      fprintf(f, " }\n");
    }

    fprintf(f, "\n// ORDER OF BATTLE:\n\n");
    fclose(f);

    c_iter.reset();
    while (++c_iter)
    {
      Combatant* c = c_iter.value();
      CombatGroup* g = c->GetForce();
      CombatGroup::SaveOrderOfBattle(s, g);
    }
  }
}

void CampaignSaveGame::Delete(const std::string& name) { DeleteFileA(std::string(GetSaveDirectory() + "\\" + name).c_str()); }

void CampaignSaveGame::RemovePlayer(Player* p)
{
  std::vector<std::string> save_list;
  std::string save_dir = GetSaveDirectory(p) + "\\";

  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath(save_dir);
  loader->ListFiles("*.*", save_list);
  loader->SetDataPath();

  for (int i = 0; i < save_list.size(); i++)
  {
    auto filename = save_list[i];
    DeleteFileA(std::string(save_dir + filename).c_str());
  }

  RemoveDirectoryA(GetSaveDirectory(p).c_str());
}

void CampaignSaveGame::SaveAuto() { Save("AutoSave"); }

void CampaignSaveGame::LoadAuto() { Load("AutoSave"); }

std::string CampaignSaveGame::GetResumeFile()
{
  // check for auto save game:
  FILE* f;
  fopen_s(&f,std::string(GetSaveDirectory() + "\\AutoSave").c_str(), "r");

  if (f)
  {
    fclose(f);

    return "AutoSave";
  }

  return std::string();
}

int CampaignSaveGame::GetSaveGameList(std::vector<std::string>& save_list)
{
  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath(GetSaveDirectory() + "\\");
  loader->ListFiles("*.*", save_list);
  loader->SetDataPath();

  return save_list.size();
}
