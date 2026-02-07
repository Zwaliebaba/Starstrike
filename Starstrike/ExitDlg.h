#ifndef ExitDlg_h
#define ExitDlg_h

#include "Button.h"
#include "Font.h"
#include "FormWindow.h"

class MenuScreen;

class ExitDlg : public FormWindow
{
  public:
    ExitDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~ExitDlg() override;

    void RegisterControls() override;
    void Show() override;

    // Operations:
    virtual void ExecFrame();

    virtual void OnApply(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

  protected:
    MenuScreen* manager;

    RichTextBox* credits;
    Button* apply;
    Button* cancel;
    Rect def_rect;

    bool exit_latch;
};

#endif ExitDlg_h
