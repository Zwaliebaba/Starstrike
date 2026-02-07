#include "pch.h"
#include "NetAddrDlg.h"
#include "Button.h"
#include "EditBox.h"
#include "FormatUtil.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "NetClientConfig.h"
#include "Screen.h"

DEF_MAP_CLIENT(NetAddrDlg, OnSave);
DEF_MAP_CLIENT(NetAddrDlg, OnCancel);

NetAddrDlg::NetAddrDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    btn_save(nullptr),
    btn_cancel(nullptr),
    edt_name(nullptr),
    edt_addr(nullptr),
    edt_port(nullptr),
    edt_pass(nullptr) { Init(def); }

NetAddrDlg::~NetAddrDlg() {}

void NetAddrDlg::RegisterControls()
{
  btn_save = static_cast<Button*>(FindControl(1));
  btn_cancel = static_cast<Button*>(FindControl(2));

  REGISTER_CLIENT(EID_CLICK, btn_save, NetAddrDlg, OnSave);
  REGISTER_CLIENT(EID_CLICK, btn_cancel, NetAddrDlg, OnCancel);

  edt_name = static_cast<EditBox*>(FindControl(200));
  edt_addr = static_cast<EditBox*>(FindControl(201));
  edt_port = static_cast<EditBox*>(FindControl(202));
  edt_pass = static_cast<EditBox*>(FindControl(203));

  if (edt_name)
    edt_name->SetText("");
  if (edt_addr)
    edt_addr->SetText("");
  if (edt_port)
    edt_port->SetText("");
  if (edt_pass)
    edt_pass->SetText("");
}

void NetAddrDlg::Show()
{
  if (!IsShown())
  {
    FormWindow::Show();

    if (edt_name)
      edt_name->SetText("");
    if (edt_addr)
      edt_addr->SetText("");
    if (edt_port)
      edt_port->SetText("");
    if (edt_pass)
      edt_pass->SetText("");

    if (edt_name)
      edt_name->SetFocus();
  }
}

static bool tab_latch = false;

void NetAddrDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
    OnSave(nullptr);
}

void NetAddrDlg::OnSave(AWEvent* event)
{
  NetClientConfig* config = NetClientConfig::GetInstance();

  if (config && edt_addr && edt_addr->GetText().length() > 0 && edt_port && edt_port->GetText().length() > 0)
  {
    std::string name;
    std::string addr;
    std::string pass;
    int port;

    sscanf_s(edt_port->GetText().data(), "%d", &port);

    if (edt_name && edt_name->GetText().length() < 250)
    {
      char buffer[256];
      strcpy_s(buffer, edt_name->GetText().data());
      char* p = strpbrk(buffer, "\n\r\t");
      if (p)
        *p = 0;

      name = SafeQuotes(buffer);
    }

    if (edt_pass && edt_pass->GetText().length() < 250)
    {
      char buffer[256];
      strcpy_s(buffer, edt_pass->GetText().data());
      char* p = strpbrk(buffer, "\n\r\t");
      if (p)
        *p = 0;

      pass = SafeQuotes(buffer);
    }

    if (edt_addr && edt_addr->GetText().length() < 250)
    {
      char buffer[256];
      strcpy_s(buffer, edt_addr->GetText().data());
      char* p = strpbrk(buffer, "\n\r\t");
      if (p)
        *p = 0;

      addr = SafeQuotes(buffer);
    }

    config->AddServer(name, addr, port, pass, true);
    config->Save();
  }

  if (manager)
    manager->ShowNetClientDlg();
}

void NetAddrDlg::OnCancel(AWEvent* event)
{
  if (manager)
    manager->ShowNetClientDlg();
}
