#ifndef CmpSelectDlg_h
#define CmpSelectDlg_h

#include "Bitmap.h"
#include "Button.h"
#include "ComboBox.h"
#include "FormWindow.h"
#include "ListBox.h"

class MenuScreen;
class Campaign;
class Starshatter;

class CmpSelectDlg : public FormWindow
{
  public:
    CmpSelectDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~CmpSelectDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();
    virtual bool CanClose();

    // Operations:
    virtual void OnCampaignSelect(AWEvent* event);
    virtual void OnNew(AWEvent* event);
    virtual void OnSaved(AWEvent* event);
    virtual void OnDelete(AWEvent* event);
    virtual void OnConfirmDelete(AWEvent* event);
    virtual void OnAccept(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

    virtual DWORD LoadProc();

  protected:
    virtual void StartLoadProc();
    virtual void StopLoadProc();
    virtual void ShowNewCampaigns();
    virtual void ShowSavedCampaigns();

    MenuScreen* manager;

    Button* btn_new;
    Button* btn_saved;
    Button* btn_delete;
    Button* btn_accept;
    Button* btn_cancel;

    ListBox* lst_campaigns;

    ActiveWindow* description;

    Starshatter* stars;
    Campaign* campaign;
    int selected_mission;
    HANDLE hproc;
    std::mutex m_mutex;
    bool loading;
    bool loaded;
    std::string load_file;
    int load_index;
    bool show_saved;
    List<Bitmap> images;

    std::string select_msg;
};

#endif CmpSelectDlg_h
