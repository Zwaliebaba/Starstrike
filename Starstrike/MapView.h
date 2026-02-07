#pragma once

#include "ActiveWindow.h"
#include "Bitmap.h"
#include "Campaign.h"
#include "EventTarget.h"
#include "List.h"
#include "Menu.h"
#include "MenuView.h"
#include "Mission.h"
#include "Ship.h"
#include "SimObject.h"
#include "StarSystem.h"
#include "View.h"

constexpr int EID_MAP_CLICK = 1000;

class MapView : public View, public EventTarget, public SimObserver
{
  public:
    MapView(Window* win);
    ~MapView() override;

    // Operations:
    void Refresh() override;
    void OnWindowMove() override;
    void OnShow() override;
    void OnHide() override;

    virtual void DrawTitle();
    virtual void DrawGalaxy();
    virtual void DrawSystem();
    virtual void DrawRegion();

    virtual void DrawGrid();
    virtual void DrawOrbital(Orbital& orbital, int index);
    virtual void DrawShip(Ship& ship, bool current = false, int rep = 3);
    virtual void DrawElem(MissionElement& elem, bool current = false, int rep = 3);
    virtual void DrawNavRoute(OrbitalRegion* rgn, List<Instruction>& route, Color smarker, Ship* ship = nullptr,
                              MissionElement* elem = nullptr);

    virtual void DrawCombatantSystem(Combatant* c, Orbital* rgn, int x, int y, int r);
    virtual void DrawCombatGroupSystem(CombatGroup* g, Orbital* rgn, int x1, int x2, int& y, int a);
    virtual void DrawCombatGroup(CombatGroup* g, int rep = 3);

    virtual int GetViewMode() const { return view_mode; }
    virtual void SetViewMode(int mode);
    virtual void SetSelectionMode(int mode);
    virtual int GetSelectionMode() const { return seln_mode; }
    virtual void SetSelection(int index);
    virtual void SetSelectedShip(Ship* ship);
    virtual void SetSelectedElem(MissionElement* elem);
    virtual void SetRegion(OrbitalRegion* rgn);
    virtual void SetRegionByName(std::string_view rgn_name);
    virtual void SelectAt(int x, int y);
    virtual Orbital* GetSelection();
    virtual Ship* GetSelectedShip();
    virtual MissionElement* GetSelectedElem();
    virtual int GetSelectionIndex();
    virtual void SetShipFilter(DWORD f) { ship_filter = f; }

    // Event Target Interface:
    int OnMouseMove(int x, int y) override;
    int OnLButtonDown(int x, int y) override;
    int OnLButtonUp(int x, int y) override;
    int OnClick() override;
    int OnRButtonDown(int x, int y) override;
    int OnRButtonUp(int x, int y) override;

    bool IsEnabled() const override;
    bool IsVisible() const override;
    bool IsFormActive() const override;
    Rect TargetRect() const override;

    void ZoomIn();
    void ZoomOut();

    void SetGalaxy(List<StarSystem>& systems);
    void SetSystem(StarSystem* s);
    void SetMission(Mission* m);
    void SetShip(Ship* s);
    void SetCampaign(Campaign* c);

    bool IsVisible(const Point& loc);

    // accessors:
    virtual void GetClickLoc(double& x, double& y)
    {
      x = click_x;
      y = click_y;
    }

    List<StarSystem>& GetGalaxy() { return system_list; }
    StarSystem* GetSystem() const { return system; }
    OrbitalRegion* GetRegion() const;

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override { return "MapWin"; }

    bool GetEditorMode() const { return editor; }
    void SetEditorMode(bool b) { editor = b; }

  protected:
    virtual void BuildMenu();
    virtual void ClearMenu();
    virtual void ProcessMenuItem(int action);
    virtual bool SetCapture();
    virtual bool ReleaseCapture();

    virtual void DrawTabbedText(Font* font, std::string_view text);

    bool IsClutter(Ship& s);
    bool IsCrowded(Ship& s);
    bool IsCrowded(MissionElement& elem);
    void GetShipLoc(Ship& s, POINT& loc);
    void GetElemLoc(MissionElement& s, POINT& loc);
    void SelectShip(Ship* selship);
    void SelectElem(MissionElement* selelem);
    void SelectNavpt(Instruction* navpt);
    void FindShips(bool friendly, bool station, bool starship, bool dropship, std::vector<std::string>& result);
    void SetupScroll(Orbital* s);

    double GetMinRadius(int type);

    std::string m_title;
    Rect rect;
    Campaign* campaign = {};
    Mission* mission = {};
    List<StarSystem> system_list;
    StarSystem* system = {};
    List<Orbital> stars;
    List<Orbital> planets;
    List<Orbital> m_regions;
    Ship* ship = {};
    Bitmap galaxy_image;
    bool editor;

    int current_star = {};
    int current_planet = {};
    size_t m_currentRegion = {};
    Ship* current_ship = {};
    MissionElement* current_elem = {};
    Instruction* current_navpt = {};
    int current_status = {};

    int view_mode;
    int seln_mode;
    bool captured;
    bool dragging;
    bool adding_navpt;
    bool moving_navpt;
    bool moving_elem;
    int scrolling;
    int mouse_x;
    int mouse_y;
    DWORD ship_filter;

    double zoom;
    double view_zoom[3];
    double offset_x;
    double offset_y;
    double view_offset_x[3];
    double view_offset_y[3];
    double c, r;
    double scroll_x;
    double scroll_y;
    double click_x;
    double click_y;

    Font* font = {};
    Font* m_titleFont = {};

    ActiveWindow* active_window = {};
    Menu* active_menu = {};

    Menu* map_menu = {};
    Menu* map_system_menu = {};
    Menu* map_sector_menu = {};
    Menu* ship_menu = {};
    Menu* nav_menu = {};
    Menu* action_menu = {};
    Menu* objective_menu = {};
    Menu* formation_menu = {};
    Menu* speed_menu = {};
    Menu* hold_menu = {};
    Menu* farcast_menu = {};

    MenuView* menu_view = {};
};
