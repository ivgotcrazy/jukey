#include "sending-controller.h"
#include "session-protocol.h"
#include "common-config.h"
#include "if-sending-queue.h"
#include "common/util-time.h"
#include "data-splitter.h"
#include "fec-protocol.h"
#include "fec/luigi-fec-encoder.h"
#include "log.h"


using namespace jukey::com;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SendingController::SendingController(const SessionParam& param,
	IRttFilterSP filter, ISessionPktSenderSP sender, LinkCapEstimatorSP estimator)
	: m_sess_param(param)
	, m_rtt_filter(filter)
	, m_session_pkt_sender(sender)
	, m_link_cap_estimator(estimator)
{
	m_data_splitter.reset(new DataSplitter(param));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendingController::Update()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendingController::SetFecParam(const FecParam& param)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (param.r != 0) {
		if (param.k == 0) {
			LOG_ERR("[session:{}] Set invalid fec param, k:{}, r:{}",
				m_sess_param.local_sid, param.k, param.r);
		}
		else {
			if (param.k == m_fec_param.k && param.r == m_fec_param.r) {
				LOG_DBG("[session:{}] Set the same fec param, k:{}, r:{}",
					m_sess_param.local_sid, param.k, param.r);
				return;
			}

			m_fec_encoder.reset(new util::LuigiFecEncoder(param.k, param.r));

			LOG_INF("[session:{}] Create fec encoder, [{}:{}] -> [{}:{}]",
				m_sess_param.local_sid,
				m_fec_param.k,
				m_fec_param.r,
				param.k,
				param.r);

			m_fec_param.k = param.k;
			m_fec_param.r = param.r;
		}
	}
	else {
		if (m_fec_param.r != 0) {
			LOG_INF("[session:{}] Destroy fec encoder, [{}:{}] -> [{}:{}]",
				m_sess_param.local_sid,
				m_fec_param.k,
				m_fec_param.r,
				param.k,
				param.r);

			m_fec_param.k = param.k;
			m_fec_param.r = param.r;

			m_fec_encoder.reset();
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SendingController::OnSessionDataPush(const com::Buffer& buf)
{
	if (m_wait_que_size >= SEND_CACHE_QUEUE_MAX_SIZE) {
		LOG_WRN("[session:{}] Send wait queue is full, size:{}",
			m_sess_param.local_sid, m_wait_que_size);
		m_pending = true;
		return false;
	}

	uint32_t count = m_data_splitter->SplitSessionData(buf, m_send_wait_que);
	m_wait_que_size += count;

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendingController::TryEncodingFecData()
{
	if (!m_fec_encoder) {
		LOG_DBG("[session:{}] Invalid fec encoder!", m_sess_param.local_sid);
		return;
	}

	uint8_t** src_data = new uint8_t * [m_fec_param.k];
	uint8_t** red_data = new uint8_t * [m_fec_param.r];

	uint32_t head_len = SES_PKT_HDR_LEN + FEC_PKT_HDR_LEN;

	while (m_cache_que_size >= m_fec_param.k) {
		// Prepare source data
		uint32_t index = 0;
		for (auto i = m_send_cache_que.begin(); i != m_send_cache_que.end(); i++) {
			src_data[index] = i->data.get() + FEC_PKT_HDR_LEN;
			if (++index >= m_fec_param.k)
				break;
		}

		// Alloc redundant data
		std::list<com::Buffer> red_buf;
		for (uint32_t i = 0; i < m_fec_param.r; i++) {
			com::Buffer buf(MAX_FRAG_SIZE + head_len);
			buf.data_len = MAX_FRAG_SIZE + head_len;
			red_buf.push_back(buf);
			red_data[i] = buf.data.get() + FEC_PKT_HDR_LEN;
		}

		// FEC encode
		m_fec_encoder->Encode((void**)src_data, (void**)red_data,
			MAX_FRAG_SIZE + SES_PKT_HDR_LEN);

		LOG_DBG("[session:{}] FEC encode", m_sess_param.local_sid);

		// FEC sequence number in group start from 0
		uint8_t gsn = 0;

		// Add FEC header for source data and move it to wait queue
		for (uint32_t i = 0; i < m_fec_param.k; i++) {
			com::Buffer buf = m_send_cache_que.front();

			FecProtocol::BuildFecPkt(m_fec_param.k, m_fec_param.r, m_fec_next_sn++,
				m_fec_next_group, gsn++, SESSION_PKT_DATA, buf);

			m_send_wait_que.push_back(buf);
			m_wait_que_size++;
			LOG_DBG("[session:{}] Add source data to wait queue, wait size:{}",
				m_sess_param.local_sid, m_wait_que_size);

			m_send_cache_que.pop_front();
			m_cache_que_size--;
			LOG_DBG("[session:{}] Cache queue size:{}", m_sess_param.local_sid,
				m_cache_que_size);
		}

		// Add FEC head for redundant data and move it to wait queue
		for (uint32_t i = 0; i < m_fec_param.r; i++) {
			com::Buffer buf = red_buf.front();

			FecProtocol::BuildFecPkt(m_fec_param.k, m_fec_param.r, m_fec_next_sn++,
				m_fec_next_group, gsn++, SESSION_PKT_DATA, buf);

			m_send_wait_que.push_back(buf);
			m_wait_que_size++;
			LOG_DBG("[session:{}] Add redundant data to wait queue, wait size:{}",
				m_sess_param.local_sid, m_wait_que_size);

			red_buf.pop_front();
		}

		m_fec_next_group++;
	}

	delete[] src_data;
	delete[] red_data;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SendingController::OnFecDataPush(const com::Buffer& buf)
{
	if (m_cache_que_size >= SEND_CACHE_QUEUE_MAX_SIZE) {
		LOG_WRN("[session:{}] Send cache queue is full, size:{}",
			m_sess_param.local_sid, m_cache_que_size);
		m_pending = true;
		return false;
	}

	uint32_t count = m_data_splitter->SplitFecData(buf, m_send_cache_que);
	if (count != 0) {
		m_last_cache_ts = util::Now();
	}
	m_cache_que_size += count;

	TryEncodingFecData();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SendingController::PushSessionData(const com::Buffer& buf)
{
	if (m_pending) {
		LOG_DBG("[session:{}] Sending controller is pending", m_sess_param.local_sid);
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	return (m_fec_param.r == 0) ? OnSessionDataPush(buf) : OnFecDataPush(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint64_t SendingController::GetNextSendTime()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_send_wait_que.empty()) {
		return INVALID_SEND_TIME;
	}

	// 不可靠会话不做平滑处理
	if (m_sess_param.session_type == SessionType::UNRELIABLE) {
		return 0;
	}

	uint32_t link_cap = m_link_cap_estimator->GetLinkCap();
	if (link_cap == 0) {
		return 0; // send at once
	}

	// pacing rate
	SesPktHdr* hdr = (SesPktHdr*)(m_send_wait_que.front().data.get());
	if (hdr->psn % 16 == 1) {
		return 0;
	}
	else {
		return (uint64_t)(1000000.0 / (double)link_cap);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SendResult SendingController::SendWithSingleMode(CacheSessionPktList& cache_list)
{
	SendResult send_result;

	// Send the front data
	com::Buffer buf = m_send_wait_que.front();

	SesPktHdr* ses_hdr = nullptr;
	if ((*buf.data.get() & 0x3) == 0) { // session packet
		ses_hdr = (SesPktHdr*)buf.data.get();
	}
	else { // fec packet
		ses_hdr = (SesPktHdr*)(buf.data.get() + FEC_PKT_HDR_LEN);
	}

	// Set the real transmission time
	ses_hdr->ts = static_cast<uint32_t>(util::Now());

	if (ERR_CODE_OK != m_session_pkt_sender->SendPkt(SESSION_PKT_DATA, buf)) {
		LOG_ERR("[session:{}] Send packet failed, psn:{}",
			m_sess_param.local_sid, ses_hdr->psn);
		return send_result;
	}

	// Remove from send wait queue
	m_send_wait_que.pop_front();
	m_wait_que_size--;

	CacheSessionPktSP pkt(new CacheSessionPkt(buf, ses_hdr->ts,
		m_rtt_filter->GetRto(), ses_hdr->psn, ses_hdr->msn, 1, 0));

	// 重传缓存（NACK）
	if (m_sess_param.session_type == SessionType::RELIABLE) {
		cache_list.push_back(pkt);
		send_result.cache_count += 1;
		LOG_DBG("[session:{}] Add packet to cache queue, psn:{}, cache size:{}",
			m_sess_param.local_sid, ses_hdr->psn, cache_list.size());
	}
	
	send_result.send_count += 1;
	send_result.send_size += buf.data_len;

	return send_result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SendResult SendingController::SendWithGroupMode(CacheSessionPktList& cache_list)
{
	SendResult send_result;

	if (m_send_wait_que.empty()) {
		LOG_DBG("[session:{}] Send wait queue is empty", m_sess_param.local_sid);
		return send_result;
	}

	uint16_t group = ((FecPktHdr*)m_send_wait_que.front().data.get())->grp;

	// Send whole fec group
	while (!m_send_wait_que.empty()) {
		Buffer buf = m_send_wait_que.front();
		FecPktHdr* fec_hdr = (FecPktHdr*)buf.data.get();

		// Meet next group, quit
		if (fec_hdr->grp != group) break;

		SesPktHdr* ses_hdr = (SesPktHdr*)(buf.data.get() + FEC_PKT_HDR_LEN);

		// Set the real tx time
		ses_hdr->ts = static_cast<uint32_t>(util::Now());

		if (ERR_CODE_OK != m_session_pkt_sender->SendPkt(FEC_PKT_TYPE, buf)) {
			LOG_ERR("[session:{}] Send fec packet failed, fec group:{}, fec sn:{}, "
				"psn:{}, msn:{}", m_sess_param.local_sid, fec_hdr->grp, fec_hdr->gsn,
				ses_hdr->psn, ses_hdr->msn);
			return send_result;
		}

		send_result.send_count += 1;
		send_result.send_size += buf.data_len;

		// Remove packet from wait queue
		m_send_wait_que.pop_front();
		m_wait_que_size--;

		// NACK 重传缓存（不包括冗余报文）
		if (fec_hdr->gsn < fec_hdr->k) {
			CacheSessionPktSP pkt(new CacheSessionPkt(buf, ses_hdr->ts,
				m_rtt_filter->GetRto(), ses_hdr->psn, ses_hdr->msn, 1, 0));

			// Adjust to session header
			pkt->buf.start_pos = FEC_PKT_HDR_LEN;

			if (m_sess_param.session_type == SessionType::RELIABLE) {
				cache_list.push_back(pkt);
				send_result.cache_count += 1;
				LOG_DBG("[session:{}] Add packet to cache queue, psn:{}, cache size:{}",
					m_sess_param.local_sid, ses_hdr->psn, cache_list.size());
			}
		}
	}

	return send_result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SendResult SendingController::SendSessionData(CacheSessionPktList& cache_list)
{
	SendResult send_result;

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_send_wait_que.empty()) {
		LOG_DBG("[session:{}] Empty send wait queue", m_sess_param.local_sid);
		return send_result;
	}

	uint8_t flag = (*m_send_wait_que.front().data.get()) & 0x3;
	if (flag == 0) {
		send_result = SendWithSingleMode(cache_list);
	}
	else {
		send_result = SendWithGroupMode(cache_list);
	}

	if (m_pending && m_wait_que_size == 16) {
		com::CommonMsg msg;
		msg.msg_type = NET_MSG_SESSION_RESUME;
		msg.msg_data.reset(new SessionResumeMsg(m_sess_param.local_sid,
			m_sess_param.remote_sid));
		m_sess_param.thread->PostMsg(msg);

		m_pending = false;
	}

	return send_result;
}

}