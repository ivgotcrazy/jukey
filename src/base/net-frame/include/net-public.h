#pragma once

#include <inttypes.h>
#include <string>

#include "common-define.h"

namespace jukey::net
{

// Cross-platform define
#ifdef _WIN32
#define Socket intptr_t
#else
#define Socket int
#endif

#define INVALID_SOCKET_ID    0
#define INVALID_SERVICE_TYPE 0
#define INVALID_SESSION_ID   0
#define INVALID_LISTEN_ID    0


// Sessino identifier
typedef uint16_t SessionId;

// Listen identifier
typedef uint16_t ListenId;

// Connection identifier
typedef Socket SocketId;

//==============================================================================
// 
//==============================================================================
enum class SessionType
{
	INVALID = 0,
	RELIABLE = 1,
	UNRELIABLE = 2
};

//==============================================================================
// 
//==============================================================================
enum class SessionRole
{
	INVALID = 0,
	CLIENT,
	SERVER
};

}