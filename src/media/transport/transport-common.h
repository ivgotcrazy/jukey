#pragma once

#include <map>

#include "if-pin.h"
#include "if-stream-receiver.h"
#include "if-stream-sender.h"
#include "protocol.h"

namespace jukey::txp
{

//==============================================================================
//
//==============================================================================
#define LOG_FEC_FRAME_INF(str, buf) \
{ \
	prot::FecHdr* fec_hdr = (prot::FecHdr*)DP(buf); \
	prot::SegHdr* seg_hdr = (prot::SegHdr*)(DP(buf) + FEC_HDR_LEN); \
	LOG_INF("{}, seq:{}, k:{}, r:{}, rtx:{}, group:{}, gseq:{}, " \
		"fseq:{}, sseq:{}, mt:{}, st:{}", \
		str, fec_hdr->seq, FEC_K(fec_hdr), FEC_R(fec_hdr), (uint32_t)fec_hdr->rtx, \
		fec_hdr->group, (uint32_t)fec_hdr->gseq, seg_hdr->fseq, seg_hdr->sseq, \
		(uint32_t)seg_hdr->mt, (uint32_t)seg_hdr->st) \
}

#define LOG_FEC_FRAME_DBG(str, buf) \
{ \
	prot::FecHdr* fec_hdr = (prot::FecHdr*)DP(buf); \
	prot::SegHdr* seg_hdr = (prot::SegHdr*)(DP(buf) + FEC_HDR_LEN); \
	LOG_DBG("{}, seq:{}, k:{}, r:{}, rtx:{}, group:{}, gseq:{}, " \
		"fseq:{}, sseq:{}, mt:{}, st:{}", \
		str, fec_hdr->seq, FEC_K(fec_hdr), FEC_R(fec_hdr), (uint32_t)fec_hdr->rtx, \
		fec_hdr->group, (uint32_t)fec_hdr->gseq, seg_hdr->fseq, seg_hdr->sseq, \
		(uint32_t)seg_hdr->mt, (uint32_t)seg_hdr->st) \
}

//==============================================================================
//
//==============================================================================
struct ReceiverInfo
{
	stmr::PinCaps avai_caps;
	std::string nego_cap;
	IStreamReceiver* stream_receiver = nullptr;
};

//==============================================================================
//
//==============================================================================
struct SenderInfo
{
	prot::SigMsgHdr sig_hdr;
	stmr::PinCaps avai_caps;
	std::string nego_cap;
	IServerStreamSender* stream_sender = nullptr;
};

typedef std::shared_ptr<SenderInfo> SenderInfoSP;

//==============================================================================
// Negotiation state
//==============================================================================
enum class NegoState
{
	NEGO_STATE_INIT = 0,
	NEGO_STATE_WAIT = 1,
	NEGO_STATE_DONE = 2,
};

//==============================================================================
//
//==============================================================================
enum class SegPktType
{
	SPT_PADDING = 0,
	SPT_AUDIO,
	SPT_VIDEO
};

/*******************************************************************************
 * Feedback
 * 1: State
 * 2: NACK
 * 3: RTT request
 * 4: RTT response
 * 5: Transport-CC
 * 6: VideoInfo
*******************************************************************************/

enum FeedbackType
{
	FBT_INVALID = 0,
	FBT_STATE = 1,
	FBT_NACK = 2,
	FBT_RTT_REQ = 3,
	FBT_RTT_RSP = 4,
	FBT_TRANSPORT = 5, // 使用 WebRTC 格式
	FBT_VIDEO_INFO = 6,
};

#pragma pack(push, 1)
struct TLV
{
	uint8_t type; // 1:STATE, 2:NACK, 3: RTT request, 4:Transport-CC
	uint16_t length;
	uint8_t value[];
};

////////////////////////////////////////////////////////////////////////////////

struct NackFB
{
	uint32_t* seqs;
};

////////////////////////////////////////////////////////////////////////////////

struct StateFB
{
	// for gcc
	uint32_t start_time;
	uint32_t end_time;
	uint32_t recv_count;
	uint32_t lost_count;

	uint16_t rtt; // round trip time
	uint8_t olr;  // original loss rate
	uint8_t flr;  // fec loss rate
	uint8_t nlr;  // nack loss rate
	uint8_t clc;  // maximum consecutive loss count
	uint8_t flc;  // frame loss count
	uint8_t fc;   // frame count
};

////////////////////////////////////////////////////////////////////////////////

struct RttReq
{
	uint32_t seq;
	uint64_t ts;
};

struct RttRsp
{
	uint32_t seq;
	uint64_t ts;
};

////////////////////////////////////////////////////////////////////////////////

struct VideoInfoFB
{
	uint8_t spatial_layers;
	uint8_t temporal_layers;
	uint16_t* params;
};
#pragma pack(pop)


////////////////////////////////////////////////////////////////////////////////

StateFeedback ToStateFeedback(const StateFB& fb);

}