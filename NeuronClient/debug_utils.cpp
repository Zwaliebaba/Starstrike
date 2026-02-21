#include "pch.h"
#include "debug_utils.h"


void DarwiniaReleaseAssert(bool _condition, char const *_fmt, ...)
{
	if (!_condition)
	{
		char buf[512];
		va_list ap;
		va_start (ap, _fmt);
		vsprintf(buf, _fmt, ap);

		DWORD rc = GetLastError();
		if (rc != ERROR_SUCCESS) {
			LPVOID lpMsgBuf;

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				rc,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0, NULL );

			sprintf( buf + strlen(buf), "\nLast error: %s (%d)", lpMsgBuf, rc );
		}
		ShowCursor(true);
		MessageBoxA(NULL, buf, "Fatal Error", MB_OK);
#ifndef _DEBUG
		exit(-1);
#else
		_ASSERT(_condition);
#endif
	}
}
