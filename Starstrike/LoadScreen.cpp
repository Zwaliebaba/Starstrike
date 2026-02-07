#include "pch.h"
#include "LoadScreen.h"
#include "ActiveWindow.h"
#include "CmpLoadDlg.h"
#include "Color.h"
#include "DataLoader.h"
#include "FormDef.h"
#include "Game.h"
#include "LoadDlg.h"
#include "Mouse.h"
#include "Screen.h"
#include "Starshatter.h"
#include "Window.h"

LoadScreen::LoadScreen()
  : screen(nullptr),
    load_dlg(nullptr),
    cmp_load_dlg(nullptr),
    isShown(false) {}

LoadScreen::~LoadScreen() { TearDown(); }

void LoadScreen::Setup(Screen* s)
{
  if (!s)
    return;

  screen = s;

  DataLoader* loader = DataLoader::GetLoader();

  // create windows
  FormDef load_def("LoadDlg", 0);
  load_def.Load("LoadDlg");
  load_dlg = NEW LoadDlg(screen, load_def);

  FormDef cmp_load_def("CmpLoadDlg", 0);
  cmp_load_def.Load("CmpLoadDlg");
  cmp_load_dlg = NEW CmpLoadDlg(screen, cmp_load_def);

  ShowLoadDlg();
}

void LoadScreen::TearDown()
{
  if (screen)
  {
    if (load_dlg)
      screen->DelWindow(load_dlg);
    if (cmp_load_dlg)
      screen->DelWindow(cmp_load_dlg);
  }

  delete load_dlg;
  delete cmp_load_dlg;

  load_dlg = nullptr;
  cmp_load_dlg = nullptr;
  screen = nullptr;
}

void LoadScreen::ExecFrame()
{
  Game::SetScreenColor(Color::Black);

  if (load_dlg && load_dlg->IsShown())
    load_dlg->ExecFrame();

  if (cmp_load_dlg && cmp_load_dlg->IsShown())
    cmp_load_dlg->ExecFrame();
}

bool LoadScreen::CloseTopmost() { return false; }

void LoadScreen::Show()
{
  if (!isShown)
  {
    ShowLoadDlg();
    isShown = true;
  }
}

void LoadScreen::Hide()
{
  if (isShown)
  {
    HideLoadDlg();
    isShown = false;
  }
}

void LoadScreen::ShowLoadDlg()
{
  if (load_dlg)
    load_dlg->Hide();
  if (cmp_load_dlg)
    cmp_load_dlg->Hide();

  Starshatter* stars = Starshatter::GetInstance();

  // show campaign load dialog if available and loading campaign
  if (stars && cmp_load_dlg)
  {
    if (stars->GetGameMode() == Starshatter::CLOD_MODE || stars->GetGameMode() == Starshatter::CMPN_MODE)
    {
      cmp_load_dlg->Show();
      Mouse::Show(false);
      return;
    }
  }

  // otherwise, show regular load dialog
  if (load_dlg)
  {
    load_dlg->Show();
    Mouse::Show(false);
  }
}

void LoadScreen::HideLoadDlg()
{
  if (load_dlg && load_dlg->IsShown())
    load_dlg->Hide();

  if (cmp_load_dlg && cmp_load_dlg->IsShown())
    cmp_load_dlg->Hide();
}
