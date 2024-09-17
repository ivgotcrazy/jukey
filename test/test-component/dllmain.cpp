// dllmain.cpp : 定义 DLL 应用程序的入口点。

#ifdef _WINDOWS
#include <Windows.h>
#endif

#include "component.h"
#include "com-test.h"

using namespace jukey::base;

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

jukey::base::ComEntry com_entries[] = {
	{
		"ComTest",
		"ComTest_CID",
		test::ComTest::CreateInstance
	}
};

COMPONENT_ENTRY_IMPLEMENTATION

