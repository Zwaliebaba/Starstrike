#ifndef NetAddrDlg_h
#define NetAddrDlg_h

#include "Button.h"
#include "ComboBox.h"
#include "FormWindow.h"

class MenuScreen;

class NetAddrDlg : public FormWindow
{
  public:
    NetAddrDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~NetAddrDlg() override;

    void RegisterControls() override;
    void Show() override;

    // Operations:
    virtual void ExecFrame();

    virtual void OnSave(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

  protected:
    MenuScreen* manager;

    Button* btn_save;
    Button* btn_cancel;
    EditBox* edt_name;
    EditBox* edt_addr;
    EditBox* edt_port;
    EditBox* edt_pass;
};

#endif NetAddrDlg_h
