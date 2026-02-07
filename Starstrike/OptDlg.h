#ifndef OptDlg_h
#define OptDlg_h

#include "Button.h"
#include "ComboBox.h"
#include "FormWindow.h"

class BaseScreen;

class OptDlg : public FormWindow
{
  public:
    OptDlg(Screen* s, FormDef& def, BaseScreen* mgr);
    ~OptDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();

    // Operations:
    virtual void Apply();
    virtual void Cancel();

    virtual void OnApply(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

    virtual void OnEnter(AWEvent* event);
    virtual void OnExit(AWEvent* event);

    virtual void OnAudio(AWEvent* event);
    virtual void OnVideo(AWEvent* event);
    virtual void OnOptions(AWEvent* event);
    virtual void OnControls(AWEvent* event);

  protected:
    BaseScreen* manager;

    ComboBox* flight_model;
    ComboBox* flying_start;
    ComboBox* landings;
    ComboBox* ai_difficulty;
    ComboBox* hud_mode;
    ComboBox* hud_color;
    ComboBox* joy_mode;
    ComboBox* ff_mode;
    ComboBox* grid_mode;
    ComboBox* gunsight;

    ActiveWindow* description;

    Button* aud_btn;
    Button* vid_btn;
    Button* opt_btn;
    Button* ctl_btn;

    Button* apply;
    Button* cancel;

    bool closed;
};

#endif OptDlg_h
