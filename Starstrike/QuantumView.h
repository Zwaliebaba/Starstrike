#ifndef QuantumView_h
#define QuantumView_h

#include "Color.h"
#include "SimObject.h"
#include "View.h"

class Ship;
class RadioMessage;
class HUDView;
class Menu;
class Font;

class QuantumView : public View, public SimObserver
{
  public:
    QuantumView(Window* c);
    ~QuantumView() override;

    // Operations:
    void Refresh() override;
    void OnWindowMove() override;
    virtual void ExecFrame();

    virtual Menu* GetQuantumMenu(Ship* ship);
    virtual bool IsMenuShown();
    virtual void ShowMenu();
    virtual void CloseMenu();

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

    static void SetColor(Color c);

    static void Initialize();
    static void Close();

    static QuantumView* GetInstance() { return quantum_view; }

  protected:
    int width, height;
    double xcenter, ycenter;

    Font* font;
    Sim* sim;
    Ship* ship;

    static QuantumView* quantum_view;
};

#endif QuantumView_h
