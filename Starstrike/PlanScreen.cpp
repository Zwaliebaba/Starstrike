#include "pch.h"
#include "PlanScreen.h"
#include "ActiveWindow.h"
#include "AwardDlg.h"
#include "Campaign.h"
#include "Color.h"
#include "DataLoader.h"
#include "DebriefDlg.h"
#include "FormDef.h"
#include "Game.h"
#include "Mission.h"
#include "Mouse.h"
#include "MsnNavDlg.h"
#include "MsnObjDlg.h"
#include "MsnPkgDlg.h"
#include "MsnWepDlg.h"
#include "Screen.h"
#include "Starshatter.h"

PlanScreen::PlanScreen()
  : screen(nullptr),
    objdlg(nullptr),
    pkgdlg(nullptr),
    wepdlg(nullptr),
    navdlg(nullptr),
    debrief_dlg(nullptr),
    award_dlg(nullptr),
    isShown(false) { loader = DataLoader::GetLoader(); }

PlanScreen::~PlanScreen() { TearDown(); }

void PlanScreen::Setup(Screen* s)
{
  if (!s)
    return;

  screen = s;

  // create windows

  FormDef msn_obj_def("MsnObjDlg", 0);
  msn_obj_def.Load("MsnObjDlg");
  objdlg = NEW MsnObjDlg(screen, msn_obj_def, this);

  FormDef msn_pkg_def("MsnPkgDlg", 0);
  msn_pkg_def.Load("MsnPkgDlg");
  pkgdlg = NEW MsnPkgDlg(screen, msn_pkg_def, this);

  FormDef msn_nav_def("MsnNavDlg", 0);
  msn_nav_def.Load("MsnNavDlg");
  navdlg = NEW MsnNavDlg(screen, msn_nav_def, this);

  FormDef msn_wep_def("MsnWepDlg", 0);
  msn_wep_def.Load("MsnWepDlg");
  wepdlg = NEW MsnWepDlg(screen, msn_wep_def, this);

  FormDef award_def("AwardDlg", 0);
  award_def.Load("AwardDlg");
  award_dlg = NEW AwardDlg(screen, award_def, this);

  FormDef debrief_def("DebriefDlg", 0);
  debrief_def.Load("DebriefDlg");
  debrief_dlg = NEW DebriefDlg(screen, debrief_def, this);

  ShowMsnDlg();
}

void PlanScreen::TearDown()
{
  if (screen)
  {
    screen->DelWindow(objdlg);
    screen->DelWindow(pkgdlg);
    screen->DelWindow(wepdlg);
    screen->DelWindow(navdlg);
    screen->DelWindow(debrief_dlg);
    screen->DelWindow(award_dlg);
  }

  delete objdlg;
  delete pkgdlg;
  delete wepdlg;
  delete navdlg;
  delete debrief_dlg;
  delete award_dlg;

  objdlg = nullptr;
  pkgdlg = nullptr;
  wepdlg = nullptr;
  navdlg = nullptr;
  debrief_dlg = nullptr;
  award_dlg = nullptr;
  screen = nullptr;
}

void PlanScreen::ExecFrame()
{
  Game::SetScreenColor(Color::Black);

  Mission* mission = nullptr;
  Campaign* campaign = Campaign::GetCampaign();

  if (campaign)
    mission = campaign->GetMission();

  if (navdlg)
  {
    navdlg->SetMission(mission);

    if (navdlg->IsShown())
      navdlg->ExecFrame();
  }

  if (objdlg && objdlg->IsShown())
    objdlg->ExecFrame();

  if (pkgdlg && pkgdlg->IsShown())
    pkgdlg->ExecFrame();

  if (wepdlg && wepdlg->IsShown())
    wepdlg->ExecFrame();

  if (award_dlg && award_dlg->IsShown())
    award_dlg->ExecFrame();

  if (debrief_dlg && debrief_dlg->IsShown())
    debrief_dlg->ExecFrame();
}

bool PlanScreen::CloseTopmost()
{
  if (debrief_dlg->IsShown())
    debrief_dlg->OnClose(nullptr);

  if (award_dlg->IsShown())
    return true;

  return false;
}

void PlanScreen::Show()
{
  if (!isShown)
  {
    ShowMsnDlg();
    isShown = true;
  }
}

void PlanScreen::Hide()
{
  HideAll();
  isShown = false;
}

void PlanScreen::ShowMsnDlg()
{
  HideAll();
  Mouse::Show(true);
  objdlg->Show();
}

void PlanScreen::HideMsnDlg()
{
  HideAll();
  Mouse::Show(true);
  objdlg->Show();
}

bool PlanScreen::IsMsnShown() { return IsMsnObjShown() || IsMsnPkgShown() || IsMsnWepShown(); }

void PlanScreen::ShowMsnObjDlg()
{
  HideAll();
  Mouse::Show(true);
  objdlg->Show();
}

void PlanScreen::HideMsnObjDlg()
{
  HideAll();
  Mouse::Show(true);
}

bool PlanScreen::IsMsnObjShown() { return objdlg && objdlg->IsShown(); }

void PlanScreen::ShowMsnPkgDlg()
{
  HideAll();
  Mouse::Show(true);
  pkgdlg->Show();
}

void PlanScreen::HideMsnPkgDlg()
{
  HideAll();
  Mouse::Show(true);
}

bool PlanScreen::IsMsnPkgShown() { return pkgdlg && pkgdlg->IsShown(); }

void PlanScreen::ShowMsnWepDlg()
{
  HideAll();
  Mouse::Show(true);
  wepdlg->Show();
}

void PlanScreen::HideMsnWepDlg()
{
  HideAll();
  Mouse::Show(true);
}

bool PlanScreen::IsMsnWepShown() { return wepdlg && wepdlg->IsShown(); }

void PlanScreen::ShowNavDlg()
{
  if (navdlg && !navdlg->IsShown())
  {
    HideAll();
    Mouse::Show(true);
    navdlg->Show();
  }
}

void PlanScreen::HideNavDlg()
{
  if (navdlg && navdlg->IsShown())
  {
    HideAll();
    Mouse::Show(true);
  }
}

bool PlanScreen::IsNavShown() { return navdlg && navdlg->IsShown(); }

void PlanScreen::ShowDebriefDlg()
{
  HideAll();
  Mouse::Show(true);
  debrief_dlg->Show();
}

void PlanScreen::HideDebriefDlg()
{
  HideAll();
  Mouse::Show(true);
}

bool PlanScreen::IsDebriefShown() { return debrief_dlg && debrief_dlg->IsShown(); }

void PlanScreen::ShowAwardDlg()
{
  HideAll();
  Mouse::Show(true);
  award_dlg->Show();
}

void PlanScreen::HideAwardDlg()
{
  HideAll();
  Mouse::Show(true);
}

bool PlanScreen::IsAwardShown() { return award_dlg && award_dlg->IsShown(); }

void PlanScreen::HideAll()
{
  if (objdlg)
    objdlg->Hide();
  if (pkgdlg)
    pkgdlg->Hide();
  if (wepdlg)
    wepdlg->Hide();
  if (navdlg)
    navdlg->Hide();
  if (award_dlg)
    award_dlg->Hide();
  if (debrief_dlg)
    debrief_dlg->Hide();
}
