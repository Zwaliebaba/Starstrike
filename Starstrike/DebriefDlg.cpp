#include "pch.h"
#include "DebriefDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "Element.h"
#include "FormatUtil.h"
#include "Game.h"
#include "Instruction.h"
#include "Keyboard.h"
#include "ListBox.h"
#include "Mission.h"
#include "Mouse.h"
#include "NetLobby.h"
#include "PlanScreen.h"
#include "Player.h"
#include "Ship.h"
#include "Sim.h"
#include "SimEvent.h"
#include "StarSystem.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(DebriefDlg, OnClose);
DEF_MAP_CLIENT(DebriefDlg, OnUnit);

DebriefDlg::DebriefDlg(Screen* s, FormDef& def, PlanScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    close_btn(nullptr),
    campaign(nullptr),
    mission(nullptr),
    info(nullptr),
    unit_index(0),
    sim(nullptr),
    ship(nullptr)
{
  campaign = Campaign::GetCampaign();

  if (campaign)
    mission = campaign->GetMission();

  Init(def);
}

DebriefDlg::~DebriefDlg() {}

void DebriefDlg::RegisterControls()
{
  mission_name = FindControl(200);
  mission_system = FindControl(202);
  mission_sector = FindControl(204);
  mission_time_start = FindControl(206);

  objectives = FindControl(210);
  situation = FindControl(240);
  mission_score = FindControl(211);
  unit_list = static_cast<ListBox*>(FindControl(320));
  summary_list = static_cast<ListBox*>(FindControl(330));
  event_list = static_cast<ListBox*>(FindControl(340));

  if (unit_list)
    REGISTER_CLIENT(EID_SELECT, unit_list, DebriefDlg, OnUnit);

  close_btn = static_cast<Button*>(FindControl(1));

  if (close_btn)
    REGISTER_CLIENT(EID_CLICK, close_btn, DebriefDlg, OnClose);
}

void DebriefDlg::Show()
{
  FormWindow::Show();
  Game::SetTimeCompression(1);

  mission = nullptr;
  campaign = Campaign::GetCampaign();
  sim = Sim::GetSim();

  if (sim)
    ship = sim->GetPlayerShip();

  if (campaign)
    mission = campaign->GetMission();

  if (mission_name)
  {
    if (mission)
      mission_name->SetText(mission->Name());
    else
      mission_name->SetText(Game::GetText("DebriefDlg.mission-name"));
  }

  if (mission_system)
  {
    mission_system->SetText("");

    if (mission)
    {
      StarSystem* sys = mission->GetStarSystem();

      if (sys)
        mission_system->SetText(sys->Name());
    }
  }

  if (mission_sector)
  {
    mission_sector->SetText("");

    if (mission)
    {
      MissionElement* elem = mission->GetElements()[0];

      if (elem)
        mission_sector->SetText(elem->Region());
    }
  }

  if (mission_time_start)
  {
    if (mission)
    {
      char txt[32];
      FormatDayTime(txt, mission->Start());
      mission_time_start->SetText(txt);
    }
  }

  if (objectives)
  {
    bool found_objectives = false;

    if (sim && sim->GetPlayerElement())
    {
      std::string text;
      Element* elem = sim->GetPlayerElement();

      for (int i = 0; i < elem->NumObjectives(); i++)
      {
        Instruction* obj = elem->GetObjective(i);
        text += std::string("* ") + obj->GetDescription() + std::string("\n");

        found_objectives = true;
      }

      objectives->SetText(text);
    }

    if (!found_objectives)
    {
      if (mission)
        objectives->SetText(mission->Objective());
      else
        objectives->SetText(Game::GetText("DebriefDlg.unspecified"));
    }
  }

  if (situation)
  {
    if (mission)
      situation->SetText(mission->Situation());
    else
      situation->SetText(Game::GetText("DebriefDlg.unknown"));
  }

  if (mission_score)
  {
    mission_score->SetText(Game::GetText("DebriefDlg.no-stats"));

    if (ship)
    {
      for (int i = 0; i < ShipStats::NumStats(); i++)
      {
        ShipStats* stats = ShipStats::GetStats(i);
        if (stats && ship->Name() == stats->GetName())
        {
          stats->Summarize();

          Player* player = Player::GetCurrentPlayer();
          int points = stats->GetPoints() + stats->GetCommandPoints();

          if (player && sim)
            points = player->GetMissionPoints(stats, sim->StartTime()) + stats->GetCommandPoints();

          char score[32];
          sprintf_s(score, "%d %s", points, Game::GetText("DebriefDlg.points").data());
          mission_score->SetText(score);
          break;
        }
      }
    }
  }

  DrawUnits();
}

void DebriefDlg::DrawUnits()
{
  if (mission)
  {
    if (unit_list)
    {
      unit_list->ClearItems();

      int seln = -1;
      bool netgame = false;

      if (sim && sim->IsNetGame())
        netgame = true;

      for (int i = 0; i < ShipStats::NumStats(); i++)
      {
        ShipStats* stats = ShipStats::GetStats(i);
        stats->Summarize();

        if (netgame || (stats->GetIFF() == mission->Team() && stats->GetRegion() == mission->GetRegion()))
        {
          int n = unit_list->AddItemWithData(" ", i) - 1;
          unit_list->SetItemText(n, 1, stats->GetName().c_str());
          unit_list->SetItemText(n, 2, stats->GetRole());
          unit_list->SetItemText(n, 3, stats->GetType());

          if (ship && ship->Name() == stats->GetName())
            seln = n;
        }
      }

      if (seln >= 0)
      {
        unit_list->SetSelected(seln);
        OnUnit(nullptr);
      }
    }
  }
}

void DebriefDlg::ExecFrame()
{
  if (unit_list && unit_list->NumItems() && unit_list->GetSelCount() < 1)
  {
    unit_list->SetSelected(0);
    OnUnit(nullptr);
  }

  if (Keyboard::KeyDown(VK_RETURN))
    OnClose(nullptr);
}

void DebriefDlg::OnUnit(AWEvent* event)
{
  if (!unit_list || !event_list || !summary_list)
    return;

  summary_list->ClearItems();
  event_list->ClearItems();

  int seln = unit_list->GetSelection();
  int unit = unit_list->GetItemData(seln);

  ShipStats* stats = ShipStats::GetStats(unit);
  if (stats)
  {
    stats->Summarize();

    char txt[64];
    int i = 0;

    sprintf_s(txt, "%d", stats->GetGunShots());
    summary_list->AddItem("Guns Fired: ");
    summary_list->SetItemText(i++, 1, txt);

    sprintf_s(txt, "%d", stats->GetGunHits());
    summary_list->AddItem("Gun Hits: ");
    summary_list->SetItemText(i++, 1, txt);

    sprintf_s(txt, "%d", stats->GetGunKills());
    summary_list->AddItem("Gun Kills: ");
    summary_list->SetItemText(i++, 1, txt);

    // one line spacer:
    summary_list->AddItem(" ");
    i++;

    sprintf_s(txt, "%d", stats->GetMissileShots());
    summary_list->AddItem("Missiles Fired: ");
    summary_list->SetItemText(i++, 1, txt);

    sprintf_s(txt, "%d", stats->GetMissileHits());
    summary_list->AddItem("Missile Hits: ");
    summary_list->SetItemText(i++, 1, txt);

    sprintf_s(txt, "%d", stats->GetMissileKills());
    summary_list->AddItem("Missile Kills: ");
    summary_list->SetItemText(i++, 1, txt);

    i = 0;
    ListIter<SimEvent> iter = stats->GetEvents();
    while (++iter)
    {
      SimEvent* event = iter.value();

      char txt[64];
      int time = event->GetTime();

      if (time > 24 * 60 * 60)
        FormatDayTime(txt, time);
      else
        FormatTime(txt, time);

      event_list->AddItem(txt);
      event_list->SetItemText(i, 1, event->GetEventDesc());

      if (!event->GetTarget().empty())
        event_list->SetItemText(i, 2, event->GetTarget());

      i++;
    }
  }
}

void DebriefDlg::OnClose(AWEvent* event)
{
  Sim* sim = Sim::GetSim();

  sim->CommitMission();
  sim->UnloadMission();

  NetLobby* lobby = NetLobby::GetInstance();
  if (lobby && lobby->IsHost())
  {
    lobby->SelectMission(0);
    lobby->ExecFrame();
  }

  Player* player = Player::GetCurrentPlayer();
  if (player && player->ShowAward())
    manager->ShowAwardDlg();

  else
  {
    Starshatter* stars = Starshatter::GetInstance();

    if (stars)
    {
      Mouse::Show(false);

      Campaign* campaign = Campaign::GetCampaign();
      if (campaign && campaign->GetCampaignId() < Campaign::SINGLE_MISSIONS)
        stars->SetGameMode(Starshatter::CMPN_MODE);
      else
        stars->SetGameMode(Starshatter::MENU_MODE);
    }

    else
      Game::Panic("DebriefDlg::OnClose() - Game instance not found");
  }
}
