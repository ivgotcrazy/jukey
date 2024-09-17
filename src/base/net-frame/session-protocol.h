#pragma once

#include <list>
#include <vector>
#include "common-struct.h"
#include "if-session-mgr.h"
#include "net-common.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
enum SessionState
{
	SESSION_STATE_INVALID         = 0,
	SESSION_STATE_INITIALIZED     = 1,
	SESSION_STATE_HANDSHAKING     = 2,
	SESSION_STATE_TRANSPORTING    = 3,
	SESSION_STATE_RECONNECTING    = 4,
	SESSION_STATE_CLOSED          = 5,
};

//==============================================================================
// 
//==============================================================================
enum SessionPktType
{
	SESSION_PKT_INVALID       = 0,
	SESSION_PKT_HANDSHAKE     = 1,
	SESSION_PKT_DATA          = 2,
	SESSION_PKT_ACK           = 3,
	SESSION_PKT_ACK2          = 4,
	SESSION_PKT_REPORT        = 5,
	SESSION_PKT_KEEP_ALIVE    = 6,
	SESSION_PKT_RECONNECT_REQ = 7,
	SESSION_PKT_RECONNECT_RSP = 8,
	SESSION_PKT_CLOSE         = 9,
};

/*
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | f |r|     pt    |pos|   rsv   |             length            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |       source session ID       |     destination session ID    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                    packet sequence number                     |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                    message sequence number                    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           timestamp                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                         data(optional)                        |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * f(2bits): flag, 00:session protocol, 01:fec protocol, others:undefined
 * pt(6bits): packet type
 * pos(2bits): 00:middle packet, 10:first packet, 01:last packet, 11:only packet
 * 
 */

//==============================================================================
// 
//==============================================================================
struct SesPktHdr
{
	uint16_t flg:2; // flag
	uint16_t rtx:1; // retransmit
	uint16_t pt:6;  // packet type
	uint16_t pos:2; // position of packet
	uint16_t rsv:5; // reserved
	uint16_t len;   // packet len, excluding header
	uint16_t src;   // source session ID
	uint16_t dst;   // dest session ID
	uint32_t psn;   // packet sequence number
	uint32_t msn;   // message sequence number
	uint32_t ts;    // timestamp
};

/* 
 * 1: handshake
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  v  |  st |  kai  |ft |  rsv  |          service type         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                               IP                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * v(3bits): version
 * st(3bits): session type
 * kai(4bits): keep alive interval, in second
 * ft(2bits): fec type, 0:none, 1:luigi fec
 * rsv(6bits): reserved
 * service type(16bits): which service to connect
 * IP(32bits): peer's IP address
 */

//==============================================================================
//
//==============================================================================
struct HandshakeData
{
	uint16_t version : 3;
	uint16_t session_type : 3;
	uint16_t ka_interval : 4;
	uint16_t fec_type : 2;
	uint16_t reserved : 4;
	uint16_t service_type;
	uint32_t peer_ip;
};

/* 
 * 2: data
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                              ...                              |
 |                              ...                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * 3: ack
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                              ...                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                        sequence numbers                       |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

//==============================================================================
// 
//==============================================================================
struct AckData
{
	std::vector<uint32_t> sns;
};

/* 4: ack2
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                             no data                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 * 5: report
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                          window size                          |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                              RTT                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                         RTT variance                          |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                         receive rate                          |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                    estimated link capacity                    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

//==============================================================================
// 
//==============================================================================
struct ReportData
{
	uint32_t wnd;
	uint32_t rtt;
	uint32_t rtt_var;
	uint32_t recv_rate;     // in packets/second
	uint32_t link_cap;      // in packets/second
	uint32_t fec_loss_rate; // multiply 100
	uint32_t ses_loss_rate; // multiply 100
};

/* 6: keep-alive
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                             no data                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * 7: reconnect request
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                             no data                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * 8: reconnect response
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                             no data                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * 9: close
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                             no data                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

#define SES_PKT_HDR_LEN sizeof(SesPktHdr)

//==============================================================================
// 
//==============================================================================
struct SessionPkt
{
	SessionPkt() 
	{
		memset(&head, 0, sizeof(head));
	}

	SessionPkt(uint32_t buf_len) : buf(buf_len) 
	{
		memset(&head, 0, sizeof(head));
	}

	SessionPkt(const SesPktHdr& h, const com::Buffer& b)
		: head(h), buf(b) {}

	SesPktHdr head;
	com::Buffer buf;
	uint64_t ts = 0;
};
typedef std::shared_ptr<SessionPkt> SessionPktSP;

//==============================================================================
// 
//==============================================================================
class SessionProtocol
{
public:
	static com::Buffer BuildHandshakePkt(SessionId src,
		SessionId dst,
		const HandshakeData& data);

	static com::Buffer BuildSessionDataPkt(bool rtx,
		SessionId src,
		SessionId dst,
		PktPos pkt_pos, 
		uint32_t psn, 
		uint32_t msn,
		const char* data,
		uint32_t len,
		uint32_t head_rsv_len = 0,
		uint32_t fix_data_len = 0);

	static com::Buffer BuildAckPkt(SessionId src,
		SessionId dst,
		uint32_t pkt_sn,
		uint32_t ack_sn,
		std::vector<uint32_t> sns);

	static com::Buffer BuildAck2Pkt(SessionId src,
		SessionId dst,
		uint32_t ack_sn,
		uint64_t ts);

	static com::Buffer BuildReportPkt(SessionId src,
		SessionId dst,
		const ReportData& data);

	static com::Buffer BuildKeepAlivePkt(SessionId src, SessionId dst);
	static com::Buffer BuildSessionClosePkt(SessionId src, SessionId dst);
	static com::Buffer BuildReconnectReqPkt(SessionId src, SessionId dst);
	static com::Buffer BuildReconnectRspPkt(SessionId src, SessionId dst);
	
	static HandshakeData ParseHandshakeData(const com::Buffer& buf);
	static ReportData ParseReportData(const com::Buffer& buf);
	static AckData ParseAckData(const com::Buffer& buf);

	static void DumpSessionPkt(SessionId sid, const SessionPktSP& pkt);
	static void DumpSessionPktHdr(SessionId sid, const SesPktHdr& pkt_hdr);
};

}