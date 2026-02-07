#include "pch.h"
#include "MsnNavDlg.h"
#include "Campaign.h"
#include "Keyboard.h"
#include "PlanScreen.h"

DEF_MAP_CLIENT(MsnNavDlg, OnCommit);
DEF_MAP_CLIENT(MsnNavDlg, OnCancel);
DEF_MAP_CLIENT(MsnNavDlg, OnTabButton);

MsnNavDlg::MsnNavDlg(Screen* s, FormDef& def, PlanScreen* mgr)
  : NavDlg(s, def, mgr),
    MsnDlg(mgr) { RegisterControls(); }

MsnNavDlg::~MsnNavDlg() {}

void MsnNavDlg::RegisterControls()
{
  RegisterMsnControls(this);

  if (commit)
    REGISTER_CLIENT(EID_CLICK, commit, MsnNavDlg, OnCommit);

  if (cancel)
    REGISTER_CLIENT(EID_CLICK, cancel, MsnNavDlg, OnCancel);

  if (sit_button)
    REGISTER_CLIENT(EID_CLICK, sit_button, MsnNavDlg, OnTabButton);

  if (pkg_button)
    REGISTER_CLIENT(EID_CLICK, pkg_button, MsnNavDlg, OnTabButton);

  if (nav_button)
    REGISTER_CLIENT(EID_CLICK, nav_button, MsnNavDlg, OnTabButton);

  if (wep_button)
    REGISTER_CLIENT(EID_CLICK, wep_button, MsnNavDlg, OnTabButton);
}

void MsnNavDlg::Show()
{
  NavDlg::Show();
  ShowMsnDlg();
}

void MsnNavDlg::ExecFrame()
{
  NavDlg::ExecFrame();

  if (Keyboard::KeyDown(VK_RETURN))
    OnCommit(nullptr);
}

void MsnNavDlg::OnCommit(AWEvent* event)
{
  MsnDlg::OnCommit(event);
  NavDlg::OnCommit(event);
}

void MsnNavDlg::OnCancel(AWEvent* event)
{
  MsnDlg::OnCancel(event);
  NavDlg::OnCancel(event);
}
