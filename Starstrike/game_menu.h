#ifndef _included_gamemenu_h
#define _included_gamemenu_h

#include "GuiButton.h"
#include "GuiWindow.h"

#define MAX_GAME_TYPES 6

class GlobalInternet;

class GameMenu
{
  public:
    bool m_menuCreated;
    GlobalInternet* m_internet;

    GameMenu();

    void Render();

    void CreateMenu();
    void DestroyMenu();
};

class GameMenuButton : public GuiButton
{
  public:
    char* m_iconName;
    GameMenuButton(const char* _iconName);
    void Render(int realX, int realY, bool highlighted, bool clicked) override;
};

class GameMenuWindow : public GuiWindow
{
  public:
    int m_currentPage;
    int m_newPage;

    enum
    {
      PageMain = 0,
      PageDarwinia,
      PageMultiwinia,
      PageGameSetup,
      PageResearch,
      NumPages
    };

    GameMenuWindow();

    void Create() override;
    void Update() override;
    void Render(bool _hasFocus) override;

    void SetupNewPage(int _page);
    void SetupMainPage();
    void SetupDarwiniaPage();
    void GetDefaultPositions(int* _x, int* _y, int* _gap);
};

#endif
