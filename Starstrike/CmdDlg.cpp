#include "pch.h"
#include "CmdDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "CmpFileDlg.h"
#include "CmpnScreen.h"
#include "CombatGroup.h"
#include "FormatUtil.h"
#include "Mouse.h"
#include "Starshatter.h"

CmdDlg::CmdDlg(CmpnScreen* mgr)
  : cmpn_screen(mgr),
    txt_group(nullptr),
    txt_score(nullptr),
    txt_name(nullptr),
    txt_time(nullptr),
    btn_save(nullptr),
    btn_exit(nullptr),
    stars(nullptr),
    campaign(nullptr),
    mode(0)
{
  stars = Starshatter::GetInstance();
  campaign = Campaign::GetCampaign();

  for (int i = 0; i < 5; i++)
    btn_mode[i] = nullptr;
}

CmdDlg::~CmdDlg() {}

void CmdDlg::RegisterCmdControls(FormWindow* win)
{
  btn_save = static_cast<Button*>(win->FindControl(1));
  btn_exit = static_cast<Button*>(win->FindControl(2));

  for (int i = 0; i < 5; i++)
    btn_mode[i] = static_cast<Button*>(win->FindControl(100 + i));

  txt_group = win->FindControl(200);
  txt_score = win->FindControl(201);
  txt_name = win->FindControl(300);
  txt_time = win->FindControl(301);
}

void CmdDlg::ShowCmdDlg()
{
  campaign = Campaign::GetCampaign();

  if (txt_name)
  {
    if (campaign)
      txt_name->SetText(campaign->Name());
    else
      txt_name->SetText("No Campaign Selected");
  }

  ShowMode();

  if (btn_save)
    btn_save->SetEnabled(!campaign->IsTraining());

  if (btn_mode[MODE_FORCES])
    btn_mode[MODE_FORCES]->SetEnabled(!campaign->IsTraining());
  if (btn_mode[MODE_INTEL])
    btn_mode[MODE_INTEL]->SetEnabled(!campaign->IsTraining());
}

void CmdDlg::ExecFrame()
{
  CombatGroup* g = campaign->GetPlayerGroup();
  if (g && txt_group)
    txt_group->SetText(g->GetDescription());

  if (txt_score)
  {
    char score[32];
    sprintf_s(score, "Team Score: %d", campaign->GetPlayerTeamScore());
    txt_score->SetText(score);
    txt_score->SetTextAlign(DT_RIGHT);
  }

  if (txt_time)
  {
    double t = campaign->GetTime();
    char daytime[32];
    FormatDayTime(daytime, t);
    txt_time->SetText(daytime);
  }

  int unread = campaign->CountNewEvents();

  if (btn_mode[MODE_INTEL])
  {
    if (unread > 0)
    {
      char text[64];
      sprintf_s(text, "INTEL (%d)", unread);
      btn_mode[MODE_INTEL]->SetText(text);
    }
    else
      btn_mode[MODE_INTEL]->SetText("INTEL");
  }
}

void CmdDlg::ShowMode()
{
  for (int i = 0; i < 5; i++)
    if (btn_mode[i])
      btn_mode[i]->SetButtonState(0);

  if (mode < 0 || mode > 4)
    mode = 0;

  if (btn_mode[mode])
    btn_mode[mode]->SetButtonState(1);
}

void CmdDlg::OnMode(AWEvent* event)
{
  for (int i = MODE_ORDERS; i < NUM_MODES; i++)
  {
    Button* btn = btn_mode[i];

    if (event->window == btn)
      mode = i;
  }

  switch (mode)
  {
    case MODE_ORDERS:
      cmpn_screen->ShowCmdOrdersDlg();
      break;
    case MODE_THEATER:
      cmpn_screen->ShowCmdTheaterDlg();
      break;
    case MODE_FORCES:
      cmpn_screen->ShowCmdForceDlg();
      break;
    case MODE_INTEL:
      cmpn_screen->ShowCmdIntelDlg();
      break;
    case MODE_MISSIONS:
      cmpn_screen->ShowCmdMissionsDlg();
      break;
    default:
      cmpn_screen->ShowCmdOrdersDlg();
      break;
  }
}

void CmdDlg::OnSave(AWEvent* event)
{
  if (campaign && cmpn_screen)
  {
    CmpFileDlg* fdlg = cmpn_screen->GetCmpFileDlg();

    cmpn_screen->ShowCmpFileDlg();
  }
}

void CmdDlg::OnExit(AWEvent* event)
{
  if (stars)
  {
    Mouse::Show(false);
    stars->SetGameMode(Starshatter::MENU_MODE);
  }
}
