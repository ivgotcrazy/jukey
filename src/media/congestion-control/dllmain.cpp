// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "component.h"
#include "gcc-congestion-controller.h"
#include "webrtc-tfb-adapter.h"

using namespace jukey::base;
using namespace jukey::cc;

#ifdef _WINDOWS
BOOL APIENTRY DllMain(HMODULE hModule,
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
		"GCC congestion control",
		CID_GCC_CONGESTION_CONTROLLER,
		&GccCongestionController::CreateInstance
	},
	{
		"WebRTC transport feedback adapter",
		CID_WEBRTC_TFB_ADAPTER,
		&WebrtcTfbAdapter::CreateInstance
	},
};

COMPONENT_ENTRY_IMPLEMENTATION

