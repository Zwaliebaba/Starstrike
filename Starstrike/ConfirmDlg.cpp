#include "pch.h"
#include "ConfirmDlg.h"
#include "Button.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(ConfirmDlg, OnApply);
DEF_MAP_CLIENT(ConfirmDlg, OnCancel);

ConfirmDlg::ConfirmDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    btn_apply(nullptr),
    btn_cancel(nullptr),
    parent_control(nullptr) { Init(def); }

ConfirmDlg::~ConfirmDlg() {}

void ConfirmDlg::RegisterControls()
{
  if (btn_apply)
    return;

  btn_apply = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, btn_apply, ConfirmDlg, OnApply);

  btn_cancel = static_cast<Button*>(FindControl(2));
  REGISTER_CLIENT(EID_CLICK, btn_cancel, ConfirmDlg, OnCancel);

  lbl_title = FindControl(100);
  lbl_message = FindControl(101);
}

ActiveWindow* ConfirmDlg::GetParentControl() { return parent_control; }

void ConfirmDlg::SetParentControl(ActiveWindow* p) { parent_control = p; }

std::string ConfirmDlg::GetTitle()
{
  if (lbl_title)
    return lbl_title->GetText();

  return "";
}

void ConfirmDlg::SetTitle(std::string_view t)
{
  if (lbl_title)
    lbl_title->SetText(t);
}

std::string ConfirmDlg::GetMessage()
{
  if (lbl_message)
    return lbl_message->GetText();

  return "";
}

void ConfirmDlg::SetMessage(std::string_view m)
{
  if (lbl_message)
    lbl_message->SetText(m);
}

void ConfirmDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
    OnApply(nullptr);

  if (Keyboard::KeyDown(VK_ESCAPE))
    OnCancel(nullptr);
}

void ConfirmDlg::Show()
{
  if (!IsShown())
    Button::PlaySound(Button::SND_CONFIRM);

  FormWindow::Show();
  SetFocus();
}

void ConfirmDlg::OnApply(AWEvent* event)
{
  manager->HideConfirmDlg();

  if (parent_control)
    parent_control->ClientEvent(EID_USER_1);
}

void ConfirmDlg::OnCancel(AWEvent* event) { manager->HideConfirmDlg(); }
