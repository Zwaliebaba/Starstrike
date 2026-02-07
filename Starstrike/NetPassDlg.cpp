#include "pch.h"
#include "NetPassDlg.h"
#include "Button.h"
#include "EditBox.h"
#include "FormatUtil.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "NetClientConfig.h"
#include "NetLobby.h"
#include "Screen.h"

DEF_MAP_CLIENT(NetPassDlg, OnApply);
DEF_MAP_CLIENT(NetPassDlg, OnCancel);

NetPassDlg::NetPassDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    btn_apply(nullptr),
    btn_cancel(nullptr),
    edt_pass(nullptr),
    lbl_name(nullptr) { Init(def); }

NetPassDlg::~NetPassDlg() {}

void NetPassDlg::RegisterControls()
{
  btn_apply = static_cast<Button*>(FindControl(1));
  btn_cancel = static_cast<Button*>(FindControl(2));

  REGISTER_CLIENT(EID_CLICK, btn_apply, NetPassDlg, OnApply);
  REGISTER_CLIENT(EID_CLICK, btn_cancel, NetPassDlg, OnCancel);

  lbl_name = FindControl(110);
  edt_pass = static_cast<EditBox*>(FindControl(200));

  if (edt_pass)
    edt_pass->SetText("");
}

void NetPassDlg::Show()
{
  if (!IsShown())
  {
    FormWindow::Show();

    NetClientConfig* config = NetClientConfig::GetInstance();

    if (config && lbl_name)
    {
      NetServerInfo* info = config->GetSelectedServer();

      if (info)
        lbl_name->SetText(info->name);
    }

    if (edt_pass)
    {
      edt_pass->SetText("");
      edt_pass->SetFocus();
    }
  }
}

static bool tab_latch = false;

void NetPassDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
    OnApply(nullptr);
}

void NetPassDlg::OnApply(AWEvent* event)
{
  NetClientConfig* config = NetClientConfig::GetInstance();

  if (config && edt_pass)
  {
    NetServerInfo* info = config->GetSelectedServer();

    if (info && edt_pass->GetText().length() < 250)
    {
      char buffer[256];
      strcpy_s(buffer, edt_pass->GetText().data());

      // trim from first occurrence of invalid character
      char* p = strpbrk(buffer, "\n\r\t");
      if (p)
        *p = 0;

      info->password = SafeQuotes(buffer);

      if (manager)
      {
        manager->ShowNetLobbyDlg();
        return;
      }
    }
  }

  if (manager)
    manager->ShowNetClientDlg();
}

void NetPassDlg::OnCancel(AWEvent* event)
{
  if (manager)
    manager->ShowNetClientDlg();
}
