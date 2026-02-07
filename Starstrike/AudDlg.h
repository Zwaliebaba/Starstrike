#ifndef AudDlg_h
#define AudDlg_h

#include "BaseScreen.h"
#include "Button.h"
#include "ComboBox.h"
#include "FormWindow.h"

class AudDlg : public FormWindow
{
  public:
    AudDlg(Screen* s, FormDef& def, BaseScreen* mgr);
    ~AudDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();

    // Operations:
    virtual void Apply();
    virtual void Cancel();

    virtual void OnApply(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

    virtual void OnAudio(AWEvent* event);
    virtual void OnVideo(AWEvent* event);
    virtual void OnOptions(AWEvent* event);
    virtual void OnControls(AWEvent* event);

  protected:
    BaseScreen* manager;

    Slider* efx_volume_slider;
    Slider* gui_volume_slider;
    Slider* wrn_volume_slider;
    Slider* vox_volume_slider;

    Slider* menu_music_slider;
    Slider* game_music_slider;

    Button* aud_btn;
    Button* vid_btn;
    Button* opt_btn;
    Button* ctl_btn;
    Button* mod_btn;

    Button* apply;
    Button* cancel;

    bool closed;
};

#endif AudDlg_h
