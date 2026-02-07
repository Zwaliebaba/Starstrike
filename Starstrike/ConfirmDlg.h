#ifndef ConfirmDlg_h
#define ConfirmDlg_h

#include "FormWindow.h"

#undef GetMessage

class MenuScreen;

class ConfirmDlg : public FormWindow
{
  public:
    ConfirmDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~ConfirmDlg() override;

    void RegisterControls() override;
    void Show() override;

    ActiveWindow* GetParentControl();
    void SetParentControl(ActiveWindow* p);

    std::string GetTitle();
    void SetTitle(std::string_view t);

    std::string GetMessage();
    void SetMessage(std::string_view m);

    // Operations:
    virtual void ExecFrame();

    virtual void OnApply(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

  protected:
    MenuScreen* manager;

    ActiveWindow* lbl_title;
    ActiveWindow* lbl_message;

    Button* btn_apply;
    Button* btn_cancel;

    ActiveWindow* parent_control;
};

#endif ConfirmDlg_h
