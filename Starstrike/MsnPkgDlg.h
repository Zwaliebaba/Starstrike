#ifndef MsnPkgDlg_h
#define MsnPkgDlg_h

#include "FormWindow.h"
#include "ListBox.h"
#include "MsnDlg.h"

class PlanScreen;
class Campaign;
class Mission;
class MissionInfo;

class MsnPkgDlg : public FormWindow, public MsnDlg
{
  public:
    MsnPkgDlg(Screen* s, FormDef& def, PlanScreen* mgr);
    ~MsnPkgDlg() override;

    void RegisterControls() override;
    virtual void ExecFrame();
    void Show() override;

    // Operations:
    void OnCommit(AWEvent* event) override;
    void OnCancel(AWEvent* event) override;
    void OnTabButton(AWEvent* event) override;
    virtual void OnPackage(AWEvent* event);

  protected:
    virtual void DrawPackages();
    virtual void DrawNavPlan();
    virtual void DrawThreats();

    ListBox* pkg_list;
    ListBox* nav_list;

    ActiveWindow* threat[5];
};

#endif MsnPkgDlg_h
