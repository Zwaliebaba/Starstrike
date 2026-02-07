#ifndef CtlDlg_h
#define CtlDlg_h

#include "Button.h"
#include "FormWindow.h"

class BaseScreen;
class MenuScreen;
class GameScreen;

class CtlDlg : public FormWindow
{
  public:
    CtlDlg(Screen* s, FormDef& def, BaseScreen* mgr);
    ~CtlDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();

    // Operations:
    virtual void OnCommand(AWEvent* event);
    virtual void OnCategory(AWEvent* event);

    virtual void OnControlModel(AWEvent* event);

    virtual void OnMouseSelect(AWEvent* event);
    virtual void OnMouseSensitivity(AWEvent* event);
    virtual void OnMouseInvert(AWEvent* event);

    virtual void Apply();
    virtual void Cancel();

    virtual void OnApply(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

    virtual void OnAudio(AWEvent* event);
    virtual void OnVideo(AWEvent* event);
    virtual void OnOptions(AWEvent* event);
    virtual void OnControls(AWEvent* event);

  protected:
    void ShowCategory();
    void UpdateCategory();

    BaseScreen* manager;

    Button* category[4];
    ListBox* commands;
    int command_index;

    ComboBox* control_model_combo;

    ComboBox* mouse_select_combo;
    Slider* mouse_sensitivity_slider;
    Button* mouse_invert_checkbox;

    Button* aud_btn;
    Button* vid_btn;
    Button* opt_btn;
    Button* ctl_btn;

    Button* apply;
    Button* cancel;

    int selected_category;
    int control_model;

    int mouse_select;
    int mouse_sensitivity;
    int mouse_invert;

    bool closed;
};

#endif CtlDlg_h
