#ifndef MsnNavDlg_h
#define MsnNavDlg_h

#include "MsnDlg.h"
#include "NavDlg.h"

class MsnNavDlg : public NavDlg, public MsnDlg
{
  public:
    MsnNavDlg(Screen* s, FormDef& def, PlanScreen* mgr);
    ~MsnNavDlg() override;

    void RegisterControls() override;
    void Show() override;
    void ExecFrame() override;

    // Operations:
    void OnCommit(AWEvent* event) override;
    void OnCancel(AWEvent* event) override;
};

#endif MsnNavDlg_h
