#include "pch.h"
#include "CtlDlg.h"
#include "Button.h"
#include "ComboBox.h"
#include "KeyDlg.h"
#include "Keyboard.h"
#include "ListBox.h"
#include "MenuScreen.h"
#include "Ship.h"
#include "Slider.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(CtlDlg, OnCommand);
DEF_MAP_CLIENT(CtlDlg, OnCategory);
DEF_MAP_CLIENT(CtlDlg, OnControlModel);
DEF_MAP_CLIENT(CtlDlg, OnMouseSelect);
DEF_MAP_CLIENT(CtlDlg, OnMouseSensitivity);
DEF_MAP_CLIENT(CtlDlg, OnMouseInvert);
DEF_MAP_CLIENT(CtlDlg, OnApply);
DEF_MAP_CLIENT(CtlDlg, OnCancel);
DEF_MAP_CLIENT(CtlDlg, OnAudio);
DEF_MAP_CLIENT(CtlDlg, OnVideo);
DEF_MAP_CLIENT(CtlDlg, OnOptions);
DEF_MAP_CLIENT(CtlDlg, OnControls);

CtlDlg::CtlDlg(Screen* s, FormDef& def, BaseScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    command_index(0),
    aud_btn(nullptr),
    vid_btn(nullptr),
    opt_btn(nullptr),
    ctl_btn(nullptr),
    apply(nullptr),
    cancel(nullptr),
    selected_category(0),
    control_model(0),
    mouse_select(0),
    mouse_sensitivity(0),
    mouse_invert(0),
    closed(true) { Init(def); }

CtlDlg::~CtlDlg() {}

void CtlDlg::RegisterControls()
{
  if (apply)
    return;

  for (int i = 0; i < 4; i++)
  {
    category[i] = static_cast<Button*>(FindControl(101 + i));
    REGISTER_CLIENT(EID_CLICK, category[i], CtlDlg, OnCategory);
  }

  commands = static_cast<ListBox*>(FindControl(200));
  REGISTER_CLIENT(EID_SELECT, commands, CtlDlg, OnCommand);

  control_model_combo = static_cast<ComboBox*>(FindControl(210));
  REGISTER_CLIENT(EID_SELECT, control_model_combo, CtlDlg, OnControlModel);

  mouse_select_combo = static_cast<ComboBox*>(FindControl(511));
  REGISTER_CLIENT(EID_SELECT, mouse_select_combo, CtlDlg, OnMouseSelect);

  mouse_sensitivity_slider = static_cast<Slider*>(FindControl(514));
  if (mouse_sensitivity_slider)
  {
    mouse_sensitivity_slider->SetRangeMin(0);
    mouse_sensitivity_slider->SetRangeMax(50);
    mouse_sensitivity_slider->SetStepSize(1);
    REGISTER_CLIENT(EID_CLICK, mouse_sensitivity_slider, CtlDlg, OnMouseSensitivity);
  }

  mouse_invert_checkbox = static_cast<Button*>(FindControl(515));
  REGISTER_CLIENT(EID_CLICK, mouse_invert_checkbox, CtlDlg, OnMouseInvert);

  apply = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, apply, CtlDlg, OnApply);

  cancel = static_cast<Button*>(FindControl(2));
  REGISTER_CLIENT(EID_CLICK, cancel, CtlDlg, OnCancel);

  vid_btn = static_cast<Button*>(FindControl(901));
  REGISTER_CLIENT(EID_CLICK, vid_btn, CtlDlg, OnVideo);

  aud_btn = static_cast<Button*>(FindControl(902));
  REGISTER_CLIENT(EID_CLICK, aud_btn, CtlDlg, OnAudio);

  ctl_btn = static_cast<Button*>(FindControl(903));
  REGISTER_CLIENT(EID_CLICK, ctl_btn, CtlDlg, OnControls);

  opt_btn = static_cast<Button*>(FindControl(904));
  REGISTER_CLIENT(EID_CLICK, opt_btn, CtlDlg, OnOptions);
}

void CtlDlg::Show()
{
  if (!IsShown())
    FormWindow::Show();

  if (closed)
    ShowCategory();
  else
    UpdateCategory();

  if (vid_btn)
    vid_btn->SetButtonState(0);
  if (aud_btn)
    aud_btn->SetButtonState(0);
  if (ctl_btn)
    ctl_btn->SetButtonState(1);
  if (opt_btn)
    opt_btn->SetButtonState(0);

  closed = false;
}

void CtlDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
    OnApply(nullptr);
}

void CtlDlg::ShowCategory()
{
  if (!commands || !category[0])
    return;

  for (int i = 0; i < 4; i++)
  {
    if (i == selected_category)
      category[i]->SetButtonState(1);
    else
      category[i]->SetButtonState(0);
  }

  commands->ClearItems();
  commands->SetSelectedStyle(ListBox::LIST_ITEM_STYLE_FILLED_BOX);
  commands->SetLeading(2);

  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    KeyMap& keymap = stars->GetKeyMap();
    int n = 0;

    for (int i = 0; i < 256; i++)
    {
      KeyMapEntry* k = keymap.GetKeyMap(i);

      switch (k->act)
      {
        case 0:
          break;

        case KEY_CONTROL_MODEL:
          control_model = k->key;
          control_model_combo->SetSelection(control_model);
          break;

        case KEY_MOUSE_SELECT:
          mouse_select = k->key;
          mouse_select_combo->SetSelection(mouse_select);
          break;

        case KEY_MOUSE_SENSE:
          mouse_sensitivity = k->key;
          mouse_sensitivity_slider->SetValue(mouse_sensitivity);
          break;

        case KEY_MOUSE_INVERT:
          mouse_invert = k->key;
          mouse_invert_checkbox->SetButtonState(mouse_invert > 0);
          break;

        case KEY_AXIS_YAW:
        case KEY_AXIS_PITCH:
        case KEY_AXIS_ROLL:
        case KEY_AXIS_THROTTLE:

        case KEY_AXIS_YAW_INVERT:
        case KEY_AXIS_PITCH_INVERT:
        case KEY_AXIS_ROLL_INVERT:
        case KEY_AXIS_THROTTLE_INVERT:
          break;

        default:
          if (keymap.GetCategory(i) == selected_category)
          {
            commands->AddItemWithData(keymap.DescribeAction(i), static_cast<DWORD>(i));
            commands->SetItemText(n++, 1, keymap.DescribeKey(i));
          }
          break;
      }
    }
  }
}

void CtlDlg::UpdateCategory()
{
  if (!commands)
    return;

  Starshatter* stars = Starshatter::GetInstance();
  KeyMap& keymap = stars->GetKeyMap();

  for (int i = 0; i < commands->NumItems(); i++)
  {
    int key = commands->GetItemData(i);
    commands->SetItemText(i, 1, keymap.DescribeKey(key));
  }
}

void CtlDlg::OnCategory(AWEvent* event)
{
  selected_category = 0;

  for (int i = 0; i < 4; i++)
  {
    if (event->window == category[i])
      selected_category = i;
  }

  ShowCategory();
}

static DWORD command_click_time = 0;

void CtlDlg::OnCommand(AWEvent* event)
{
  int list_index = commands->GetListIndex();

  // double-click:
  if (list_index == command_index && Game::RealTime() - command_click_time < 350)
  {
    KeyDlg* key_dlg = nullptr;

    if (manager)
      key_dlg = manager->GetKeyDlg();

    if (key_dlg)
      key_dlg->SetKeyMapIndex(commands->GetItemData(list_index));

    if (manager)
      manager->ShowKeyDlg();
  }

  command_click_time = Game::RealTime();
  command_index = list_index;
}

void CtlDlg::OnControlModel(AWEvent* event) { control_model = control_model_combo->GetSelectedIndex(); }

void CtlDlg::OnMouseSelect(AWEvent* event) { mouse_select = mouse_select_combo->GetSelectedIndex(); }

void CtlDlg::OnMouseSensitivity(AWEvent* event) { mouse_sensitivity = mouse_sensitivity_slider->GetValue(); }

void CtlDlg::OnMouseInvert(AWEvent* event) { mouse_invert = mouse_invert_checkbox->GetButtonState() > 0; }

void CtlDlg::OnAudio(AWEvent* event) { manager->ShowAudDlg(); }
void CtlDlg::OnVideo(AWEvent* event) { manager->ShowVidDlg(); }
void CtlDlg::OnOptions(AWEvent* event) { manager->ShowOptDlg(); }
void CtlDlg::OnControls(AWEvent* event) { manager->ShowCtlDlg(); }

void CtlDlg::OnApply(AWEvent* event)
{
  if (manager)
    manager->ApplyOptions();
}

void CtlDlg::OnCancel(AWEvent* event)
{
  if (manager)
    manager->CancelOptions();
}

void CtlDlg::Apply()
{
  if (closed)
    return;

  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    KeyMap& keymap = stars->GetKeyMap();

    keymap.Bind(KEY_CONTROL_MODEL, control_model, 0);

    keymap.Bind(KEY_MOUSE_SELECT, mouse_select, 0);
    keymap.Bind(KEY_MOUSE_SENSE, mouse_sensitivity, 0);
    keymap.Bind(KEY_MOUSE_INVERT, mouse_invert, 0);

    keymap.SaveKeyMap("key.cfg", 256);

    stars->MapKeys();
    Ship::SetControlModel(control_model);
  }

  closed = true;
}

void CtlDlg::Cancel() { closed = true; }
