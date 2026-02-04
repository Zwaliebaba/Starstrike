#include "pch.h"
#include "game_menu.h"
#include "DX9TextRenderer.h"
#include "DropDownMenu.h"
#include "GameApp.h"
#include "camera.h"
#include "global_internet.h"
#include "renderer.h"

GameMenuButton::GameMenuButton(const char* _iconName)
{
  m_iconName = _strdup(_iconName);
  m_fontSize = 65.0f;
}

void GameMenuButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
  if (!m_iconName)
    return;
  auto parent = static_cast<GuiWindow*>(m_parent);

  realX += 150;
  UpdateButtonHighlight();

  glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
  g_gameFont.DrawText2DCenter(realX, realY, m_fontSize, m_iconName);

  glColor4f(1.3f, 1.0f, 1.3f, 1.0f);

  if (!m_mouseHighlightMode)
    highlighted = false;

  if (parent->m_buttonOrder[parent->m_currentButton] == this)
    highlighted = true;

  if (highlighted)
    glColor4f(1.0, 0.3f, 0.3, 1.0f);

  g_gameFont.DrawText2DCenter(realX, realY, m_fontSize, m_iconName);
}

class BackPageButton : public GameMenuButton
{
  public:
    int m_pageId;

    BackPageButton(int _page)
    //:   GameMenuButton( "icons\\menu_back.bmp" )
      : GameMenuButton("Back") { m_pageId = _page; }

    void MouseUp() override { dynamic_cast<GameMenuWindow*>(m_parent)->m_newPage = m_pageId; }
};

class QuitButton : public GameMenuButton
{
  public:
    QuitButton()
    //:   GameMenuButton( "icons\\menu_quit.bmp" )
      : GameMenuButton("Quit") {}

    void MouseUp() override { g_app->m_requestQuit = true; }
};

class CampaignButton : public GameMenuButton
{
  public:
    CampaignButton(const char* _iconName)
      : GameMenuButton(_iconName) {}

    void MouseUp() override { g_app->LoadCampaign(); }
};

class DarwiniaModeButton : public GameMenuButton
{
  public:
    DarwiniaModeButton(const char* _iconName)
      : GameMenuButton(_iconName) {}

    void MouseUp() override { dynamic_cast<GameMenuWindow*>(m_parent)->m_newPage = GameMenuWindow::PageDarwinia; }
};

GameMenu::GameMenu()
  : m_menuCreated(false) { m_internet = new GlobalInternet(); }

void GameMenu::Render()
{
  if (m_internet)
    m_internet->Render();
}

void GameMenu::CreateMenu()
{
  g_app->m_renderer->StartFadeIn(0.25f);
  // close all currently open windows
  auto windows = Canvas::EclGetWindows();
  while (windows->Size() > 0)
  {
    auto w = windows->GetData(0);
    Canvas::EclRemoveWindow(w->m_name);
  }

  // create the actual menu window
  Canvas::EclRegisterWindow(new GameMenuWindow());

  // set the camera to a position with a good view of the internet
  g_app->m_camera->RequestMode(Camera::ModeMainMenu);
  g_app->m_camera->SetTarget(LegacyVector3(-900000, 3000000, 397000), LegacyVector3(0, 0.5f, -1));
  g_app->m_camera->CutToTarget();

  g_app->m_gameMode = GameApp::GameModeNone;

  m_menuCreated = true;
}

void GameMenu::DestroyMenu()
{
  m_menuCreated = false;
  Canvas::EclRemoveWindow("GameMenu");
  g_app->m_renderer->StartFadeOut();
}

// *************************
// GameMenuWindow Class
// *************************

GameMenuWindow::GameMenuWindow()
  : GuiWindow("GameMenu"),
    m_currentPage(-1),
    m_newPage(PageDarwinia)
{
  int w = ClientEngine::OutputSize().Width;
  int h = ClientEngine::OutputSize().Height;
  SetPosition(5, 5);
  SetSize(w - 10, h - 10);
  m_resizable = false;
  SetMovable(false);
}

void GameMenuWindow::Create() {}

void GameMenuWindow::Update()
{
  GuiWindow::Update();
  if (m_currentPage != m_newPage)
  {
    SetupNewPage(m_newPage);

    int w = ClientEngine::OutputSize().Width;
    int h = ClientEngine::OutputSize().Height;
    SetPosition(5, 5);
    SetSize(w - 10, h - 10);
  }
}

void GameMenuWindow::Render(bool _hasFocus)
{
  //GuiWindow::Render( _hasFocus );
  // render nothing but the buttons
  GuiWindow::Render(_hasFocus);

  int w = ClientEngine::OutputSize().Width;
  int h = ClientEngine::OutputSize().Height;

  glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
  //g_gameFont.SetRenderOutline(true);
  g_gameFont.DrawText2DCenter(w / 2, 30, 80.0f, "DARWINIA");

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  //g_gameFont.SetRenderOutline(false);
  g_gameFont.DrawText2DCenter(w / 2, 30, 80.0f, "DARWINIA");
}

void GameMenuWindow::SetupNewPage(int _page)
{
  Remove();

  switch (_page)
  {
    case PageMain:
      SetupMainPage();
      break;
    case PageDarwinia:
      SetupDarwiniaPage();
      break;
  }

  m_currentPage = _page;
}

void GameMenuWindow::SetupMainPage()
{
  int x, y, gap;
  GetDefaultPositions(&x, &y, &gap);
  int h = 60;
  int w = 300;

  auto dmb = new DarwiniaModeButton("Darwinia");
  dmb->SetProperties("darwinia", x, y, w, h);
  RegisterButton(dmb);
  m_buttonOrder.PutData(dmb);

  auto quit = new QuitButton();
  quit->SetProperties("quit", x, y += gap, w, h);
  RegisterButton(quit);
  m_buttonOrder.PutData(quit);
}

void GameMenuWindow::SetupDarwiniaPage()
{
  int x, y, gap;
  GetDefaultPositions(&x, &y, &gap);
  int h = 60;
  int w = 300;

  auto cb = new CampaignButton("Campaign");
  cb->SetProperties("campaign", x, y += gap, w, h);
  RegisterButton(cb);
  m_buttonOrder.PutData(cb);

  auto quit = new QuitButton();
  quit->SetProperties("quit", x, y += gap, w, h);
  RegisterButton(quit);
  m_buttonOrder.PutData(quit);
}

void GameMenuWindow::GetDefaultPositions(int* _x, int* _y, int* _gap)
{
  float w = ClientEngine::OutputSize().Width;
  float h = ClientEngine::OutputSize().Height;

  *_x = (w / 2) - 150;
  switch (m_newPage)
  {
    case PageMain:
    case PageDarwinia:
      *_y = h / 864.0f * 200.0f;
      *_gap = *_y;
      break;
    case PageMultiwinia:
      *_y = h / 864.0f * 200.0f;
      *_gap = *_y / 1.5f;
      break;
    case PageGameSetup:
    case PageResearch:
      *_y = h / 864.0f * 70.0f;
      *_gap = (h / 864) * 60;
      break;
  }

  //*_x = min( *_x, 200 );
}
