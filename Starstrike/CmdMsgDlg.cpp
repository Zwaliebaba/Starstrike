#include "pch.h"
#include "CmdMsgDlg.h"
#include "Button.h"
#include "CmpnScreen.h"
#include "Keyboard.h"

DEF_MAP_CLIENT(CmdMsgDlg, OnApply);

CmdMsgDlg::CmdMsgDlg(Screen* s, FormDef& def, CmpnScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    exit_latch(false) { Init(def); }

CmdMsgDlg::~CmdMsgDlg() {}

void CmdMsgDlg::RegisterControls()
{
  title = FindControl(100);
  message = FindControl(101);

  apply = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, apply, CmdMsgDlg, OnApply);
}

void CmdMsgDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
    OnApply(nullptr);

  if (Keyboard::KeyDown(VK_ESCAPE))
  {
    if (!exit_latch)
      OnApply(nullptr);

    exit_latch = true;
  }
  else
    exit_latch = false;
}

void CmdMsgDlg::Show()
{
  FormWindow::Show();
  SetFocus();
}

void CmdMsgDlg::OnApply(AWEvent* event)
{
  if (manager)
    manager->CloseTopmost();
}
