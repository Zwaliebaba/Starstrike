#ifndef ListBox_h
#define ListBox_h

#include "List.h"
#include "ScrollWindow.h"

class Bitmap;
class ListBox;
class ListBoxCell;
class ListBoxItem;
class ListBoxColumn;

class ListBox : public ScrollWindow
{
  public:
    enum SORT
    {
      LIST_SORT_NUMERIC_DESCENDING = -2,
      LIST_SORT_ALPHA_DESCENDING,
      LIST_SORT_NONE,
      LIST_SORT_ALPHA_ASCENDING,
      LIST_SORT_NUMERIC_ASCENDING,
      LIST_SORT_NEVER
    };

    enum ALIGN
    {
      LIST_ALIGN_LEFT   = DT_LEFT,
      LIST_ALIGN_CENTER = DT_CENTER,
      LIST_ALIGN_RIGHT  = DT_RIGHT
    };

    enum STYLE
    {
      LIST_ITEM_STYLE_PLAIN,
      LIST_ITEM_STYLE_BOX,
      LIST_ITEM_STYLE_FILLED_BOX
    };

    ListBox(ActiveWindow* p, int ax, int ay, int aw, int ah, DWORD aid);
    ListBox(Screen* s, int ax, int ay, int aw, int ah, DWORD aid);
    ~ListBox() override;

    // Operations:
    void DrawContent(const Rect& ctrl_rect) override;

    // Event Target Interface:
    int OnMouseMove(int x, int y) override;
    int OnLButtonDown(int x, int y) override;
    int OnLButtonUp(int x, int y) override;
    int OnMouseWheel(int wheel) override;
    int OnClick() override;

    int OnKeyDown(int vk, int flags) override;

    // pseudo-events:
    int OnDragStart(int x, int y) override;
    int OnDragDrop(int x, int y, ActiveWindow* source) override;

    // Property accessors:
    int NumItems() const;
    int NumColumns() const;

    std::string GetItemText(int index);
    void SetItemText(int _index, std::string_view _text);
    uint64_t GetItemData(int index);
    void SetItemData(int index, DWORD data);
    Bitmap* GetItemImage(int index);
    void SetItemImage(int index, Bitmap* img);
    Color GetItemColor(int index);
    void SetItemColor(int index, Color c);

    std::string GetItemText(int index, int column);
    void SetItemText(int _index, int _column, std::string_view _text);
    uint64_t GetItemData(int index, int column);
    void SetItemData(int index, int column, DWORD data);
    Bitmap* GetItemImage(int index, int column);
    void SetItemImage(int index, int column, Bitmap* img);

    int AddItem(std::string_view text);
    int AddImage(Bitmap* img);
    int AddItemWithData(std::string_view text, uint64_t data);
    void InsertItem(int index, std::string_view text);
    void InsertItemWithData(int index, std::string_view text, uint64_t data);
    void ClearItems();
    void RemoveItem(int index);
    void RemoveSelectedItems();

    void AddColumn(std::string_view title, int width, int align = LIST_ALIGN_LEFT, int sort = LIST_SORT_NONE);

    std::string GetColumnTitle(int index);
    void SetColumnTitle(int index, std::string_view title);
    int GetColumnWidth(int index);
    void SetColumnWidth(int index, int width);
    int GetColumnAlign(int index);
    void SetColumnAlign(int index, int align);
    int GetColumnSort(int index);
    void SetColumnSort(int index, int sort);
    Color GetColumnColor(int index);
    void SetColumnColor(int index, Color c);

    Color GetItemColor(int index, int column);

    int GetMultiSelect();
    void SetMultiSelect(int nNewValue);
    bool GetShowHeadings();
    void SetShowHeadings(bool nNewValue);
    Color GetSelectedColor();
    void SetSelectedColor(Color c);

    int GetItemStyle() const;
    void SetItemStyle(int style);
    int GetSelectedStyle() const;
    void SetSelectedStyle(int style);

    bool IsSelected(int index);
    void SetSelected(int index, bool bNewValue = true);
    void ClearSelection();

    int GetSortColumn();
    void SetSortColumn(int col_index);
    int GetSortCriteria();
    void SetSortCriteria(SORT sort);
    void SortItems();
    void SizeColumns();

    // read-only:
    virtual int GetListIndex();
    int GetLineCount() override;
    virtual int GetSelCount();
    virtual int GetSelection();
    virtual std::string GetSelectedItem();

  protected:
    int IndexFromPoint(int x, int y) const;

    // properties:
    List<ListBoxItem> items;
    List<ListBoxColumn> columns;

    bool show_headings;
    int multiselect;
    int list_index;
    int selcount;

    Color selected_color;

    int sort_column;
    int item_style;
    int seln_style;
};

#endif ListBox_h
