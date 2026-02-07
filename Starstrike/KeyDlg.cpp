#include "pch.h"
#include "KeyDlg.h"
#include "Button.h"
#include "KeyMap.h"
#include "MenuScreen.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(KeyDlg, OnApply);
DEF_MAP_CLIENT(KeyDlg, OnCancel);
DEF_MAP_CLIENT(KeyDlg, OnClear);

KeyDlg::KeyDlg(Screen* s, FormDef& def, BaseScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    key_key(0),
    key_shift(0),
    key_clear(0),
    clear(nullptr),
    apply(nullptr),
    cancel(nullptr),
    command(nullptr),
    current_key(nullptr),
    new_key(nullptr) { Init(def); }

KeyDlg::~KeyDlg() {}

void KeyDlg::RegisterControls()
{
  if (apply)
    return;

  command = FindControl(201);
  current_key = FindControl(202);
  new_key = FindControl(203);

  clear = static_cast<Button*>(FindControl(300));
  REGISTER_CLIENT(EID_CLICK, clear, KeyDlg, OnClear);

  apply = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, apply, KeyDlg, OnApply);

  cancel = static_cast<Button*>(FindControl(2));
  REGISTER_CLIENT(EID_CLICK, cancel, KeyDlg, OnCancel);
}

void KeyDlg::ExecFrame()
{
  int key = 0;
  int shift = 0;

  for (int i = 0; i < 256; i++)
  {
    int vk = KeyMap::GetMappableVKey(i);

    if (GetAsyncKeyState(vk))
    {
      if (vk == VK_SHIFT || vk == VK_MENU)
        shift = vk;
      else
        key = vk;
    }
  }

  if (key)
  {
    key_key = key;
    key_shift = shift;

    new_key->SetText(KeyMap::DescribeKey(key, shift));
  }
}

void KeyDlg::Show()
{
  FormWindow::Show();

  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    KeyMap& keymap = stars->GetKeyMap();

    if (command)
      command->SetText(keymap.DescribeAction(key_index));

    if (current_key)
      current_key->SetText(keymap.DescribeKey(key_index));
  }

  key_clear = false;
  new_key->SetText("");
  SetFocus();
}

void KeyDlg::SetKeyMapIndex(int i)
{
  key_index = i;
  key_key = 0;
  key_shift = 0;
}

void KeyDlg::OnClear(AWEvent* event)
{
  key_clear = true;

  key_key = 0;
  key_shift = 0;
}

void KeyDlg::OnApply(AWEvent* event)
{
  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    KeyMap& keymap = stars->GetKeyMap();
    KeyMapEntry* map = keymap.GetKeyMap(key_index);

    if (key_clear)
    {
      map->key = 0;
      map->alt = 0;
    }

    if (key_key)
    {
      map->key = key_key;
      map->alt = key_shift;
    }
  }

  if (manager)
    manager->ShowCtlDlg();
}

void KeyDlg::OnCancel(AWEvent* event)
{
  if (manager)
    manager->ShowCtlDlg();
}
