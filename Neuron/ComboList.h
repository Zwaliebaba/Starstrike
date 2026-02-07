#ifndef ComboList_h
#define ComboList_h

#include "List.h"
#include "ScrollWindow.h"


class ComboBox;

class ComboList : public ScrollWindow
{
  public:
    
    ComboList(ComboBox* ctrl, ActiveWindow* p, int ax, int ay, int aw, int ah, int maxentries);
    ComboList(ComboBox* ctrl, Screen* s, int ax, int ay, int aw, int ah, int maxentries);
    ~ComboList() override;

    // Operations:
    void Draw() override;
    void Show() override;
    void Hide() override;

    // Event Target Interface:
    int OnMouseMove(int x, int y) override;
    int OnLButtonDown(int x, int y) override;
    int OnLButtonUp(int x, int y) override;
    int OnClick() override;
    int OnMouseEnter(int x, int y) override;
    int OnMouseExit(int x, int y) override;
    void KillFocus() override;

    // Property accessors:
    virtual void ClearItems();
    virtual void AddItem(const std::string& item);
    virtual void AddItems(std::vector<std::string>& item_list);
    virtual void SetItems(std::vector<std::string>& item_list);
    virtual std::string GetItem(int index);
    virtual void SetItem(int index, const std::string& item);
    virtual size_t GetCount() { return items.size(); }
    virtual std::string GetSelectedItem();
    virtual int GetSelectedIndex();
    virtual void SetSelection(int index);

  protected:
    void DrawRectSimple(const Rect& rect, int stat);
    void DrawItem(const std::string& label, const Rect& btn_rect, int state);
    Rect CalcLabelRect(const Rect& btn_rect);
    int CalcSeln(int x, int y);
    void CopyStyle(const ComboBox& ctrl);

    ComboBox* combo_box;
    std::vector<std::string> items;
    bool animated;
    bool border;
    int seln;
    int captured;
    int bevel_width;
    int button_state;
    int button_height;
    size_t max_entries;
    int scroll;
    int scrolling;

    Color active_color;
    Color border_color;
};

#endif ComboList_h
