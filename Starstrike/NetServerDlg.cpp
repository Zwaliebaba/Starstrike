#include "pch.h"
#include "NetServerDlg.h"
#include "Button.h"
#include "ComboBox.h"
#include "EditBox.h"
#include "MenuScreen.h"
#include "NetAddr.h"
#include "NetServerConfig.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(NetServerDlg, OnApply);
DEF_MAP_CLIENT(NetServerDlg, OnCancel);

NetServerDlg::NetServerDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr)
{
  config = NetServerConfig::GetInstance();
  Init(def);
}

NetServerDlg::~NetServerDlg() {}

void NetServerDlg::RegisterControls()
{
  edt_name = static_cast<EditBox*>(FindControl(200));
  cmb_type = static_cast<ComboBox*>(FindControl(201));
  edt_game_port = static_cast<EditBox*>(FindControl(202));
  edt_admin_port = static_cast<EditBox*>(FindControl(203));
  edt_game_pass = static_cast<EditBox*>(FindControl(204));
  edt_admin_name = static_cast<EditBox*>(FindControl(205));
  edt_admin_pass = static_cast<EditBox*>(FindControl(206));

  btn_apply = static_cast<Button*>(FindControl(1));
  btn_cancel = static_cast<Button*>(FindControl(2));

  REGISTER_CLIENT(EID_CLICK, btn_apply, NetServerDlg, OnApply);
  REGISTER_CLIENT(EID_CLICK, btn_cancel, NetServerDlg, OnCancel);
}

void NetServerDlg::Show()
{
  if (!IsShown())
    FormWindow::Show();

  NetServerConfig::Initialize();
  config = NetServerConfig::GetInstance();

  if (config)
  {
    config->Load();

    char buff[32];

    if (edt_name)
    {
      edt_name->SetText(config->Name());
      edt_name->SetFocus();
    }

    if (cmb_type)
      cmb_type->SetSelection(config->GetGameType());

    if (edt_game_port)
    {
      sprintf_s(buff, "%d", config->GetLobbyPort());
      edt_game_port->SetText(buff);
    }

    if (edt_admin_port)
    {
      sprintf_s(buff, "%d", config->GetAdminPort());
      edt_admin_port->SetText(buff);
    }

    if (edt_game_pass)
      edt_game_pass->SetText(config->GetGamePass());

    if (edt_admin_name)
      edt_admin_name->SetText(config->GetAdminName());

    if (edt_admin_pass)
      edt_admin_pass->SetText(config->GetAdminPass());
  }
}

void NetServerDlg::ExecFrame() {}

void NetServerDlg::OnApply(AWEvent* event)
{
  if (config)
  {
    if (edt_name)
      config->SetName(edt_name->GetText());

    if (cmb_type)
      config->SetGameType(cmb_type->GetSelectedIndex());

    if (edt_game_port)
    {
      int port = 0;
      sscanf_s(edt_game_port->GetText().c_str(), "%d", &port);
      config->SetLobbyPort(static_cast<WORD>(port));
      config->SetGamePort(static_cast<WORD>(port) + 1);
    }

    if (edt_admin_port)
    {
      int port = 0;
      sscanf_s(edt_admin_port->GetText().c_str(), "%d", &port);
      config->SetAdminPort(static_cast<WORD>(port));
    }

    if (edt_game_pass)
      config->SetGamePass(edt_game_pass->GetText());

    if (edt_admin_name)
      config->SetAdminName(edt_admin_name->GetText());

    if (edt_admin_pass)
      config->SetAdminPass(edt_admin_pass->GetText());

    config->Save();
  }

  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    ::DebugTrace("\nSTART LOCAL SERVER\n\n");
    stars->SetLobbyMode(Starshatter::NET_LOBBY_SERVER);
    manager->ShowNetLobbyDlg();
  }
  else
    manager->ShowMenuDlg();
}

void NetServerDlg::OnCancel(AWEvent* event)
{
  NetServerConfig::Close();
  manager->ShowNetClientDlg();
}
