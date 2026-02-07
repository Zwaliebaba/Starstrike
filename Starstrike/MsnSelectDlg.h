#ifndef MsnSelectDlg_h
#define MsnSelectDlg_h

#include "Button.h"
#include "ComboBox.h"
#include "FormWindow.h"
#include "ListBox.h"

class MenuScreen;
class Campaign;
class Starshatter;

class MsnSelectDlg : public FormWindow
{
  public:
    MsnSelectDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~MsnSelectDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();

    // Operations:
    virtual void OnCampaignSelect(AWEvent* event);
    virtual void OnMissionSelect(AWEvent* event);

    virtual void OnMod(AWEvent* event);
    virtual void OnNew(AWEvent* event);
    virtual void OnEdit(AWEvent* event);
    virtual void OnDel(AWEvent* event);
    virtual void OnDelConfirm(AWEvent* event);
    virtual void OnAccept(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

  protected:
    MenuScreen* manager;

    Button* btn_mod;
    Button* btn_new;
    Button* btn_edit;
    Button* btn_del;
    Button* btn_accept;
    Button* btn_cancel;

    ComboBox* cmb_campaigns;
    ListBox* lst_campaigns;
    ListBox* lst_missions;

    ActiveWindow* description;

    Starshatter* stars;
    Campaign* campaign;
    int selected_mission;
    int mission_id;
    bool editable;
};

#endif MsnSelectDlg_h
