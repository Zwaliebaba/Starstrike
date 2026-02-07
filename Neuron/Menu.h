#ifndef Menu_h
#define Menu_h

#include "List.h"

class Menu;
class MenuItem;
class MenuHistory;

class Menu
{
  public:
    Menu() = default;

    Menu(std::string t)
      : title(std::move(t)) {}

    virtual ~Menu() { items.destroy(); }

    virtual std::string GetTitle() const { return title; }
    virtual void SetTitle(std::string t) { title = t; }
    virtual Menu* GetParent() const { return parent; }
    virtual void SetParent(Menu* p) { parent = p; }

    virtual void AddItem(std::string label, DWORD value = 0, bool enabled = true);
    virtual void AddItem(MenuItem* item);
    virtual void AddMenu(std::string label, Menu* menu, DWORD value = 0);
    virtual MenuItem* GetItem(int index);
    virtual void SetItem(int index, MenuItem* item);
    virtual int NumItems() const;
    virtual void ClearItems();

    ListIter<MenuItem> GetItems() { return items; }

  protected:
    std::string title;
    List<MenuItem> items;
    Menu* parent = {};

    friend class MenuItem;
};

class MenuItem
{
  public:
    MenuItem(std::string_view label, DWORD value = 0, bool enabled = true);
    virtual ~MenuItem();

    virtual std::string GetText() const { return text; }
    virtual void SetText(std::string t) { text = t; }

    virtual uint64_t GetData() const { return data; }
    virtual void SetData(uint64_t d) { data = d; }

    virtual int GetEnabled() const { return enabled; }
    virtual void SetEnabled(int e) { enabled = e; }

    virtual int GetSelected() const { return selected; }
    virtual void SetSelected(int s) { selected = s; }

    virtual Menu* GetMenu() const { return menu; }
    virtual void SetMenu(Menu* m) { menu = m; }

    virtual Menu* GetSubmenu() const { return submenu; }
    virtual void SetSubmenu(Menu* s) { submenu = s; }

  protected:
    std::string text;
    uint64_t data;
    int enabled;
    int selected;

    Menu* menu;
    Menu* submenu;

    friend class Menu;
};

class MenuHistory
{
  public:
    MenuHistory() {}
    virtual ~MenuHistory() { history.clear(); }

    virtual Menu* GetCurrent();
    virtual Menu* GetLevel(int n);
    virtual Menu* Find(std::string_view title);
    virtual void Pop();
    virtual void Push(Menu* menu);
    virtual void Clear();

  private:
    List<Menu> history;
};

#endif Menu_h
