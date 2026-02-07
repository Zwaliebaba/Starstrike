#ifndef GameScreen_h
#define GameScreen_h

#include "BaseScreen.h"
#include "Bitmap.h"

class Screen;
class Sim;
class Window;
class Font;

class NavDlg;
class EngDlg;
class FltDlg;
class CtlDlg;
class KeyDlg;

class CameraDirector;
class DisplayView;
class HUDView;
class WepView;
class QuantumView;
class QuitView;
class RadioView;
class TacticalView;
class CameraView;
class PolyRender;
class Bitmap;
class DataLoader;
class Video;
class VideoFactory;

class GameScreen : public BaseScreen
{
  public:
    GameScreen();
    ~GameScreen() override;

    virtual void Setup(Screen* screen);
    virtual void TearDown();
    virtual bool CloseTopmost();

    virtual bool IsShown() const { return isShown; }
    virtual void Show();
    virtual void Hide();

    virtual bool IsFormShown() const;
    virtual void ShowExternal();

    void ShowNavDlg() override;
    void HideNavDlg() override;
    bool IsNavShown() override;
    NavDlg* GetNavDlg() override { return navdlg; }

    virtual void ShowEngDlg();
    virtual void HideEngDlg();
    virtual bool IsEngShown();
    virtual EngDlg* GetEngDlg() { return engdlg; }

    virtual void ShowFltDlg();
    virtual void HideFltDlg();
    virtual bool IsFltShown();
    virtual FltDlg* GetFltDlg() { return fltdlg; }

    void ShowCtlDlg() override;
    virtual void HideCtlDlg();
    virtual bool IsCtlShown();

    void ShowKeyDlg() override;
    virtual bool IsKeyShown();

    AudDlg* GetAudDlg() const override { return auddlg; }
    VidDlg* GetVidDlg() const override { return viddlg; }
    OptDlg* GetOptDlg() const override { return optdlg; }
    CtlDlg* GetCtlDlg() const override { return ctldlg; }
    KeyDlg* GetKeyDlg() const override { return keydlg; }

    void ShowAudDlg() override;
    virtual void HideAudDlg();
    virtual bool IsAudShown();

    void ShowVidDlg() override;
    virtual void HideVidDlg();
    virtual bool IsVidShown();

    void ShowOptDlg() override;
    virtual void HideOptDlg();
    virtual bool IsOptShown();

    void ApplyOptions() override;
    void CancelOptions() override;

    virtual void ShowWeaponsOverlay();
    virtual void HideWeaponsOverlay();

    void SetFieldOfView(double fov);
    double GetFieldOfView() const;
    void CycleMFDMode(int mfd);
    void CycleHUDMode();
    void CycleHUDColor();
    void CycleHUDWarn();
    void FrameRate(double f);
    void ExecFrame();

    static GameScreen* GetInstance() { return game_screen; }
    CameraView* GetCameraView() const { return cam_view; }
    Bitmap* GetLensFlare(int index);

  private:
    void HideAll();

    Sim* sim;
    Screen* screen;

    Window* gamewin;
    NavDlg* navdlg;
    EngDlg* engdlg;
    FltDlg* fltdlg;
    CtlDlg* ctldlg;
    KeyDlg* keydlg;
    AudDlg* auddlg;
    VidDlg* viddlg;
    OptDlg* optdlg;

    Font* HUDfont;
    Font* GUIfont;
    Font* GUI_small_font;
    Font* title_font;

    Bitmap* flare1;
    Bitmap* flare2;
    Bitmap* flare3;
    Bitmap* flare4;

    CameraDirector* cam_dir;
    DisplayView* disp_view;
    HUDView* hud_view;
    WepView* wep_view;
    QuantumView* quantum_view;
    QuitView* quit_view;
    TacticalView* tac_view;
    RadioView* radio_view;
    CameraView* cam_view;
    DataLoader* loader;

    double frame_rate;
    bool isShown;

    static GameScreen* game_screen;
};

#endif GameScreen_h
