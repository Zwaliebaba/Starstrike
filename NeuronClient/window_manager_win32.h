#ifndef INCLUDED_WINDOW_MANAGER_WIN32_H
#define INCLUDED_WINDOW_MANAGER_WIN32_H

class WindowManagerWin32
{
public:
	HWND		m_hWnd;

	WindowManagerWin32()
	:	m_hWnd(NULL)
	{
	}
};

#endif
