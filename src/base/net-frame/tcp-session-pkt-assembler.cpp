#include "tcp-session-pkt-assembler.h"
#include "log.h"


using namespace jukey::util;

namespace
{

using namespace jukey::net;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t CalcPktLen(SesPktHdr* head)
{
	return head->len + SES_PKT_HDR_LEN;
}

}

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TcpSessionPktAssembler::TcpSessionPktAssembler(SessionId local_sid)
	: m_local_sid(local_sid)
{
	m_recv_buf = new char[m_max_len];

	LOG_INF("New TcpSessionPktAssembler, buffer total len: {}", m_max_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TcpSessionPktAssembler::~TcpSessionPktAssembler()
{
	if (!m_recv_buf) {
		delete[] m_recv_buf;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TcpSessionPktAssembler::HasEnoughSpaceForIncommingBuf(const com::Buffer& buf)
{
	if (buf.data_len + m_data_len > m_max_len) {
		LOG_ERR("[session:{}] No space for incomming data:{}, buffer len:{}",
			m_local_sid, buf.data_len, m_data_len);
		return false;
	}
	else {
		return true;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpSessionPktAssembler::MakeSessionPkt(char** data, uint32_t* len)
{
	SesPktHdr* head = (SesPktHdr*)(*data);

	SessionPktSP pkt(new SessionPkt(head->len));
	pkt->head = *head;
	pkt->buf.data_len = head->len;
	if (head->len > 0) {
		memcpy(pkt->buf.data.get(), (*data) + SES_PKT_HDR_LEN, head->len);
	}

	m_pkt_list.push_back(pkt);

	LOG_DBG("MakeSessionPkt, pt:{}, len:{}, pos:{}, psn:{}, msn:{}",
		(uint32_t)head->pt,
		head->len,
		(uint32_t)head->pos,
		head->psn,
		head->msn);

	(*data) += SES_PKT_HDR_LEN + head->len;
	(*len) -= SES_PKT_HDR_LEN + head->len;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpSessionPktAssembler::ParseDataWithNoBuffer(const com::Buffer& buf)
{
	// Parse start positon
	char* pos = (char*)DP(buf);

	// Parse data length
	uint32_t len = buf.data_len;

	// Not long enough, copy to cache for next parse
	if (len < SES_PKT_HDR_LEN) {
		if (HasEnoughSpaceForIncommingBuf(buf)) {
			memcpy(m_recv_buf, pos, len);
			m_data_len += len;
		}
		else {
			LOG_ERR("No enough space for incoming buffer!");
		}
	}
	else {
		while (len > 0) {
			SesPktHdr* head = (SesPktHdr*)pos;

			LOG_DBG("ParseDataWithNoBuffer, pt:{}, len:{}, pos:{}, psn:{}, msn:{}",
				(uint32_t)head->pt,
				head->len,
				(uint32_t)head->pos,
				head->psn,
				head->msn);

			if (len >= head->len + SES_PKT_HDR_LEN) {
				MakeSessionPkt(&pos, &len);
			}
			else {
				memcpy(m_recv_buf, pos, len);
				m_data_len += len;
				break; // copy all the rest to the receive cache
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpSessionPktAssembler::ParseDataWithBuffer(const com::Buffer& buf)
{
	// Receive buffer is not enough
	if (!HasEnoughSpaceForIncommingBuf(buf)) return;

	if (buf.data_len + m_data_len + m_data_pos > m_max_len) {
		memmove(m_recv_buf, m_recv_buf + m_data_pos, m_data_len);
		m_data_pos = 0;
	}

	// Copy to receive buffer
	memcpy(m_recv_buf + m_data_pos + m_data_len,
		(char*)buf.data.get() + buf.start_pos, buf.data_len);
	m_data_len += buf.data_len;

	// Receive buffer has enough data to be parsed
	if (CalcPktLen((SesPktHdr*)(m_recv_buf + m_data_pos)) > m_data_len) {
		return;
	}

	// parse start position
	char* data = m_recv_buf + m_data_pos;

	// data length to be parsed
	uint32_t len = m_data_len;

	while (CalcPktLen((SesPktHdr*)(data)) <= len) {
		MakeSessionPkt(&data, &len);
	}

	m_data_len = len;
	m_data_pos = (m_data_len == 0) ? 0 : (uint32_t)(data - m_recv_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpSessionPktAssembler::InputSessionData(const com::Buffer& buf)
{
	if (!CheckBuffer(buf)) { // validation
		return;
	}

	// try to parse directly
	if (m_data_len == 0) {
		ParseDataWithNoBuffer(buf);
	}
	// parse after copying while buffer is not empty
	else {
		ParseDataWithBuffer(buf);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionPktSP TcpSessionPktAssembler::GetNextSessionPkt()
{
	if (m_pkt_list.empty()) return nullptr;

	SessionPktSP pkt = m_pkt_list.front();
	m_pkt_list.pop_front();

	return pkt;
}

}