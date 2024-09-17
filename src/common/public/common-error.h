#pragma once

namespace jukey::com
{

/*
* ErrCode bits define
|---------------------------------------------------------------|
|0                   1                   2                   3  |
|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     module    |   sub-module  |              code             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
* --------------------------------------------------------------+
* | module            | sub-module                              |
* +-------------------------------------------------------------+
* | 0x00 | common     | 0x00 | common                           |
* |      |            |-----------------------------------------|
* |      |            | 0x01 | util                             |
* |      |            |-----------------------------------------|
* |      |            | 0x02 | timer                            |
* |      |            |-----------------------------------------|
* |      |            | 0x03 | property                         |
* |      |            |-----------------------------------------|
* |      |            | 0x04 | protocol                         |
* |      |            |-----------------------------------------|
* |      |            | 0x05 | amqp-client                      |
* +-------------------------------------------------------------+
* | 0x01 | core       | 0x01 | base-frame                       |
* |      |            |-----------------------------------------|
* |      |            | 0x02 | net-frame                        |
* |      |            |-----------------------------------------|
* |      |            | 0x03 | media-streamer                   |
* +-------------------------------------------------------------+
* | 0x02 | service    | 0x01 | proxy-service                    |
* |      |            |-----------------------------------------|
* |      |            | 0x02 | terminal-service                 |
* |      |            |-----------------------------------------|
* |      |            | 0x03 | user-service                     |
* |      |            |-----------------------------------------|
* |      |            | 0x04 | group-service                    |
* |      |            |-----------------------------------------|
* |      |            | 0x05 | stream-service                   |
* |      |            |-----------------------------------------|
* |      |            | 0x06 | stream-transport-service         |
* +-------------------------------------------------------------+
*/

#define ERR_CODE_COMMON_START         0x00000000
#define ERR_CODE_UTIL_START           0x00010000
#define ERR_CODE_TIMER_START          0x00020000
#define ERR_CODE_PROPERTY_START       0x00030000
#define ERR_CODE_PROTOCOL_START       0x00040000
#define ERR_CODE_AMQP_CLIENT_START    0x00050000

#define ERR_CODE_BASE_FRAME_START     0x01010000
#define ERR_CODE_NET_FRAME_START      0x01020000
#define ERR_CODE_STREAMER_START       0x01030000

//==============================================================================
// 
//==============================================================================
enum ErrCode
{
	ERR_CODE_OK                      = ERR_CODE_COMMON_START + 0,
	ERR_CODE_FAILED                  = ERR_CODE_COMMON_START + 1,
	ERR_CODE_INVALID_PARAM           = ERR_CODE_COMMON_START + 2,
	ERR_CODE_TIMEOUT                 = ERR_CODE_COMMON_START + 3,

	ERR_CODE_SESSION_NOT_EXIST       = ERR_CODE_NET_FRAME_START + 1,
	ERR_CODE_ALLOC_SESSION_ID_FAILED = ERR_CODE_NET_FRAME_START + 2,
	ERR_CODE_NEGOTIATE_FAILED        = ERR_CODE_NET_FRAME_START + 3,

	ERR_CODE_MSG_NO_PROC             = ERR_CODE_STREAMER_START + 1,
	ERR_CODE_MSG_PROC_OK             = ERR_CODE_STREAMER_START + 2,
	ERR_CODE_MSG_PROC_FAILED         = ERR_CODE_STREAMER_START + 3,
};

}