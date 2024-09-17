#include "session-protocol.h"
#include "nlohmann/json.hpp"
#include "common/util-time.h"
#include "common-config.h"
#include "log.h"

using json = nlohmann::json;

#define MAX_ACK_SIZE ((MAX_FRAG_SIZE - SES_PKT_HDR_LEN) / 8)

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildHandshakePkt(SessionId src, SessionId dst, 
  const HandshakeData& data)
{
	com::Buffer buf(SES_PKT_HDR_LEN + sizeof(data));

	char* pos = (char*)buf.data.get();

	SesPktHdr* head = (SesPktHdr*)pos;
  head->flg = 0;
	head->pt  = SESSION_PKT_HANDSHAKE;
  head->pos = 0;
	head->rsv = 0;
	head->len = (uint16_t)sizeof(data);
	head->src = src;
	head->dst = dst;
	head->psn = 0;
  head->msn = 0;
	head->ts  = static_cast<uint32_t>(util::Now());

	pos += SES_PKT_HDR_LEN;

	memcpy(pos, (char*)&data, sizeof(data));

	buf.data_len = (uint32_t)(SES_PKT_HDR_LEN + sizeof(data));

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildSessionDataPkt(bool rtx, SessionId src, 
  SessionId dst, PktPos pkt_pos, uint32_t psn, uint32_t msn, const char* data, 
  uint32_t len, uint32_t head_rsv_len, uint32_t fix_data_len)
{
  uint32_t buf_len = 0;
  if (fix_data_len == 0) {
    buf_len = SES_PKT_HDR_LEN + len + head_rsv_len;
  }
  else {
    if (fix_data_len >= len) {
      buf_len = fix_data_len + SES_PKT_HDR_LEN + head_rsv_len;
    }
    else {
      LOG_ERR("Invalid fix data len:{}, len:{}", fix_data_len, len);
      return nullptr;
    }
  }
	
  com::Buffer buf(buf_len);
  
	char* pos = (char*)buf.data.get() + head_rsv_len;

	SesPktHdr* head = (SesPktHdr*)pos;
  head->flg = 0;
  head->rtx = rtx ? 1 : 0;
	head->pt  = SESSION_PKT_DATA;
  head->pos = pkt_pos;
	head->rsv = 0;
	head->len = len;
	head->src = src;
	head->dst = dst;
	head->psn = psn;
  head->msn = msn;
	head->ts  = static_cast<uint32_t>(util::Now());

	pos += SES_PKT_HDR_LEN;

	memcpy(pos, data, len);

	//buf.data_len = SES_PKT_HDR_LEN + len;
 // buf.start_pos = head_rsv_len;

  buf.data_len = buf_len;
  buf.start_pos = 0;

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildAckPkt(SessionId src, SessionId dst,
  uint32_t pkt_sn, uint32_t ack_sn, std::vector<uint32_t> sns)
{
	com::Buffer buf(SES_PKT_HDR_LEN + (uint32_t)(sns.size() * 4));

	char* pos = (char*)buf.data.get();

	SesPktHdr * head = (SesPktHdr*)pos;
  head->flg = 0;
	head->pt  = SESSION_PKT_ACK;
  head->pos = 0;
	head->rsv = 0;
	head->len = (uint16_t)(sns.size() * 4);
	head->src = src;
	head->dst = dst;
	head->psn = pkt_sn;
  head->msn = ack_sn;
	head->ts  = (uint32_t)util::Now();

	pos += SES_PKT_HDR_LEN;
	buf.data_len = SES_PKT_HDR_LEN + (uint32_t)(sns.size() * 4);

  for (auto sn : sns) {
    *((uint32_t*)pos) = sn;
    pos += 4;
    LOG_DBG("[session:{}] Add sn:{}", src, sn);
  }

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildAck2Pkt(SessionId src, SessionId dst,
  uint32_t ack_sn, uint64_t ts)
{
  com::Buffer buf(SES_PKT_HDR_LEN);

  SesPktHdr* head = (SesPktHdr*)buf.data.get();
  head->flg = 0;
  head->pt  = SESSION_PKT_ACK2;
  head->pos = 0;
  head->rsv = 0;
  head->len = 0;
  head->src = src;
  head->dst = dst;
  head->psn = 0;
  head->msn = ack_sn;
  head->ts = static_cast<uint32_t>(ts);

  buf.data_len = SES_PKT_HDR_LEN;

  return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildReportPkt(SessionId src, SessionId dst,
  const ReportData& data)
{
  com::Buffer buf(SES_PKT_HDR_LEN + sizeof(data));

  char* pos = (char*)buf.data.get();

  SesPktHdr* head = (SesPktHdr*)pos;
  head->flg = 0;
  head->pt  = SESSION_PKT_REPORT;
  head->pos = 0;
  head->rsv = 0;
  head->len = (uint16_t)sizeof(data);
  head->src = src;
  head->dst = dst;
  head->psn = 0;
  head->msn = 0;
  head->ts = static_cast<uint32_t>(util::Now());

  pos += SES_PKT_HDR_LEN;

  memcpy(pos, (char*)&data, sizeof(data));

  buf.data_len = (uint32_t)(SES_PKT_HDR_LEN + sizeof(data));

  return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildKeepAlivePkt(SessionId src, SessionId dst)
{
	com::Buffer buf(SES_PKT_HDR_LEN);

	SesPktHdr * head = (SesPktHdr*)buf.data.get();
  head->flg = 0;
	head->pt  = SESSION_PKT_KEEP_ALIVE;
  head->pos = 0;
	head->rsv = 0;
	head->len = 0;
	head->src = src;
	head->dst = dst;
	head->psn = 0;
  head->msn = 0;
	head->ts  = static_cast<uint32_t>(util::Now());

	buf.data_len = SES_PKT_HDR_LEN;

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildSessionClosePkt(SessionId src, SessionId dst)
{
	com::Buffer buf(SES_PKT_HDR_LEN);

	SesPktHdr* head = (SesPktHdr*)buf.data.get();
  head->flg = 0;
	head->pt  = SESSION_PKT_CLOSE;
  head->pos = 0;
	head->rsv = 0;
	head->len = 0;
	head->src = src;
	head->dst = dst;
	head->psn = 0;
  head->msn = 0;
	head->ts  = static_cast<uint32_t>(util::Now());

	buf.data_len = SES_PKT_HDR_LEN;

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildReconnectReqPkt(SessionId src, SessionId dst)
{
  com::Buffer buf(SES_PKT_HDR_LEN);

  SesPktHdr* head = (SesPktHdr*)buf.data.get();
  head->flg = 0;
  head->pt  = SESSION_PKT_RECONNECT_REQ;
  head->pos = 0;
  head->rsv = 0;
  head->len = 0;
  head->src = src;
  head->dst = dst;
  head->psn = 0;
  head->msn = 0;
  head->ts = static_cast<uint32_t>(util::Now());

  buf.data_len = SES_PKT_HDR_LEN;

  return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer SessionProtocol::BuildReconnectRspPkt(SessionId src, SessionId dst)
{
  com::Buffer buf(SES_PKT_HDR_LEN);

  SesPktHdr* head = (SesPktHdr*)buf.data.get();
  head->flg = 0;
  head->pt  = SESSION_PKT_RECONNECT_RSP;
  head->pos = 0;
  head->rsv = 0;
  head->len = 0;
  head->src = src;
  head->dst = dst;
  head->psn = 0;
  head->msn = 0;
  head->ts = static_cast<uint32_t>(util::Now());

  buf.data_len = SES_PKT_HDR_LEN;

  return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionProtocol::DumpSessionPkt(SessionId sid, const SessionPktSP& pkt)
{
	LOG_DBG("[session:{}] flg:{}, pt:{}, pos:{}, len:{}, src:{}, dst:{}, psn:{},"
    " msn:{}, ts:{}",
    sid,
    (uint8_t)pkt->head.flg,
		(uint8_t)pkt->head.pt,
    (uint8_t)pkt->head.pos,
		pkt->head.len,
		pkt->head.src,
		pkt->head.dst,
    pkt->head.psn,
		pkt->head.msn,
		pkt->head.ts);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionProtocol::DumpSessionPktHdr(SessionId sid, const SesPktHdr& pkt_hdr)
{
  LOG_DBG("[session:{}] flg:{}, pt:{}, pos:{}, len:{}, src:{}, dst:{}, psn:{},"
    " msn:{}, ts:{}",
    sid,
    (uint8_t)pkt_hdr.flg,
    (uint8_t)pkt_hdr.pt,
    (uint8_t)pkt_hdr.pos,
    pkt_hdr.len,
    pkt_hdr.src,
    pkt_hdr.dst,
    pkt_hdr.psn,
    pkt_hdr.msn,
    pkt_hdr.ts);
}

//------------------------------------------------------------------------------
// Note: no consideration of byte order
//------------------------------------------------------------------------------
HandshakeData SessionProtocol::ParseHandshakeData(const com::Buffer& buf)
{
	return *((HandshakeData*)(buf.data.get() + buf.start_pos));
}

//------------------------------------------------------------------------------
// Note: no consideration of byte order
//------------------------------------------------------------------------------
ReportData SessionProtocol::ParseReportData(const com::Buffer& buf)
{
  return *((ReportData*)(buf.data.get() + buf.start_pos));
}

//------------------------------------------------------------------------------
// Note: no consideration of byte order
//------------------------------------------------------------------------------
AckData SessionProtocol::ParseAckData(const com::Buffer& buf)
{
  AckData data;
  for (uint32_t pos = 0; pos < buf.data_len; pos += 4) {
    data.sns.push_back(*((uint32_t*)(buf.data.get() + buf.start_pos + pos)));
  }

  return data;
}

}