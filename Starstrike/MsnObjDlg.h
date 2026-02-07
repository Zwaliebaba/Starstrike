#ifndef MsnObjDlg_h
#define MsnObjDlg_h

#include "CameraView.h"
#include "FormWindow.h"
#include "MsnDlg.h"
#include "Scene.h"

class PlanScreen;
class Campaign;
class Mission;
class MissionInfo;
class ShipSolid;

class MsnObjDlg : public FormWindow, public MsnDlg
{
  public:
    MsnObjDlg(Screen* s, FormDef& def, PlanScreen* mgr);
    ~MsnObjDlg() override;

    void RegisterControls() override;
    virtual void ExecFrame();
    void Show() override;

    // Operations:
    void OnCommit(AWEvent* event) override;
    void OnCancel(AWEvent* event) override;
    void OnTabButton(AWEvent* event) override;
    virtual void OnSkin(AWEvent* event);

  protected:
    ActiveWindow* objectives;
    ActiveWindow* sitrep;
    ActiveWindow* player_desc;
    ActiveWindow* beauty;
    ComboBox* cmb_skin;
    CameraView* camview;
    Scene scene;
    Camera cam;
    ShipSolid* ship;
};

#endif MsnObjDlg_h
