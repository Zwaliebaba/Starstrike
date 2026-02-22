#include "pch.h"

#include "win32_eventhandler.h"
#include "debug_utils.h"
#include "render_device.h"
#include "render_states.h"
#include "texture_manager.h"
#include "im_renderer.h"
#include "window_manager.h"
#include "window_manager_win32.h"

static HINSTANCE g_hInstance;

#define WH_KEYBOARD_LL 13

WindowManager* g_windowManager = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  W32EventHandler* w = getW32EventHandler();

  if (!w || (w->WndProc(hWnd, message, wParam, lParam) == -1))
    return DefWindowProc(hWnd, message, wParam, lParam);

  return 0;
}

WindowManager::WindowManager()
  : m_mousePointerVisible(true),
    m_mouseCaptured(false),
    m_mouseOffsetX(INT_MAX)
{
  DarwiniaDebugAssert(g_windowManager == NULL);
  m_win32Specific = new WindowManagerWin32;

  ListAllDisplayModes();

  SaveDesktop();
}

WindowManager::~WindowManager() { DestroyWin(); }

void WindowManager::SaveDesktop()
{
  DEVMODE mode;
  ZeroMemory(&mode, sizeof(mode));
  mode.dmSize = sizeof(mode);
  bool success = EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &mode);

  m_desktopScreenW = mode.dmPelsWidth;
  m_desktopScreenH = mode.dmPelsHeight;
  m_desktopColourDepth = mode.dmBitsPerPel;
  m_desktopRefresh = mode.dmDisplayFrequency;
}

void WindowManager::RestoreDesktop()
{
  //
  // Get current settings

  DEVMODE mode;
  ZeroMemory(&mode, sizeof(mode));
  mode.dmSize = sizeof(mode);
  bool success = EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &mode);

  //
  // has anything changed?

  bool changed = (m_desktopScreenW != mode.dmPelsWidth || m_desktopScreenH != mode.dmPelsHeight || m_desktopColourDepth != mode.dmBitsPerPel
    || m_desktopRefresh != mode.dmDisplayFrequency);

  //
  // Restore if required

  if (changed)
  {
    DEVMODE devmode;
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmBitsPerPel = m_desktopColourDepth;
    devmode.dmPelsWidth = m_desktopScreenW;
    devmode.dmPelsHeight = m_desktopScreenH;
    devmode.dmDisplayFrequency = m_desktopRefresh;
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;
    long result = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
  }
}

// Returns an index into the list of already registered resolutions
int WindowManager::GetResolutionId(int _width, int _height)
{
  for (int i = 0; i < m_resolutions.Size(); ++i)
  {
    Resolution* res = m_resolutions[i];
    if (res->m_width == _width && res->m_height == _height)
      return i;
  }

  return -1;
}

void WindowManager::ListAllDisplayModes()
{
  int i = 0;
  DEVMODE devMode;
  while (EnumDisplaySettings(nullptr, i, &devMode) != 0)
  {
    if (devMode.dmBitsPerPel >= 15 && devMode.dmPelsWidth >= 640 && devMode.dmPelsHeight >= 480)
    {
      int resId = GetResolutionId(devMode.dmPelsWidth, devMode.dmPelsHeight);
      Resolution* res;
      if (resId == -1)
      {
        res = new Resolution(devMode.dmPelsWidth, devMode.dmPelsHeight);
        m_resolutions.PutDataAtEnd(res);
      }
      else
        res = m_resolutions[resId];

      if (res->m_refreshRates.FindData(devMode.dmDisplayFrequency) == -1)
        res->m_refreshRates.PutDataAtEnd(devMode.dmDisplayFrequency);
    }
    ++i;
  }
}

Resolution* WindowManager::GetResolution(int _id)
{
  if (m_resolutions.ValidIndex(_id))
    return m_resolutions[_id];

  return nullptr;
}

bool WindowManager::Windowed() { return m_windowed; }

bool WindowManager::CreateWin(int _width, int _height, bool _windowed, int _colourDepth, int _refreshRate, int _zDepth, bool _waitVRT)
{
  m_screenW = _width;
  m_screenH = _height;
  m_windowed = _windowed;

  // Register window class
  WNDCLASSA wc;
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = g_hInstance;
  wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = "Darwinia";
  RegisterClassA(&wc);

  int posX, posY;
  unsigned int windowStyle = WS_VISIBLE;

  if (_windowed)
  {
    RestoreDesktop();

    windowStyle |= WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    RECT windowRect = {0, 0, _width, _height};
    AdjustWindowRect(&windowRect, windowStyle, false);
    m_borderWidth = ((windowRect.right - windowRect.left) - _width) / 2;
    m_titleHeight = ((windowRect.bottom - windowRect.top) - _height) - m_borderWidth * 2;
    _width += m_borderWidth * 2;
    _height += m_borderWidth * 2 + m_titleHeight;

    HWND desktopWindow = GetDesktopWindow();
    RECT desktopRect;
    GetWindowRect(desktopWindow, &desktopRect);
    int desktopWidth = desktopRect.right - desktopRect.left;
    int desktopHeight = desktopRect.bottom - desktopRect.top;

    if (_width > desktopWidth || _height > desktopHeight)
      return false;

    posX = (desktopRect.right - _width) / 2;
    posY = (desktopRect.bottom - _height) / 2;
  }
  else
  {
    windowStyle |= WS_POPUP;

    DEVMODE devmode;
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmBitsPerPel = _colourDepth;
    devmode.dmPelsWidth = _width;
    devmode.dmPelsHeight = _height;
    devmode.dmDisplayFrequency = _refreshRate;
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    long result = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
    if (result != DISP_CHANGE_SUCCESSFUL)
      return false;

    //		DarwiniaReleaseAssert(result == DISP_CHANGE_SUCCESSFUL, "Couldn't set full screen mode of %dx%d",
    //														_width, _height);
    //      This assert goes off on many systems, regardless of success

    posX = 0;
    posY = 0;
    m_borderWidth = 1;
    m_titleHeight = 0;
  }

  // Create main window
  m_win32Specific->m_hWnd = CreateWindowA(wc.lpszClassName, wc.lpszClassName, windowStyle, posX, posY, _width, _height, NULL, NULL,
                                          g_hInstance, NULL);

  if (!m_win32Specific->m_hWnd)
    return false;

  m_mouseOffsetX = INT_MAX;

  // Initialise D3D11 device (Phase 1 / Phase 9: OpenGL removed)
  g_renderDevice = new RenderDevice();
  if (!g_renderDevice->Initialise(m_win32Specific->m_hWnd, m_screenW, m_screenH, _windowed, _waitVRT))
  {
    Neuron::Fatal("D3D11 device creation failed");
  }

  // Initialise immediate-mode emulation layer (Phase 2)
  if (g_renderDevice)
  {
    g_imRenderer = new ImRenderer();
    g_imRenderer->Initialise(g_renderDevice->GetDevice(), g_renderDevice->GetContext());

    g_renderStates = new RenderStates();
    g_renderStates->Initialise(g_renderDevice->GetDevice());

    g_textureManager = new TextureManager();
    g_textureManager->Initialise(g_renderDevice->GetDevice(), g_renderDevice->GetContext());
  }

  return true;
}

void WindowManager::DestroyWin()
{
  SAFE_DELETE(g_textureManager);
  SAFE_DELETE(g_renderStates);
  SAFE_DELETE(g_imRenderer);
  SAFE_DELETE(g_renderDevice);
  DestroyWindow(m_win32Specific->m_hWnd);
}

void WindowManager::Flip()
{
  if (g_renderDevice)
    g_renderDevice->EndFrame();
}

void WindowManager::NastyPollForMessages()
{
  MSG msg;
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    // handle or dispatch messages
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void WindowManager::NastySetMousePos(int x, int y)
{
  if (m_mouseOffsetX == INT_MAX)
  {
    RECT rect1;
    GetWindowRect(m_win32Specific->m_hWnd, &rect1);

    //RECT rect2;
    //GetClientRect(m_win32Specific->m_hWnd, &rect2);

    //int borderWidth = (m_screenW - rect2.right) / 2;
    //int menuHeight = (m_screenH - rect2.bottom) - (borderWidth * 2);
    if (m_windowed)
    {
      m_mouseOffsetX = rect1.left + m_borderWidth;
      m_mouseOffsetY = rect1.top + m_borderWidth + m_titleHeight;
    }
    else
    {
      m_mouseOffsetX = 0;
      m_mouseOffsetY = 0;
    }
  }

  SetCursorPos(x + m_mouseOffsetX, y + m_mouseOffsetY);

  extern W32InputDriver* g_win32InputDriver;
  g_win32InputDriver->SetMousePosNoVelocity(x, y);
}

void WindowManager::NastyMoveMouse(int x, int y)
{
  POINT pos;
  GetCursorPos(&pos);
  SetCursorPos(x + pos.x, y + pos.y);

  extern W32InputDriver* g_win32InputDriver;
  g_win32InputDriver->SetMousePosNoVelocity(x, y);
}

bool WindowManager::Captured() { return m_mouseCaptured; }

void WindowManager::CaptureMouse()
{
  SetCapture(m_win32Specific->m_hWnd);

  m_mouseCaptured = true;
}

void WindowManager::EnsureMouseCaptured()
{
  // Called from camera.cpp Camera::Advance
  //
  // Might not need to do anything, the intention is
  // to have the mouse/keyboard captured whenever
  // the mouse is being warped (i.e. not in
  // menu mode)
  //
  // Look carefully at input.cpp if implementing this
  // code, since that calls CaptureMouse / UncaptureMouse
  // on various input events

  if (!m_mouseCaptured)
    CaptureMouse();
}

void WindowManager::UncaptureMouse()
{
  ReleaseCapture();
  m_mouseCaptured = false;
}

void WindowManager::EnsureMouseUncaptured()
{
  // Called from camera.cpp Camera::Advance
  //
  // Might not need to do anything, the intention is
  // to have the mouse/keyboard captured whenever
  // the mouse is being warped (i.e. not in
  // menu mode)
  //
  // Look carefully at input.cpp if implementing this
  // code, since that calls CaptureMouse / UncaptureMouse
  // on various input events

  if (m_mouseCaptured)
    UncaptureMouse();
}

void WindowManager::HideMousePointer()
{
  ShowCursor(false);
  m_mousePointerVisible = false;
}

void WindowManager::UnhideMousePointer()
{
  ShowCursor(true);
  m_mousePointerVisible = true;
}

void WindowManager::WindowMoved() { m_mouseOffsetX = INT_MAX; }

void WindowManager::SuggestDefaultRes(int* _width, int* _height, int* _refresh, int* _depth)
{
  *_width = m_desktopScreenW;
  *_height = m_desktopScreenH;
  *_refresh = m_desktopRefresh;
  *_depth = m_desktopColourDepth;
}

void WindowManager::OpenWebsite(const char* _url) { ShellExecuteA(nullptr, "open", _url, nullptr, nullptr, SW_SHOWNORMAL); }

int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _cmdLine, int _iCmdShow)
{
#if defined(_DEBUG)
//  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  wchar_t filename[MAX_PATH];
  GetModuleFileNameW(nullptr, filename, MAX_PATH);
  auto path = std::wstring(filename);
  path = path.substr(0, path.find_last_of('\\'));

  FileSys::SetHomeDirectory(path);

  g_hInstance = _hInstance;

  g_windowManager = new WindowManager();

  AppMain();

  return WM_QUIT;
}
