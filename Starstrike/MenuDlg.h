#ifndef MenuDlg_h
#define MenuDlg_h

#include "Button.h"
#include "ComboBox.h"
#include "FormWindow.h"

class MenuScreen;
class Campaign;
class Starshatter;

class MenuDlg : public FormWindow
{
  public:
    MenuDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~MenuDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();

    // Operations:
    virtual void OnStart(AWEvent* event);
    virtual void OnCampaign(AWEvent* event);
    virtual void OnMission(AWEvent* event);
    virtual void OnPlayer(AWEvent* event);
    virtual void OnMultiplayer(AWEvent* event);
    virtual void OnTacReference(AWEvent* event);

    virtual void OnVideo(AWEvent* event);
    virtual void OnOptions(AWEvent* event);
    virtual void OnControls(AWEvent* event);
    virtual void OnQuit(AWEvent* event);

    virtual void OnButtonEnter(AWEvent* event);
    virtual void OnButtonExit(AWEvent* event);

  protected:
    MenuScreen* manager;

    Button* btn_start;
    Button* btn_campaign;
    Button* btn_mission;
    Button* btn_player;
    Button* btn_multi;
    Button* btn_tac;

    Button* btn_video;
    Button* btn_options;
    Button* btn_controls;
    Button* btn_quit;

    ActiveWindow* version;
    ActiveWindow* description;

    Starshatter* stars;
    Campaign* campaign;
};

#endif MenuDlg_h
