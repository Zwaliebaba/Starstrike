#include "pch.h"
#include "net_lib.h"




// ****************************************************************************
// Class NetLib
// ****************************************************************************

NetLib::NetLib()
{
}


NetLib::~NetLib()
{
#ifdef _WIN32
    WSACleanup();
#endif
}


bool NetLib::Initialise()
{
#ifdef _WIN32
    WORD versionRequested;
    WSADATA wsaData;
    
    versionRequested = MAKEWORD(2, 2);
 
    if (WSAStartup(versionRequested, &wsaData))
    {
        DebugTrace("WinSock startup failed");
        return false;
    }
 
    // Confirm that the WinSock DLL supports 2.2. Note that if the DLL supports
	// versions greater than 2.2 in addition to 2.2, it will still return
    // 2.2 in wVersion since that is the version we requested.                                       
    if ((LOBYTE(wsaData.wVersion) != 2) || (HIBYTE(wsaData.wVersion) != 2))
    {
        // Tell the user that we could not find a usable WinSock DLL
        WSACleanup();
        DebugTrace("No valid WinSock DLL found");
        return false; 
    }
#endif

    return true;
}
