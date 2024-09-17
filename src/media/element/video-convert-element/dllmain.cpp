﻿// dllmain.cpp : 定义 DLL 应用程序的入口点。
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "component.h"
#include "video-convert-element.h"

using namespace jukey::base;
using namespace jukey::stmr;

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

ComEntry com_entries[] = {
	{
		"video-convert-element",
		CID_VIDEO_CONVERT,
		&VideoConvertElement::CreateInstance
	}
};

COMPONENT_ENTRY_IMPLEMENTATION

