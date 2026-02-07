#include "pch.h"
#include "Menu.h"

void Menu::AddItem(std::string label, DWORD value, bool enabled)
{
  auto item = NEW MenuItem(label, value, enabled);

  if (item)
  {
    item->menu = this;
    items.append(item);
  }
}

void Menu::AddItem(MenuItem* item)
{
  if (item->submenu)
    item->submenu->SetParent(this);
  item->menu = this;
  items.append(item);
}

void Menu::AddMenu(std::string label, Menu* menu, DWORD value)
{
  auto item = NEW MenuItem(label, value);

  if (item)
  {
    item->menu = this;
    item->submenu = menu;
    menu->parent = this;

    items.append(item);
  }
}

MenuItem* Menu::GetItem(int index)
{
  if (index >= 0 && index < items.size())
    return items[index];

  return nullptr;
}

void Menu::SetItem(int index, MenuItem* item)
{
  if (item && index >= 0 && index < items.size())
    items[index] = item;
}

int Menu::NumItems() const { return items.size(); }

void Menu::ClearItems() { items.destroy(); }

MenuItem::MenuItem(std::string_view label, DWORD value, bool e)
  : text(label),
    data(value),
    enabled(e),
    selected(0),
    submenu(nullptr) {}

MenuItem::~MenuItem() {}

Menu* MenuHistory::GetCurrent()
{
  int n = history.size();

  if (n)
    return history[n - 1];

  return nullptr;
}

Menu* MenuHistory::GetLevel(int n)
{
  if (n >= 0 && n < history.size())
    return history[n];

  return nullptr;
}

Menu* MenuHistory::Find(std::string_view title)
{
  for (int i = 0; i < history.size(); i++)
  {
    if (history[i]->GetTitle() == title)
      return history[i];
  }

  return nullptr;
}

void MenuHistory::Pop()
{
  int n = history.size();

  if (n)
    history.removeIndex(n - 1);
}

void MenuHistory::Push(Menu* menu) { history.append(menu); }

void MenuHistory::Clear() { history.clear(); }
