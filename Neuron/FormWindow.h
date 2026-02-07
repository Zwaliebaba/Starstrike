#pragma once

#include "ActiveWindow.h"
#include "Button.h"
#include "ComboList.h"
#include "EditBox.h"
#include "FormDef.h"
#include "ImageBox.h"
#include "List.h"
#include "ListBox.h"
#include "RichTextBox.h"
#include "Slider.h"

class FormWindow : public ActiveWindow
{
  public:
    FormWindow(Screen* s, int ax, int ay, int aw, int ah, DWORD aid = 0, DWORD style = 0,
               ActiveWindow* parent = nullptr);
    ~FormWindow() override;

    // operations:
    virtual void Init();
    virtual void Init(const FormDef& def);
    virtual void Destroy();
    virtual ActiveWindow* FindControl(DWORD id);
    ActiveWindow* FindControl(int x, int y) override;
    virtual void RegisterControls() {}

    virtual void AdoptFormDef(const FormDef& def);
    virtual void AddControl(ActiveWindow* ctrl);

    virtual ActiveWindow* CreateLabel(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0,
                                      DWORD style = 0);
    virtual Button* CreateButton(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0);
    virtual ImageBox* CreateImageBox(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0);
    virtual ListBox* CreateListBox(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0);
    virtual ComboBox* CreateComboBox(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0);
    virtual EditBox* CreateEditBox(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0);
    virtual RichTextBox* CreateRichTextBox(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0,
                                           DWORD style = 0);
    virtual Slider* CreateSlider(std::string_view text, int x, int y, int w, int h, DWORD id = 0, DWORD pid = 0,
                                 DWORD style = 0);

    // property accessors:
    ListIter<ActiveWindow> Controls() { return children; }

  protected:
    virtual void CreateDefLabel(CtrlDef& def);
    virtual void CreateDefButton(CtrlDef& def);
    virtual void CreateDefImage(CtrlDef& def);
    virtual void CreateDefList(CtrlDef& def);
    virtual void CreateDefCombo(CtrlDef& def);
    virtual void CreateDefEdit(CtrlDef& def);
    virtual void CreateDefSlider(CtrlDef& def);
    virtual void CreateDefRichText(CtrlDef& def);
};
