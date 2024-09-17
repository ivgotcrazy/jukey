// dllmain.cpp : 定义 DLL 应用程序的入口点。
// Windows 头文件
#include "common-export.h"
#include "media-player-impl.h"

using namespace jukey::sdk;

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

extern "C"                                                                      
{                                                                               
    JUKEY_API IMediaPlayer* JUKEY_CALL CreateMediaPlayer() 
    {                                                                           
        return MediaPlayerImpl::Instance();
    }

    JUKEY_API void JUKEY_CALL ReleaseMediaPlayer()
    {
        MediaPlayerImpl::Release();
    }
}

