// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "component.h"
#include "group-service.h"

using namespace jukey::base;
using namespace jukey::srv;

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
		"group service",
		CID_GROUP_SERVICE,
		&GroupService::CreateInstance
	}
};

COMPONENT_ENTRY_IMPLEMENTATION

