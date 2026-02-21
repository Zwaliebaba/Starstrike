#include "pch.h"
#include "language_table.h"
#include "resource.h"
#include "text_renderer.h"
#include "text_stream_readers.h"
#include "drop_down_menu.h"
#include "input_field.h"
#include "im_renderer.h"
#include "game_menu.h"
#include "app.h"
#include "camera.h"
#include "global_internet.h"
#include "renderer.h"

// *************************
// Button Classes
// *************************

GameMenuButton::GameMenuButton(char* _iconName)
{
  m_iconName = strdup(_iconName);
  m_fontSize = 65.0f;
}

void GameMenuButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
  if (!m_iconName)
    return;
  auto parent = static_cast<DarwiniaWindow*>(m_parent);

  realX += 150;
  UpdateButtonHighlight();

  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.0f);
  g_gameFont.SetRenderOutline(true);
  g_gameFont.DrawText2DCentre(realX, realY, m_fontSize, m_iconName);

  g_imRenderer->Color4f(1.3f, 1.0f, 1.3f, 1.0f);

  if (!m_mouseHighlightMode)
    highlighted = false;

  if (parent->m_buttonOrder[parent->m_currentButton] == this)
    highlighted = true;

  if (highlighted)
  {
    g_imRenderer->Color4f(1.0, 0.3f, 0.3, 1.0f);
  }

  g_gameFont.SetRenderOutline(false);
  g_gameFont.DrawText2DCentre(realX, realY, m_fontSize, m_iconName);
}

class QuitButton : public GameMenuButton
{
  public:
    QuitButton()
      : GameMenuButton("Quit") {}

    void MouseUp() override { g_app->m_requestQuit = true; }
};

class PrologueButton : public GameMenuButton
{
  public:
    PrologueButton(char* _iconName)
      : GameMenuButton(_iconName) {}

    void MouseUp() override { g_app->LoadPrologue(); }
};

class CampaignButton : public GameMenuButton
{
  public:
    CampaignButton(char* _iconName)
      : GameMenuButton(_iconName) {}

    void MouseUp() override { g_app->LoadCampaign(); }
};

class DarwiniaModeButton : public GameMenuButton
{
  public:
    DarwiniaModeButton(char* _iconName)
      : GameMenuButton(_iconName) {}

    void MouseUp() override { static_cast<GameMenuWindow*>(m_parent)->m_newPage = GameMenuWindow::PageDarwinia; }
};

// *************************
// Game Menu Class
// *************************

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
  LList<EclWindow*>* windows = EclGetWindows();
  while (windows->Size() > 0)
  {
    EclWindow* w = windows->GetData(0);
    EclRemoveWindow(w->m_name);
  }

  // create the actual menu window
  EclRegisterWindow(new GameMenuWindow());

  // set the camera to a position with a good view of the internet
  g_app->m_camera->RequestMode(Camera::ModeMainMenu);
  g_app->m_camera->SetDebugMode(Camera::DebugModeNever);
  g_app->m_camera->SetTarget(LegacyVector3(-900000, 3000000, 397000), LegacyVector3(0, 0.5f, -1));
  g_app->m_camera->CutToTarget();

  g_app->m_gameMode = App::GameModeNone;

  m_menuCreated = true;
}

void GameMenu::DestroyMenu()
{
  m_menuCreated = false;
  EclRemoveWindow("GameMenu");
  g_app->m_renderer->StartFadeOut();
}

// *************************
// GameMenuWindow Class
// *************************

GameMenuWindow::GameMenuWindow()
  : DarwiniaWindow("GameMenu"),
    m_currentPage(-1),
    m_newPage(PageDarwinia)
{
  int w = g_app->m_renderer->ScreenW();
  int h = g_app->m_renderer->ScreenH();
  SetPosition(5, 5);
  SetSize(w - 10, h - 10);
  m_resizable = false;
  SetMovable(false);
}

void GameMenuWindow::Create() {}

void GameMenuWindow::Update()
{
  DarwiniaWindow::Update();
  if (m_currentPage != m_newPage)
  {
    SetupNewPage(m_newPage);

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();
    SetPosition(5, 5);
    SetSize(w - 10, h - 10);
  }
}

void GameMenuWindow::Render(bool _hasFocus)
{
  EclWindow::Render(_hasFocus);

  int w = g_app->m_renderer->ScreenW();
  int h = g_app->m_renderer->ScreenH();

  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.0f);
  g_gameFont.SetRenderOutline(true);
  g_gameFont.DrawText2DCentre(w / 2, 30, 80.0f, "DARWINIA");

  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
  g_gameFont.SetRenderOutline(false);
  g_gameFont.DrawText2DCentre(w / 2, 30, 80.0f, "DARWINIA");
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
  dmb->SetShortProperties("darwinia", x, y, w, h);
  RegisterButton(dmb);
  m_buttonOrder.PutData(dmb);

  auto quit = new QuitButton();
  quit->SetShortProperties("quit", x, y += gap, w, h);
  RegisterButton(quit);
  m_buttonOrder.PutData(quit);
}

void GameMenuWindow::SetupDarwiniaPage()
{
  int x, y, gap;
  GetDefaultPositions(&x, &y, &gap);
  int h = 60;
  int w = 300;

  auto pb = new PrologueButton("Prologue");
  pb->SetShortProperties("prologue", x, y, w, h);
  RegisterButton(pb);
  m_buttonOrder.PutData(pb);

  auto cb = new CampaignButton("Campaign");
  cb->SetShortProperties("campaign", x, y += gap, w, h);
  RegisterButton(cb);
  m_buttonOrder.PutData(cb);

  auto quit = new QuitButton();
  quit->SetShortProperties("quit", x, y += gap, w, h);
  RegisterButton(quit);
  m_buttonOrder.PutData(quit);
}

void GameMenuWindow::GetDefaultPositions(int* _x, int* _y, int* _gap)
{
  float w = g_app->m_renderer->ScreenW();
  float h = g_app->m_renderer->ScreenH();

  *_x = (w / 2) - 150;
  switch (m_newPage)
  {
  case PageMain:
  case PageDarwinia:
    *_y = static_cast<float>((h / 864.0f) * 200.0f);
    *_gap = *_y;
    break;
  case PageMultiwinia:
    *_y = static_cast<float>((h / 864.0f) * 200.0f);
    *_gap = *_y / 1.5f;
    break;
  case PageGameSetup:
  case PageResearch:
    *_y = static_cast<float>((h / 864.0f) * 70.0f);
    *_gap = (h / 864) * 60;
    break;
  }
}
