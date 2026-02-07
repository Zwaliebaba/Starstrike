#ifndef MenuScreen_h
#define MenuScreen_h

#include "BaseScreen.h"

class MenuDlg;
class AudDlg;
class VidDlg;
class OptDlg;
class CtlDlg;
class KeyDlg;
class ExitDlg;
class ConfirmDlg;

class FirstTimeDlg;
class PlayerDlg;
class AwardShowDlg;

class MsnSelectDlg;
class CmpSelectDlg;
class MsnEditDlg;
class MsnElemDlg;
class MsnEventDlg;

class NetClientDlg;
class NetAddrDlg;
class NetPassDlg;
class NetLobbyDlg;
class NetServerDlg;
class NetUnitDlg;

class LoadDlg;
class TacRefDlg;

class ActiveWindow;
class Bitmap;
class DataLoader;
class FadeView;
class Font;
class FormWindow;
class Screen;
class Video;
class VideoFactory;
class Window;

class MenuScreen : public BaseScreen
{
  public:
    MenuScreen();
    ~MenuScreen() override;

    virtual void Setup(Screen* screen);
    virtual void TearDown();
    virtual bool CloseTopmost();

    virtual bool IsShown() const { return isShown; }
    virtual void Show();
    virtual void Hide();

    virtual void ExecFrame();

    virtual void ShowMenuDlg();
    virtual void ShowCmpSelectDlg();
    virtual void ShowMsnSelectDlg();
    virtual void ShowMsnEditDlg();
    virtual void ShowNetClientDlg();
    virtual void ShowNetAddrDlg();
    virtual void ShowNetPassDlg();
    virtual void ShowNetLobbyDlg();
    virtual void ShowNetServerDlg();
    virtual void ShowNetUnitDlg();
    virtual void ShowFirstTimeDlg();
    virtual void ShowPlayerDlg();
    virtual void ShowTacRefDlg();
    virtual void ShowAwardDlg();
    void ShowAudDlg() override;
    void ShowVidDlg() override;
    void ShowOptDlg() override;
    void ShowCtlDlg() override;
    void ShowKeyDlg() override;
    virtual void ShowExitDlg();
    virtual void ShowConfirmDlg();
    virtual void HideConfirmDlg();
    virtual void ShowLoadDlg();
    virtual void HideLoadDlg();

    // base screen interface:
    void ShowMsnElemDlg() override;
    void HideMsnElemDlg() override;
    MsnElemDlg* GetMsnElemDlg() override { return msnElemDlg; }

    virtual void ShowMsnEventDlg();
    virtual void HideMsnEventDlg();
    virtual MsnEventDlg* GetMsnEventDlg() { return msnEventDlg; }

    void ShowNavDlg() override;
    void HideNavDlg() override;
    bool IsNavShown() override;
    NavDlg* GetNavDlg() override { return msnEditNavDlg; }

    virtual MsnSelectDlg* GetMsnSelectDlg() const { return msnSelectDlg; }
    virtual MsnEditDlg* GetMsnEditDlg() const { return msnEditDlg; }
    virtual NetClientDlg* GetNetClientDlg() const { return netClientDlg; }
    virtual NetAddrDlg* GetNetAddrDlg() const { return netAddrDlg; }
    virtual NetPassDlg* GetNetPassDlg() const { return netPassDlg; }
    virtual NetLobbyDlg* GetNetLobbyDlg() const { return netLobbyDlg; }
    virtual NetServerDlg* GetNetServerDlg() const { return netServerDlg; }
    virtual NetUnitDlg* GetNetUnitDlg() const { return netUnitDlg; }
    virtual LoadDlg* GetLoadDlg() const { return loadDlg; }
    virtual TacRefDlg* GetTacRefDlg() const { return tacRefDlg; }

    AudDlg* GetAudDlg() const override { return auddlg; }
    VidDlg* GetVidDlg() const override { return viddlg; }
    OptDlg* GetOptDlg() const override { return optdlg; }
    CtlDlg* GetCtlDlg() const override { return ctldlg; }
    KeyDlg* GetKeyDlg() const override { return keydlg; }
    virtual ExitDlg* GetExitDlg() const { return exitdlg; }
    virtual FirstTimeDlg* GetFirstTimeDlg() const { return firstdlg; }
    virtual PlayerDlg* GetPlayerDlg() const { return playdlg; }
    virtual AwardShowDlg* GetAwardDlg() const { return awarddlg; }
    virtual ConfirmDlg* GetConfirmDlg() const { return confirmdlg; }

    void ApplyOptions() override;
    void CancelOptions() override;

  private:
    void HideAll();

    Screen* screen;
    MenuDlg* menudlg;

    Window* fadewin;
    FadeView* fadeview;
    ExitDlg* exitdlg;
    AudDlg* auddlg;
    VidDlg* viddlg;
    OptDlg* optdlg;
    CtlDlg* ctldlg;
    KeyDlg* keydlg;
    ConfirmDlg* confirmdlg;
    PlayerDlg* playdlg;
    AwardShowDlg* awarddlg;
    MsnSelectDlg* msnSelectDlg;
    MsnEditDlg* msnEditDlg;
    MsnElemDlg* msnElemDlg;
    MsnEventDlg* msnEventDlg;
    NavDlg* msnEditNavDlg;
    LoadDlg* loadDlg;
    TacRefDlg* tacRefDlg;

    CmpSelectDlg* cmpSelectDlg;
    FirstTimeDlg* firstdlg;
    NetClientDlg* netClientDlg;
    NetAddrDlg* netAddrDlg;
    NetPassDlg* netPassDlg;
    NetLobbyDlg* netLobbyDlg;
    NetServerDlg* netServerDlg;
    NetUnitDlg* netUnitDlg;

    FormWindow* current_dlg;
    DataLoader* loader;

    int wc, hc;
    bool isShown;
};

#endif MenuScreen_h
