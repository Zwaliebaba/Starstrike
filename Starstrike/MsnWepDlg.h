#ifndef MsnWepDlg_h
#define MsnWepDlg_h

#include "Bitmap.h"
#include "Button.h"
#include "FormWindow.h"
#include "ImageBox.h"
#include "ListBox.h"
#include "MsnDlg.h"

class PlanScreen;
class Campaign;
class Mission;
class MissionElement;
class HardPoint;
class WeaponDesign;

class MsnWepDlg : public FormWindow, public MsnDlg
{
  public:
    MsnWepDlg(Screen* s, FormDef& def, PlanScreen* mgr);
    ~MsnWepDlg() override;

    void RegisterControls() override;
    virtual void ExecFrame();
    void Show() override;

    // Operations:
    void OnCommit(AWEvent* event) override;
    void OnCancel(AWEvent* event) override;
    void OnTabButton(AWEvent* event) override;
    virtual void OnMount(AWEvent* event);
    virtual void OnLoadout(AWEvent* event);

  protected:
    virtual void SetupControls();
    virtual void BuildLists();
    virtual int LoadToPointIndex(int n);
    virtual int PointIndexToLoad(int n, int index);

    ActiveWindow* lbl_element;
    ActiveWindow* lbl_type;
    ActiveWindow* lbl_weight;
    ActiveWindow* player_desc;
    ImageBox* beauty;

    ActiveWindow* lbl_station[8];
    ActiveWindow* lbl_desc[8];
    Button* btn_load[8][8];

    ListBox* loadout_list;

    MissionElement* elem;
    WeaponDesign* designs[8];
    bool mounts[8][8];
    int loads[8];
    int first_station;

    Bitmap led_off;
    Bitmap led_on;
};

#endif MsnWepDlg_h
