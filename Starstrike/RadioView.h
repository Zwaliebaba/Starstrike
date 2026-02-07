#ifndef RadioView_h
#define RadioView_h

#include "Color.h"
#include "SimObject.h"
#include "View.h"

class Font;
class Element;
class Ship;
class RadioMessage;
class CameraView;
class HUDView;
class Menu;
class MenuItem;

class RadioView : public View, public SimObserver
{
  public:
    RadioView(Window* c);
    ~RadioView() override;

    // Operations:
    void Refresh() override;
    void OnWindowMove() override;
    virtual void ExecFrame();

    virtual Menu* GetRadioMenu(Ship* ship);
    virtual bool IsMenuShown();
    virtual void ShowMenu();
    virtual void CloseMenu();

    static void Message(std::string_view msg);
    static void ClearMessages();

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

    static void SetColor(Color c);

    static void Initialize();
    static void Close();

    static RadioView* GetInstance() { return radio_view; }

  protected:
    void SendRadioMessage(Ship* ship, MenuItem* item);

    int width, height;
    double xcenter, ycenter;

    Font* font;
    Sim* sim;
    Ship* ship;
    Element* dst_elem;

    enum { MAX_MSG = 6 };

    std::string msg_text[MAX_MSG];
    double msg_time[MAX_MSG];

    static RadioView* radio_view;
    static std::mutex m_mutex;
};

#endif RadioView_h
