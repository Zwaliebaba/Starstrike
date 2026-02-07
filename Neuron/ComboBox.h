#pragma once

#include "ActiveWindow.h"

#undef SetForm

class ComboList;

class ComboBox : public ActiveWindow
{
  public:
    ComboBox(ActiveWindow* p, int ax, int ay, int aw, int ah, DWORD aid = 0);
    ComboBox(Screen* s, int ax, int ay, int aw, int ah, DWORD aid = 0);
    ~ComboBox() override;

    // Operations:
    void Draw() override;
    virtual void ShowList();
    virtual void HideList();
    void MoveTo(const Rect& r) override;

    // Event Target Interface:
    int OnMouseMove(int x, int y) override;
    int OnLButtonDown(int x, int y) override;
    int OnLButtonUp(int x, int y) override;
    int OnClick() override;
    int OnMouseEnter(int x, int y) override;
    int OnMouseExit(int x, int y) override;

    virtual void OnListSelect(AWEvent* event);
    virtual void OnListExit(AWEvent* event);

    // Property accessors:
    virtual int NumItems();
    virtual void ClearItems();
    virtual void AddItem(std::string_view item);
    virtual std::string GetItem(int index);
    virtual void SetItem(int index, const std::string& item);
    virtual void SetLabel(std::string_view label);

    virtual int GetCount();
    virtual std::string GetSelectedItem();
    virtual int GetSelectedIndex();
    virtual void SetSelection(int index);

    Color GetActiveColor() const { return active_color; }
    void SetActiveColor(const Color& _newValue);
    bool GetAnimated() const { return animated; }
    void SetAnimated(bool bNewValue);
    short GetBevelWidth() const { return bevel_width; }
    void SetBevelWidth(short nNewValue);
    bool GetBorder() const { return border; }
    void SetBorder(bool bNewValue);
    Color GetBorderColor() const { return border_color; }
    void SetBorderColor(const Color& _newValue);
    short GetButtonState() const { return button_state; }
    void SetButtonState(short nNewValue);

    bool IsListShowing() const { return list_showing; }

  protected:
    Rect CalcLabelRect();
    void DrawRectSimple(Rect& rect, int stat);

    std::vector<std::string> items;
    ComboList* list;
    bool list_showing;
    bool animated;
    bool border;
    int seln;
    int captured;
    int bevel_width;
    int button_state;
    int pre_state;

    Color active_color;
    Color border_color;
};
