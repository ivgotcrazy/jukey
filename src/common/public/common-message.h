#pragma once

#include <memory>
#include "common-error.h"

namespace jukey::com
{

/*
 * Message type bits define
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |     module    |   sub-module  |          message type         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * ---------------------------------------------------------------
 * |       module      |   sub-module                            |
 * ---------------------------------------------------------------
 * | 0x00 | common     | 0x00 | common                           |
 * ---------------------------------------------------------------
 * | 0x01 | base       | 0x01 | net-frame                        |
 * ---------------------------------------------------------------
 * | 0x02 | service    | 0x01 | proxy-service                    |
 * |      |            |------------------------------------------
 * |      |            | 0x02 | terminal-service                 |
 * |      |            |------------------------------------------
 * |      |            | 0x03 | user-service                     |
 * |      |            |------------------------------------------
 * |      |            | 0x04 | group-service                    |
 * |      |            |------------------------------------------
 * |      |            | 0x05 | stream-service                   |
 * |      |            |------------------------------------------
 * |      |            | 0x06 | transport-service                |
 * |      |            |------------------------------------------
 * |      |            | 0x07 | route-service                    |
 * ---------------------------------------------------------------
 * | 0x03 | sdk        | 0x01 | client-sdk                       |
 * ---------------------------------------------------------------
 */

#define COMMON_MSG_START            0x00000000
#define NET_FRAME_MSG_START         0x01010000
#define SERVICE_PROXY_MSG_START     0x02010000
#define SERVICE_TERMINAL_MSG_START  0x02020000
#define SERVICE_USER_MSG_START      0x02030000
#define SERVICE_GROUP_MSG_START     0x02040000
#define SERVICE_STREAM_MSG_START    0x02050000
#define SERVICE_TRANSPORT_MSG_START 0x02060000
#define SERVICE_ROUTE_MSG_START     0x02070000
#define CLIENT_SDK_MSG_START        0x03010000

//==============================================================================
// 
//==============================================================================
enum CommonMsgType
{
	COMMON_MSG_INVALID = COMMON_MSG_START,
};

}