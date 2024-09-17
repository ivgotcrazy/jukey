#pragma once

#include <inttypes.h>

#pragma warning(disable:4200) // 结构体中占位符
#pragma warning(disable:4267) // 从“size_t”转换到“int”，可能丢失数据


namespace jukey::prot
{

/*******************************************************************************
 * Message type bits define (10bits)
 |-------------------|
 |0 1 2 3 4 5 6 7 8 9|
 +-+-+-+-+-+-+-+-+-+-+
 | service | command |
 +-+-+-+-+-+-+-+-+-+-+
 *
 * -----------------------------
 * | service                   |
 * -----------------------------
 * | 0x00 | reserved           |
 * |----------------------------
 * | 0x01 | proxy-service      |
 * |----------------------------
 * | 0x02 | terminal-service   |
 * |----------------------------
 * | 0x03 | user-service       |
 * |----------------------------
 * | 0x04 | group-service      |
 * |----------------------------
 * | 0x05 | stream-service     |
 * |---------------------------|
 * | 0x06 | transport-service  |
 * |---------------------------|
 *
*******************************************************************************/
#define MSG_RESERVED_START           0x000 // |0 0 0 0 0 0 0 0 0 0|
#define MSG_PROXY_SERVICE_START      0x020 // |0 0 0 0 1 0 0 0 0 0|
#define MSG_TERMINAL_SERVICE_START   0x040 // |0 0 0 1 0 0 0 0 0 0|
#define MSG_USER_SERVICE_START       0x060 // |0 0 0 1 1 0 0 0 0 0|
#define MSG_GROUP_SERVICE_START      0x080 // |0 0 1 0 0 0 0 0 0 0|
#define MSG_STREAM_SERVICE_START     0x0A0 // |0 0 1 0 1 0 0 0 0 0|
#define MSG_TRANSPORT_SERVICE_START  0x0C0 // |0 0 1 1 0 0 0 0 0 0|

//==============================================================================
// Message type
// 修改消息枚举，记得同步修改MSG_TYPE_MSG函数
//==============================================================================
enum MsgType
{
	MSG_MQ_FROM_ROUTE = MSG_RESERVED_START + 1,
	MSG_MQ_TO_ROUTE,
	MSG_MQ_BETWEEN_SERVICE,
	MSG_SERVICE_PING,
	MSG_SERVICE_PONG,

	MSG_SYNC_ROUTE_ENTRIES,

	MSG_CLIENT_REGISTER_REQ = MSG_TERMINAL_SERVICE_START + 1,
	MSG_CLIENT_REGISTER_RSP,
	MSG_CLIENT_UNREGISTER_REQ,
	MSG_CLIENT_UNREGISTER_RSP,
	MSG_CLIENT_OFFLINE_REQ,
	MSG_CLIENT_OFFLINE_RSP,
	MSG_CLIENT_OFFLINE_NOTIFY,

	MSG_USER_LOGIN_REQ = MSG_USER_SERVICE_START + 1,
	MSG_USER_LOGIN_RSP,
	MSG_USER_LOGOUT_REQ,
	MSG_USER_LOGOUT_RSP,
	MSG_USER_OFFLINE_NOTIFY,

	MSG_JOIN_GROUP_REQ = MSG_GROUP_SERVICE_START + 1,
	MSG_JOIN_GROUP_RSP,
	MSG_JOIN_GROUP_NOTIFY,
	MSG_LEAVE_GROUP_REQ,
	MSG_LEAVE_GROUP_RSP,
	MSG_LEAVE_GROUP_NOTIFY,
	MSG_PUBLISH_MEDIA_REQ,
	MSG_PUBLISH_MEDIA_RSP,
	MSG_PUBLISH_MEDIA_NOTIFY,
	MSG_PUBLISH_MEDIA_ACK,
	MSG_UNPUBLISH_MEDIA_REQ,
	MSG_UNPUBLISH_MEDIA_RSP,
	MSG_UNPUBLISH_MEDIA_NOTIFY,
	MSG_UNPUBLISH_MEDIA_ACK,

	MSG_PUBLISH_STREAM_REQ = MSG_STREAM_SERVICE_START + 1,
	MSG_PUBLISH_STREAM_RSP,
	MSG_UNPUBLISH_STREAM_REQ,
	MSG_UNPUBLISH_STREAM_RSP,
	MSG_SUBSCRIBE_STREAM_REQ,
	MSG_SUBSCRIBE_STREAM_RSP,
	MSG_UNSUBSCRIBE_STREAM_REQ,
	MSG_UNSUBSCRIBE_STREAM_RSP,
	MSG_GET_PARENT_NODE_REQ,
	MSG_GET_PARENT_NODE_RSP,
	MSG_LOGIN_SEND_CHANNEL_NOTIFY,
	MSG_LOGIN_SEND_CHANNEL_ACK,

	MSG_LOGIN_SEND_CHANNEL_REQ = MSG_TRANSPORT_SERVICE_START + 1,
	MSG_LOGIN_SEND_CHANNEL_RSP,
	MSG_LOGOUT_SEND_CHANNEL_REQ,
	MSG_LOGOUT_SEND_CHANNEL_RSP,
	MSG_LOGIN_RECV_CHANNEL_REQ,
	MSG_LOGIN_RECV_CHANNEL_RSP,
	MSG_LOGOUT_RECV_CHANNEL_REQ,
	MSG_LOGOUT_RECV_CHANNEL_RSP,
	MSG_NEGOTIATE_REQ,
	MSG_NEGOTIATE_RSP,
	MSG_START_SEND_STREAM_NOTIFY,
	MSG_START_SEND_STREAM_ACK,
	MSG_STOP_SEND_STREAM_NOTIFY,
	MSG_STOP_SEND_STREAM_ACK,
	MSG_PAUSE_RECV_STREAM_REQ,
	MSG_PAUSE_RECV_STREAM_RSP,
	MSG_RESUME_RECV_STREAM_REQ,
	MSG_RESUME_RECV_STREAM_RSP,
	MSG_STREAM_DATA,
	MSG_STREAM_FEEDBACK,
};

/******************************************************************************* 
 * MQ message header
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | v |  r  |          mt         |              len              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                              seq                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                     MQ message data(pb)                       |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                       signal message                          |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*******************************************************************************/
#pragma pack(push, 1)
struct MqMsgHdr
{
	uint16_t ver:2; // Version
	uint16_t res:4; // Reserved
	uint16_t mt:10; // Message Type
	uint16_t len;   // MQ message data length
	uint32_t seq;   // Message sequence
};
#pragma pack(pop)

/******************************************************************************* 
 * Signal message header
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                             app ID                            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           client ID                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                            user ID                            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           group ID                            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | v |c|u|g|e|         mt        |              len              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                              seq                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                         data(optional)                        |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                        extend(optional)                       |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*******************************************************************************/
#pragma pack(push, 1)
struct SigMsgHdr
{
	uint32_t app;   // App ID
	uint32_t clt;   // Client ID 
	uint32_t usr;   // User ID
	uint32_t grp;   // Group ID
	uint16_t ver:2; // Version
	uint16_t c:1;   // Clear client route entry, 1:yes, 0:no
	uint16_t u:1;   // Clear group route entry, 1:yes, 0:no
	uint16_t g:1;   // Clear user route entry, 1:yes, 0:no
	uint16_t e:1;   // Append extend data, 1:yes, 0:no
	uint16_t mt:10; // Message Type
	uint16_t len;   // Payload length
	uint32_t seq;   // Sequence
};
#pragma pack(pop)

/*******************************************************************************
 * FEC header
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |ver|   K   |   R   |   seq   |r|             group             |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           sequence                            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*******************************************************************************/
#pragma pack(push, 1)
struct FecHdr
{
	uint16_t ver:2;  // version
	uint16_t K:4;    // fec K
	uint16_t R:4;    // fec R
	uint16_t gseq:5; // sequence in group
	uint16_t rtx:1;  // retransmit
	uint16_t group;  // fec group
	uint32_t seq;    // sequence
};
#pragma pack(pop)

/*******************************************************************************
 * Segment header
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |ver|mt |st |ft |sl |tl |codec|            reserved             |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |        segment squence        |        segment length         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                         frame sequence                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           timestamp                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*******************************************************************************/
#pragma pack(push, 1)
struct SegHdr
{
	uint32_t ver:2;   // version
	uint32_t mt:2;    // media type, 0:padding, 1:audio, 2:video
	uint32_t st:2;    // segment type, 1:last segment
	uint32_t ft:2;    // frame type, 1:key frame
	uint32_t sl:2;    // spatial layer
	uint32_t tl:2;    // temporal layer
	uint32_t codec:3; // codec
	uint32_t rsv:17;  // reserved
	uint16_t slen;    // segment length (including segment header)
	uint16_t sseq;    // segment sequence
	uint32_t fseq;    // frame sequence
	uint32_t ts;      // timestamp
};
#pragma pack(pop)

/******************************************************************************* 
 * Video frame header
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | v |e|ft |sl |tl |codec|       width       |      height       |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                         frame sequence                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           timestamp                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                       video frame data                        |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*******************************************************************************/
#pragma pack(push, 1)
struct VideoFrameHdr
{
	uint32_t ver:2;   // version
	uint32_t ext:1;   // 1: has extension
	uint32_t ft:2;    // frame type, 1:key frame
	uint32_t sl:2;    // spatial layer
	uint32_t tl:2;    // temporal layer
	uint32_t codec:3; // codec
	uint32_t w:10;    // width, need x8
	uint32_t h:10;    // height, need x8
	uint32_t fseq;    // frame sequence
	uint32_t ts;      // timestamp
};
#pragma pack(pop)

//==============================================================================
// TODO: to be defined
//==============================================================================
enum H264NaluType
{
	H264_NALU_TYPE_INVALID = 0,
	H264_NALU_TYPE_IDR = 1,
};

/******************************************************************************* 
 * Audio frame header
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | V |E|codec| s | c |    power    |          reserved           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                            sequence                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           timestamp                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                       audio frame data                        |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*******************************************************************************/
#pragma pack(push, 1)
struct AudioFrameHdr
{
	uint32_t ver:2;   // Version
	uint32_t ext:1;   // 1: has extension
	uint32_t codec:3; // Audio codec ID
	uint32_t srate:2; // Sample rate, 0:8000, 1:16000, 2:48000, 3:extend
	uint32_t chnls:2; // Channel count, 1:mono, 2:stereo
	uint32_t power:7; // Audio energe power(0-100)
	uint32_t rsv:15;  // Reserved
	uint16_t fseq;    // Frame sequence
	uint32_t ts;      // Timestamp
};
#pragma pack(pop)

}
