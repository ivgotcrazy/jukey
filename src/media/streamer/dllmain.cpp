// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "component.h"
#include "stream-pipeline.h"
#include "src-pin.h"
#include "sink-pin.h"
#include "bitrate-allocate-mgr.h"

using namespace jukey::base;
using namespace jukey::stmr;

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
		"pipeline",
		CID_PIPELINE,
		&StreamPipeline::CreateInstance
	},
	{
		"src-pin",
		CID_SRC_PIN,
		&SrcPin::CreateInstance
	},
	{
		"sink-pin",
		CID_SINK_PIN,
		&SinkPin::CreateInstance
	},
	{
		"bitrate-allocate-mgr",
		CID_BITRATE_ALLOCATE_MGR,
		&BitrateAllocateMgr::CreateInstance
	},
};

COMPONENT_ENTRY_IMPLEMENTATION
