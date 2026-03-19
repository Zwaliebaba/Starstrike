#include "pch.h"
#include "net_thread.h"


NetRetCode NetStartThread(NetThreadFunc functionPtr)
{
	NetRetCode retVal = NetOk;
	DWORD dwID = 0;
	
	if (CreateThread(nullptr, 0, functionPtr, nullptr, 0, &dwID) == nullptr)
	{
		DebugTrace("Thread creation failed");
		retVal = NetFailed;
	}

	return retVal;
}
