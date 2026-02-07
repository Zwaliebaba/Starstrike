#pragma once

#include "Button.h"
#include "FormWindow.h"

class MenuScreen;

class AwardShowDlg : public FormWindow
{
  public:
    AwardShowDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~AwardShowDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();

    // Operations:
    virtual void OnClose(AWEvent* event);
    virtual void ShowAward();
    virtual void SetRank(int r);
    virtual void SetMedal(int r);

  protected:
    MenuScreen* manager;

    ActiveWindow* lbl_name;
    ActiveWindow* lbl_info;
    ImageBox* img_rank;
    Button* btn_close;

    bool exit_latch;

    int rank;
    int medal;
};
