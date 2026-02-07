#include "pch.h"
#include "MenuDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "MenuScreen.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(MenuDlg, OnStart);
DEF_MAP_CLIENT(MenuDlg, OnCampaign);
DEF_MAP_CLIENT(MenuDlg, OnMission);
DEF_MAP_CLIENT(MenuDlg, OnPlayer);
DEF_MAP_CLIENT(MenuDlg, OnMultiplayer);
DEF_MAP_CLIENT(MenuDlg, OnTacReference);
DEF_MAP_CLIENT(MenuDlg, OnVideo);
DEF_MAP_CLIENT(MenuDlg, OnOptions);
DEF_MAP_CLIENT(MenuDlg, OnControls);
DEF_MAP_CLIENT(MenuDlg, OnQuit);
DEF_MAP_CLIENT(MenuDlg, OnButtonEnter);
DEF_MAP_CLIENT(MenuDlg, OnButtonExit);

extern const char* g_versionInfo;

MenuDlg::MenuDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    btn_start(nullptr),
    btn_campaign(nullptr),
    btn_mission(nullptr),
    btn_player(nullptr),
    btn_multi(nullptr),
    btn_tac(nullptr),
    btn_options(nullptr),
    btn_controls(nullptr),
    btn_quit(nullptr),
    version(nullptr),
    description(nullptr),
    stars(nullptr),
    campaign(nullptr)
{
  stars = Starshatter::GetInstance();
  campaign = Campaign::GetCampaign();

  Init(def);
}

MenuDlg::~MenuDlg() {}

void MenuDlg::RegisterControls()
{
  if (btn_start)
    return;

  btn_start = static_cast<Button*>(FindControl(120));
  btn_campaign = static_cast<Button*>(FindControl(101));
  btn_mission = static_cast<Button*>(FindControl(102));
  btn_player = static_cast<Button*>(FindControl(103));
  btn_multi = static_cast<Button*>(FindControl(104));
  btn_video = static_cast<Button*>(FindControl(111));
  btn_options = static_cast<Button*>(FindControl(112));
  btn_controls = static_cast<Button*>(FindControl(113));
  btn_quit = static_cast<Button*>(FindControl(114));
  btn_tac = static_cast<Button*>(FindControl(116));

  if (btn_start)
  {
    REGISTER_CLIENT(EID_CLICK, btn_start, MenuDlg, OnStart);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_start, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_start, MenuDlg, OnButtonExit);
  }

  if (btn_campaign)
  {
    REGISTER_CLIENT(EID_CLICK, btn_campaign, MenuDlg, OnCampaign);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_campaign, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_campaign, MenuDlg, OnButtonExit);
  }

  if (btn_mission)
  {
    REGISTER_CLIENT(EID_CLICK, btn_mission, MenuDlg, OnMission);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_mission, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_mission, MenuDlg, OnButtonExit);
  }

  if (btn_player)
  {
    REGISTER_CLIENT(EID_CLICK, btn_player, MenuDlg, OnPlayer);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_player, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_player, MenuDlg, OnButtonExit);
  }

  if (btn_multi)
  {
    REGISTER_CLIENT(EID_CLICK, btn_multi, MenuDlg, OnMultiplayer);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_multi, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_multi, MenuDlg, OnButtonExit);
  }

  if (btn_video)
  {
    REGISTER_CLIENT(EID_CLICK, btn_video, MenuDlg, OnVideo);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_video, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_video, MenuDlg, OnButtonExit);
  }

  if (btn_options)
  {
    REGISTER_CLIENT(EID_CLICK, btn_options, MenuDlg, OnOptions);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_options, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_options, MenuDlg, OnButtonExit);
  }

  if (btn_controls)
  {
    REGISTER_CLIENT(EID_CLICK, btn_controls, MenuDlg, OnControls);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_controls, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_controls, MenuDlg, OnButtonExit);
  }

  if (btn_tac)
  {
    REGISTER_CLIENT(EID_CLICK, btn_tac, MenuDlg, OnTacReference);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_tac, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_tac, MenuDlg, OnButtonExit);
  }

  if (btn_quit)
  {
    REGISTER_CLIENT(EID_CLICK, btn_quit, MenuDlg, OnQuit);
    REGISTER_CLIENT(EID_MOUSE_ENTER, btn_quit, MenuDlg, OnButtonEnter);
    REGISTER_CLIENT(EID_MOUSE_EXIT, btn_quit, MenuDlg, OnButtonExit);
  }

  version = FindControl(100);

  if (version)
    version->SetText(g_versionInfo);

  description = FindControl(202);
}

void MenuDlg::Show()
{
  FormWindow::Show();

  if (btn_multi)
    btn_multi->SetEnabled(false);
}

void MenuDlg::ExecFrame() {}

void MenuDlg::OnStart(AWEvent* event)
{
  if (description)
    description->SetText("");
  stars->StartOrResumeGame();
}

void MenuDlg::OnCampaign(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowCmpSelectDlg();
}

void MenuDlg::OnMission(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowMsnSelectDlg();
}

void MenuDlg::OnPlayer(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowPlayerDlg();
}

void MenuDlg::OnMultiplayer(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowNetClientDlg();
}

void MenuDlg::OnVideo(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowVidDlg();
}

void MenuDlg::OnOptions(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowOptDlg();
}

void MenuDlg::OnControls(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowCtlDlg();
}

void MenuDlg::OnTacReference(AWEvent* event)
{
  if (description)
    description->SetText("");
  stars->OpenTacticalReference();
}

void MenuDlg::OnQuit(AWEvent* event)
{
  if (description)
    description->SetText("");
  manager->ShowExitDlg();
}

void MenuDlg::OnButtonEnter(AWEvent* event)
{
  ActiveWindow* src = event->window;

  if (src && description)
    description->SetText(src->GetAltText());
}

void MenuDlg::OnButtonExit(AWEvent* event)
{
  if (description)
    description->SetText("");
}
