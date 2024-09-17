#pragma once

#if defined(_WIN32) || defined(_WINDOWS)
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif // !WIN32_LEAN_AND_MEAN

	#include <windows.h>
	#define JUKEY_CALL __cdecl
	#if defined(JUKEY_EXPORT)
		#define JUKEY_API extern "C" __declspec(dllexport)
	#else
		#define JUKEY_API extern "C" __declspec(dllimport)
	#endif
#elif defined(__APPLE__)
	#define JUKEY_API __attribute__((visibility("default"))) extern "C"
	#define JUKEY_CALL
#elif defined(__ANDROID__) || defined(__linux__)
	#define JUKEY_API extern "C" __attribute__((visibility("default")))
	#define JUKEY_CALL
#else
	#define JUKEY_API extern "C"
	#define JUKEY_CALL
#endif