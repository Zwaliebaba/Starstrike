#include "pch.h"
#include "WndProcManager.h"
#include "system_info.h"

bool Direct3DInit();
void Direct3DShutdown();

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ClientEngine::Startup(const wchar_t* _gameName, HINSTANCE _hInstance, int nCmdShow)
{
    CoreEngine::Startup();
    Strings::Startup();

    // Match the legacy backend's R8G8B8A8 format; depth uses Core's D32_FLOAT default.
    Graphics::Core::Startup(DXGI_FORMAT_R8G8B8A8_UNORM);

    m_instance = _hInstance;
    m_outputSize = Graphics::Core::GetScreenSize();

    // Register class
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = _hInstance;
    wcex.lpszClassName = L"Neuron";
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    ASSERT(RegisterClassEx(&wcex));

    g_systemInfo = NEW SystemInfo;

    //Canvas::Startup();

    // Create window

    DWORD style = WS_POPUP;
    DWORD exStyle = WS_EX_APPWINDOW;

    m_hwnd = CreateWindowEx(exStyle, L"Neuron", _gameName, style, CW_USEDEFAULT, CW_USEDEFAULT, m_outputSize.Width, m_outputSize.Height,
        nullptr, nullptr, m_instance, nullptr);

    if (!m_hwnd)
    {
        LPTSTR lpMsgBuf;
        DWORD dw = GetLastError();

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, nullptr);
        std::wstring errorString = lpMsgBuf;
        LocalFree(lpMsgBuf);

        Fatal(L"CreateWindowEx failed with error {:d}: {:s}", dw, errorString);
    }

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    // Create swap chain and window-size-dependent resources via Core.
    Graphics::Core::SetWindow(m_hwnd, rc.right - rc.left, rc.bottom - rc.top);

    // Open the command list so the backend can record init-time GPU work.
    Graphics::Core::Prepare();

    Direct3DInit();
    glClear(GL_COLOR_BUFFER_BIT);

    ShowWindow(m_hwnd, nCmdShow);
    ShowCursor(FALSE);
}

void ClientEngine::StartGame(const com_ptr<GameMain>& _gameMain)
{
    m_main = _gameMain;
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(m_main.get()));
    m_main->Startup();
}

void ClientEngine::Shutdown()
{
    if (m_main)
    {
        m_main->Shutdown();
        m_main = nullptr;
    }

    //Canvas::Shutdown();

    Direct3DShutdown();

    Graphics::Core::Shutdown();

    Strings::Shutdown();
    CoreEngine::Shutdown();
}

Windows::Foundation::Point ClientEngine::OutputTopLeft()
{
    POINT pt = { 0, 0 };
    ClientToScreen(m_hwnd, &pt);
    return { static_cast<float>(pt.x), static_cast<float>(pt.y) };
}

static void HandlePointer(GameMain* _app, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    const UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
    POINTER_INFO pi{};
    if (!GetPointerInfo(pointerId, &pi))
        return;

    // Ignore mouse input without primary button pressed
    if (pi.pointerType == PT_MOUSE && (pi.pointerFlags & POINTER_FLAG_FIRSTBUTTON) == 0)
        return;

    ASSERT(_app != nullptr);

    POINT pt = pi.ptPixelLocation;
    ScreenToClient(hwnd, &pt);

    if (pi.pointerFlags & POINTER_FLAG_DOWN)
        _app->AddTouch(pointerId, Windows::Foundation::Point(static_cast<float>(pt.x), static_cast<float>(pt.y)));
    else if (pi.pointerFlags & POINTER_FLAG_UPDATE)
        _app->UpdateTouch(pointerId, Windows::Foundation::Point(static_cast<float>(pt.x), static_cast<float>(pt.y)));
    else if (pi.pointerFlags & POINTER_FLAG_UP)
        _app->RemoveTouch(pointerId);
}

LRESULT CALLBACK WndProc(const HWND _hWnd, const UINT _message, WPARAM _wParam, const LPARAM _lParam)
{
    static bool s_inSizemove = false;
    static bool s_inSuspend = false;
    static bool s_minimized = false;
    static bool s_fullscreen = false;

    auto* game = reinterpret_cast<GameMain*>(GetWindowLongPtr(_hWnd, GWLP_USERDATA));

    switch (_message)
    {
        case WM_CREATE:
            if (_lParam)
            {
                const auto params = reinterpret_cast<LPCREATESTRUCTW>(_lParam);
                SetWindowLongPtr(_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
            }
            // EnableMouseInPointer(TRUE);
            break;

        case WM_POINTERDOWN:
        case WM_POINTERUPDATE:
        case WM_POINTERUP:
            HandlePointer(game, _hWnd, _wParam, _lParam);
            return 0;

        case WM_CLOSE:
            DestroyWindow(_hWnd);
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(_hWnd, &ps);
            EndPaint(_hWnd, &ps);
            return 0;
        }

        case WM_DISPLAYCHANGE:
            if (game)
                game->OnDisplayChange();
            break;

        case WM_MOVE:
            if (game)
                game->OnWindowMoved();
            break;

        case WM_SIZE:
            if (_wParam == SIZE_MINIMIZED)
            {
                if (!s_minimized)
                {
                    s_minimized = true;
                    if (!s_inSuspend && game)
                        game->OnSuspending();
                    s_inSuspend = true;
                }
            }
            else if (s_minimized)
            {
                s_minimized = false;
                if (s_inSuspend && game)
                    game->OnResuming();
                s_inSuspend = false;
            }
            else if (!s_inSizemove && game)
                game->OnWindowSizeChanged(LOWORD(_lParam), HIWORD(_lParam));
            break;

        case WM_ENTERSIZEMOVE:
            s_inSizemove = true;
            break;

        case WM_EXITSIZEMOVE:
            s_inSizemove = false;
            if (game)
            {
                RECT rc;
                GetClientRect(_hWnd, &rc);

                game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
            }
            break;

        case WM_GETMINMAXINFO:
            if (_lParam)
            {
                auto* info = reinterpret_cast<MINMAXINFO*>(_lParam);
                info->ptMinTrackSize.x = 320;
                info->ptMinTrackSize.y = 200;
            }
            break;

        case WM_ACTIVATEAPP:
            if (game)
            {
                if (_wParam)
                    game->OnActivated();
                else
                    game->OnDeactivated();
            }
            break;

        case WM_POWERBROADCAST:
            switch (_wParam)
            {
                case PBT_APMQUERYSUSPEND:
                    if (!s_inSuspend && game)
                        game->OnSuspending();
                    s_inSuspend = true;
                    return TRUE;

                case PBT_APMRESUMESUSPEND:
                    if (!s_minimized)
                    {
                        if (s_inSuspend && game)
                            game->OnResuming();
                        s_inSuspend = false;
                    }
                    return TRUE;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_SYSKEYDOWN:
            if (_wParam == VK_RETURN && (_lParam & 0x60000000) == 0x20000000)
            {
                // Implements the classic ALT+ENTER fullscreen toggle
                if (s_fullscreen)
                {
                    SetWindowLongPtr(_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                    SetWindowLongPtr(_hWnd, GWL_EXSTYLE, 0);

                    ShowWindow(_hWnd, SW_SHOWNORMAL);

                    const auto outputSize = ClientEngine::OutputSize();
                    SetWindowPos(_hWnd, HWND_TOP, 0, 0, outputSize.Width, outputSize.Height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
                else
                {
                    SetWindowLongPtr(_hWnd, GWL_STYLE, WS_POPUP);
                    SetWindowLongPtr(_hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

                    SetWindowPos(_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

                    ShowWindow(_hWnd, SW_SHOWMAXIMIZED);
                }

                if (game)
                {
                    RECT rc;
                    GetClientRect(_hWnd, &rc);

                    game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
                }

                s_fullscreen = !s_fullscreen;
            }
            break;

        case WM_MENUCHAR:
            // A menu is active and the user presses a key that does not correspond
            // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
            return MAKELRESULT(0, MNC_CLOSE);
    }

    if (WndProcManager::WndProc(_hWnd, _message, _wParam, _lParam) == -1)
        return DefWindowProc(_hWnd, _message, _wParam, _lParam);

    return 0;
}

void ClientEngine::OnDeviceLost() {
}

void ClientEngine::OnDeviceRestored() {
}
