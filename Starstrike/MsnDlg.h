#ifndef MsnDlg_h
#define MsnDlg_h

#include "Button.h"
#include "Campaign.h"
#include "FormWindow.h"
#include "PlanScreen.h"

class MsnDlg
{
  public:
    MsnDlg(PlanScreen* mgr);
    virtual ~MsnDlg();

    void RegisterMsnControls(FormWindow* win);
    void ShowMsnDlg();

    // Operations:
    virtual void OnCommit(AWEvent* event);
    virtual void OnCancel(AWEvent* event);
    virtual void OnTabButton(AWEvent* event);

  protected:
    virtual int CalcTimeOnTarget();

    PlanScreen* plan_screen;
    Button* commit;
    Button* cancel;
    Button* sit_button;
    Button* pkg_button;
    Button* nav_button;
    Button* wep_button;

    ActiveWindow* mission_name;
    ActiveWindow* mission_system;
    ActiveWindow* mission_sector;
    ActiveWindow* mission_time_start;
    ActiveWindow* mission_time_target;
    ActiveWindow* mission_time_target_label;

    Campaign* campaign;
    Mission* mission;
    MissionInfo* info;
    int pkg_index;
};

#endif MsnDlg_h
