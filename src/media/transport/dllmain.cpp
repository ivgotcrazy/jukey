// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "component.h"
#include "stream-exchange.h"
#include "stream-receiver.h"
#include "stream-sender.h"
#include "server-stream-sender.h"

using namespace jukey::base;
using namespace jukey::txp;

#ifdef _WINDOWS
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif

ComEntry com_entries[] = {
	{
		"stream exchange",
		CID_STREAM_EXCHAGE,
		&StreamExchange::CreateInstance
	},
	{
		"stream receiver",
		CID_STREAM_RECEIVER,
		&StreamReceiver::CreateInstance
	},
	{
		"stream sender",
		CID_STREAM_SENDER,
		&StreamSender::CreateInstance
	},
	{
		"servedr stream sender",
		CID_SERVER_STREAM_SENDER,
		&ServerStreamSender::CreateInstance
	}
};

COMPONENT_ENTRY_IMPLEMENTATION

