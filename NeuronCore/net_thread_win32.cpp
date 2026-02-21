#include "pch.h"
#include "net_thread.h"


NetRetCode NetStartThread(NetThreadFunc functionPtr)
{
	NetRetCode retVal = NetOk;
	DWORD dwID = 0;
	
	if (CreateThread(NULL, NULL, functionPtr, NULL, NULL, &dwID) == NULL)
	{
		DebugTrace("Thread creation failed");
		retVal = NetFailed;
	}

	return retVal;
}
