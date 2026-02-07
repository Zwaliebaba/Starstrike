#ifndef WepView_h
#define WepView_h

#include "Bitmap.h"
#include "Projector.h"
#include "SimObject.h"
#include "System.h"
#include "View.h"

class Graphic;
class Sprite;
class Ship;
class Contact;
class HUDView;

class WepView : public View, public SimObserver
{
  public:
    WepView(Window* c);
    ~WepView() override;

    // Operations:
    void Refresh() override;
    void OnWindowMove() override;
    virtual void ExecFrame();
    virtual void SetOverlayMode(int mode);
    virtual int GetOverlayMode() const { return mode; }
    virtual void CycleOverlayMode();

    virtual void RestoreOverlay();

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

    static WepView* GetInstance() { return wep_view; }
    static void SetColor(Color c);

    static bool IsMouseLatched();

  protected:
    void DrawOverlay();

    void DoMouseFrame();
    bool CheckButton(int index, int x, int y);
    void CycleSubTarget(int direction);

    int mode;
    int transition;
    int m_mouseDown;
    int width, height, aw, ah;
    double xcenter, ycenter;

    Sim* sim;
    Ship* ship;
    SimObject* target;
    HUDView* hud;

    enum
    {
      MAX_WEP = 4,
      MAX_BTN = 16
    };

    Rect btn_rect[MAX_BTN];

    SimRegion* active_region;

    static WepView* wep_view;
};

#endif WepView_h
