#include <sstream>

#include "session-receiver.h"
#include "rtt-filter.h"
#include "common/util-time.h"
#include "common-config.h"
#include "log.h"


using namespace jukey::com;
using namespace jukey::util;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionReceiver::SessionReceiver(base::IComFactory* factory,
	const SessionParam& param,
	const ISessionPktSenderSP& sender,
	const IRttFilterSP& filter,
	const LinkCapEstimatorSP& estimator,
	const FecPktAssemblerSP& assembler)
	: m_sess_param(param)
	, m_pkt_sender(sender)
	, m_rtt_filter(filter)
	, m_link_cap_estimator(estimator)
	, m_fec_assembler(assembler)
{
	m_lost_pkt_tracer.reset(new LostPktTracer(5000000));
	m_recv_pkt_tracer.reset(new RecvPktTracer(5000000));

	m_last_stats_ts = util::Now();

	InitStats(factory);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionReceiver::~SessionReceiver()
{
	m_data_stats->Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::InitStats(base::IComFactory* factory)
{
	std::stringstream fmt;
	fmt << "[session:" << m_sess_param.local_sid << "] Session receiver,";

	m_data_stats.reset(new util::DataStats(factory, g_net_logger, fmt.str(), false));
	m_data_stats->Start();

	StatsParam i_unor_frg("unor-frg", StatsType::IACCU, 5000);
	m_i_unor_frg = m_data_stats->AddStats(i_unor_frg);

	StatsParam i_recv_frg("recv-frg", StatsType::IACCU, 5000);
	m_i_recv_frg = m_data_stats->AddStats(i_recv_frg);

	StatsParam i_rtx_frg("rtx-frg", StatsType::IACCU, 5000);
	m_i_rtx_frg = m_data_stats->AddStats(i_rtx_frg);

	StatsParam i_ack_frg("ack-frg", StatsType::IACCU, 5000);
	m_i_ack_frg = m_data_stats->AddStats(i_ack_frg);

	StatsParam t_recv_frg("total-recv-frg", StatsType::TACCU, 5000);
	m_t_recv_frg = m_data_stats->AddStats(t_recv_frg);

	StatsParam t_recv_pkt("total-recv-pkt", StatsType::TACCU, 5000);
	m_t_recv_pkt = m_data_stats->AddStats(t_recv_pkt);

	StatsParam i_recv_pkt("recv-pkt", StatsType::TACCU, 5000);
	m_i_recv_pkt = m_data_stats->AddStats(i_recv_pkt);

	StatsParam t_recv_bytes("total-recv-bytes", StatsType::TACCU, 5000);
	m_t_recv_bytes = m_data_stats->AddStats(t_recv_bytes);

	StatsParam i_recv_loss("lost-pkt", StatsType::ISNAP, 5000);
	m_i_recv_loss = m_data_stats->AddStats(i_recv_loss);

	StatsParam i_drop_frg("drop-frg", StatsType::IACCU, 5000);
	m_i_drop_frg = m_data_stats->AddStats(i_drop_frg);

	StatsParam i_aver_rtt("rtt", StatsType::ICAVG, 5000);
	m_i_aver_rtt = m_data_stats->AddStats(i_aver_rtt);

	StatsParam i_recv_bitrate;
	i_recv_bitrate.name       = "recv-br";
	i_recv_bitrate.stats_type = StatsType::IAVER;
	i_recv_bitrate.interval   = 5000;
	i_recv_bitrate.unit       = "kbps";
	i_recv_bitrate.mul_factor = 8;
	i_recv_bitrate.div_factor = 1000;
	m_i_recv_br = m_data_stats->AddStats(i_recv_bitrate);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::OnUpdate()
{
	uint64_t now = jukey::util::Now();

	m_lost_pkt_tracer->Update();
	m_data_stats->OnData(m_i_recv_loss, m_lost_pkt_tracer->GetInfo().lost_count);

	m_recv_pkt_tracer->AddPktCount(m_recv_pkt_count, now - m_last_stats_ts);
	m_last_stats_ts = now;
	m_recv_pkt_count = 0;

	if (m_sess_param.session_type == SessionType::RELIABLE) {
		SendAck();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::FillLostPackets(std::vector<uint32_t>& sns)
{
	uint32_t sn = m_next_recv_psn;

	for (auto& item : m_recv_cache_que) {
		if (sns.size() >= NACK_REQUEST_MAX_COUNT) {
			LOG_DBG("[session:{}] Too many lost packets", m_sess_param.local_sid);
			break;
		}

		if (sn == item->head.psn) {
			sn++;
			continue;
		}
		else if (sn < item->head.psn) {
			for (uint32_t i = sn; i < item->head.psn; i++) {
				sns.push_back(i);
				if (sns.size() >= NACK_REQUEST_MAX_COUNT) {
					break;
				}
			}
			sn = item->head.psn + 1;
		}
		else { // sn > psn
			LOG_WRN("[session:{}] sn:{} is larger than psn:{}",
				m_sess_param.local_sid, sn, item->head.psn);
			continue;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::SendAck()
{
	if (!((m_next_recv_psn > m_last_send_ack_psn) // received new ordered data
		|| (m_last_send_ack_msn > m_last_recv_ack2_msn) // ack2 lost
		|| (m_cache_que_size > 0 && m_last_send_ack_ts + 20000 < util::Now()))) { // retransmit lost
		/*LOG_DBG("Needn't ack, last send ack:{}, next recv psn:{}, last recv ack2:{},"
			" last send ack msn:{}, recv cache queue size:{}",
			m_last_send_ack_psn, m_next_recv_psn, m_last_recv_ack2_msn,
			m_last_send_ack_msn, m_cache_que_size);*/
		return;
	}

	std::vector<uint32_t> sns;
	FillLostPackets(sns);

	com::Buffer pkt = SessionProtocol::BuildAckPkt(m_sess_param.local_sid,
		m_sess_param.remote_sid, m_next_recv_psn, m_next_ack_sn++, sns);

	m_data_stats->OnData(m_i_ack_frg, m_next_recv_psn - m_last_send_ack_psn);

	LOG_DBG("[session:{}] Send ack:{}, append sn count:{}",
		m_sess_param.local_sid, m_next_recv_psn, sns.size());

	if (ERR_CODE_OK != m_pkt_sender->SendPkt(SESSION_PKT_ACK, pkt)) {
		LOG_ERR("[session:{}] Send ack failed!", m_sess_param.local_sid);
	}

	m_last_send_ack_psn = m_next_recv_psn;
	m_last_send_ack_msn = m_next_ack_sn - 1;
	m_last_send_ack_ts = util::Now();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::OnRecvOrderedSessionPkt(const SessionPktSP& pkt)
{
	LOG_DBG("[session:{}] Received ordered packet len:{}, psn:{}, wait size:{},"
		" cache size:{}", m_sess_param.local_sid, pkt->buf.data_len,
		pkt->head.psn, m_wait_que_size, m_cache_que_size);

	if (m_wait_que_size >= RECV_WAIT_QUEUE_MAX_SIZE) {
		LOG_WRN("[session:{}] Recv wait queue is full, size:{}",
			m_sess_param.local_sid, m_wait_que_size);
		return;
	}

	m_lost_pkt_tracer->RemoveLostPkt(pkt->head.psn);

	// Put in wait queue directly
	m_recv_wait_que.push_back(pkt);
	m_wait_que_size++;
	m_next_recv_psn++;

	// Maybe cache queue packets become ordered
	while (!m_recv_cache_que.empty()) {
		SessionPktSP tmp = m_recv_cache_que.front();
		if (tmp->head.psn != m_next_recv_psn) {
			break;
		}
		m_recv_wait_que.push_back(tmp);
		m_wait_que_size++;

		m_recv_cache_que.pop_front();
		m_cache_que_size--;

		m_next_recv_psn++;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::OnReceivedUnorderedPkt(const SessionPktSP& pkt)
{
	LOG_DBG("[session:{}] Received unordered packet, psn:{}",
		m_sess_param.local_sid, pkt->head.psn);

	m_data_stats->OnData(m_i_unor_frg, 1);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::OnRecvCachingSessionPkt(const SessionPktSP& pkt)
{
	LOG_DBG("[session:{}] Received caching packet, psn:{}, msn:{}, want psn:{}",
		m_sess_param.local_sid, pkt->head.psn, pkt->head.msn, m_next_recv_psn);

	if (m_cache_que_size >= RECV_CAHCE_QUEUE_MAX_SIZE) {
		LOG_WRN("[session:{}] Recv cache queue is full, size:{}",
			m_sess_param.local_sid, m_cache_que_size);
		return;
	}

	// Add to cache queue directly
	if (m_recv_cache_que.empty()) {
		m_recv_cache_que.push_back(pkt);
		m_cache_que_size++;

		m_lost_pkt_tracer->AddLostPkt(m_next_recv_psn, pkt->head.psn);
		return;
	}

	// Insert to the right position
	auto iter = m_recv_cache_que.begin();
	for (; iter != m_recv_cache_que.end(); iter++) {
		if (pkt->head.psn == (*iter)->head.psn) {
			LOG_DBG("[session:{}] Repeat caching packet:{}",
				m_sess_param.local_sid, pkt->head.psn);
			m_data_stats->OnData(m_i_drop_frg, 1);
			break;
		}
		else if (pkt->head.psn < (*iter)->head.psn) {
			auto i = iter; // for unordered judgment
			if (i == m_recv_cache_que.begin()
				|| pkt->head.psn != (*(i--))->head.psn + 1) {
				OnReceivedUnorderedPkt(pkt);
			}
			m_recv_cache_que.insert(iter, pkt);
			m_cache_que_size++;

			// Filter retransmit packets, if not there will be no lost packet
			if (pkt->head.rtx == 0) {
				m_lost_pkt_tracer->RemoveLostPkt(pkt->head.psn);
			}

			break; // Found insert postion
		}
	}

	// Cannot find position to insert, then add to the tail
	if (iter == m_recv_cache_que.end()) {
		if (m_cache_que_size == 0
			|| pkt->head.psn != m_recv_cache_que.back()->head.psn + 1) {
			OnReceivedUnorderedPkt(pkt);
		}

		auto back_iter = m_recv_cache_que.back();
		if (back_iter->head.psn + 1 < pkt->head.psn) {
			m_lost_pkt_tracer->AddLostPkt(back_iter->head.psn + 1, pkt->head.psn);
		}

		m_recv_cache_que.push_back(pkt);
		m_cache_que_size++;
	}

	LOG_DBG("[session:{}] cache size:{}, wait size:{}", m_sess_param.local_sid,
		m_cache_que_size, m_wait_que_size);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::ProcReliableSessionPkt(const SessionPktSP& pkt)
{
	if (pkt->head.psn == m_next_recv_psn) {
		OnRecvOrderedSessionPkt(pkt);
	}
	else {
		OnRecvCachingSessionPkt(pkt);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::ProcUnreliableSessionPkt(const SessionPktSP& pkt)
{
	if (m_wait_pkt_size >= RECV_WAIT_QUEUE_MAX_SIZE) {
		if (m_wait_entries.empty()) {
			assert(false);
			LOG_ERR("Impossible!!!");
			return;
		}

		const WaitEntry& entry = m_wait_entries.front();
		
		auto iter = m_recv_wait_map.find(entry.msn);
		if (iter == m_recv_wait_map.end()) {
			assert(false);
			LOG_ERR("Cannot find msn:{} from wait map", entry.msn);
			return;
		}

		assert(m_wait_pkt_size >= (uint32_t)iter->second.size());
		m_wait_pkt_size -= (uint32_t)iter->second.size();

		m_recv_wait_map.erase(iter);
		m_wait_entries.pop_front();

		LOG_INF("Remove msn:{} for reaching max size", entry.msn);
	}

	auto iter = m_recv_wait_map.find(pkt->head.msn);
	if (iter != m_recv_wait_map.end()) {
		bool inserted = false;
		for (auto i = iter->second.begin(); i != iter->second.end(); i++) {
			if (pkt->head.psn < (*i)->head.psn) {
				iter->second.insert(i, pkt);
				inserted = true;
				break;
			}
		}
		if (!inserted) {
			iter->second.push_back(pkt);
		}

		m_wait_pkt_size++;
		
		assert(iter->second.size() > 1);

		auto& front_head = iter->second.front()->head;
		auto& back_head = iter->second.back()->head;

		if (front_head.pos == PKT_POS_FIRST
			&& back_head.pos == PKT_POS_LAST
			&& back_head.psn - front_head.psn + 1 == iter->second.size()) {
			bool found = false;
			for (auto& entry : m_wait_entries) {
				if (entry.msn == pkt->head.msn) {
					entry.complete = true;
					found = true;
					break;
				}
			}
			if (!found) {
				LOG_ERR("Cannot find wait entry to set complete, msn:{}, psn:{}",
					pkt->head.msn, pkt->head.psn);
			}
		}
	}
	else {
		SessionPktList pkt_list{pkt};
		m_recv_wait_map.insert(std::make_pair(pkt->head.msn, pkt_list));
		m_wait_pkt_size++;

		WaitEntry entry;
		entry.msn = pkt->head.msn;
		entry.ts = util::Now();
		entry.complete = (pkt->head.pos == PKT_POS_ONLY);

		m_wait_entries.push_back(entry);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::OnSessionData(const SessionPktSP& pkt)
{
	m_recv_rate_caculator.OnPktArrival(pkt->buf.data_len);

	m_data_stats->OnData(m_t_recv_frg, 1);
	m_data_stats->OnData(m_i_recv_frg, 1);
	m_data_stats->OnData(m_t_recv_bytes, pkt->buf.data_len);
	m_data_stats->OnData(m_i_recv_br, pkt->buf.data_len);

	// Retransmit statistic
	if (pkt->head.rtx != 0) {
		m_data_stats->OnData(m_i_rtx_frg, 1);
	}
	else {
		// Loss rate caculation should eliminate rtx packets
		m_recv_pkt_count++;
	}

	// The packet has been acknowledged
	if (pkt->head.psn < m_next_recv_psn) {
		LOG_DBG("[session:{}] Repeat acked packet:{}, next recv psn:{}",
			m_sess_param.local_sid, pkt->head.psn, m_next_recv_psn);
		m_data_stats->OnData(m_i_drop_frg, 1);
		return;
	}

	if (pkt->head.psn % 16 == 0) {
		m_link_cap_estimator->OnProbe1Arraval();
	}
	else if (pkt->head.psn % 16 == 1) {
		m_link_cap_estimator->OnProbe2Arraval();
	}

	if (m_sess_param.session_type == SessionType::RELIABLE) {
		ProcReliableSessionPkt(pkt);
	}
	else if (m_sess_param.session_type == SessionType::UNRELIABLE) {
		ProcUnreliableSessionPkt(pkt);
	}
	else {
		LOG_ERR("[session:{}] Invalid session type:{}", m_sess_param.local_sid, 
			m_sess_param.service_type);
	}
}

//------------------------------------------------------------------------------
// Pre-calculate the total length of the message after reassembled
//------------------------------------------------------------------------------
uint32_t SessionReceiver::GetReassembledLen(const SessionPktList& pkt_list)
{
	uint32_t pkt_len = 0;

	for (auto i = pkt_list.begin(); i != pkt_list.end(); ++i) {
		pkt_len += (*i)->head.len;
		if ((*i)->head.pos == PKT_POS_LAST)
			break;
	}

	return pkt_len;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionPktSP SessionReceiver::GetReliableSessionMsg()
{
	if (m_recv_wait_que.empty()) {
		return nullptr;
	}

	// Without fragments
	SessionPktSP front_pkt = m_recv_wait_que.front();
	if (front_pkt->head.pos == PKT_POS_ONLY) {
		m_recv_wait_que.pop_front();
		m_wait_que_size--;
		return front_pkt;
	}
	else if (front_pkt->head.pos == PKT_POS_LAST
		|| front_pkt->head.pos == PKT_POS_MIDDLE) {
		LOG_ERR("[session:{}] Invalid packet position:{}, psn:{}, msn:{}",
			m_sess_param.local_sid,
			(uint8_t)front_pkt->head.pos,
			front_pkt->head.psn,
			front_pkt->head.msn);
		m_recv_wait_que.pop_front();
		m_wait_que_size--;
		return nullptr;
	}

	// Last fragment has not yet come
	SessionPktSP back_pkt = m_recv_wait_que.back();
	if (back_pkt->head.msn == front_pkt->head.msn
		&& back_pkt->head.pos != PKT_POS_LAST) {
		return nullptr;
	}

	SessionPktSP pkt = SessionPktSP(new SessionPkt(GetReassembledLen(m_recv_wait_que)));
	pkt->head = m_recv_wait_que.front()->head;
	pkt->head.len = 0;

	uint32_t copy_pos = 0;
	while (!m_recv_wait_que.empty()) {
		SessionPktSP tmp = m_recv_wait_que.front();
		m_recv_wait_que.pop_front(); // ???为什么放这里才可以
		m_wait_que_size--;

		memcpy((char*)DP(pkt->buf) + copy_pos, (char*)DP(tmp->buf), tmp->buf.data_len);

		pkt->buf.data_len += tmp->buf.data_len;
		copy_pos += tmp->buf.data_len;

		if (tmp->head.pos == PKT_POS_LAST) break;
	}

	if (pkt->buf.data_len != pkt->buf.total_len) {
		LOG_ERR("[session:{}] Packet failed, data len:{}, total len:{}",
			m_sess_param.local_sid, pkt->buf.data_len, pkt->buf.total_len);
		return nullptr;
	}

	return pkt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionPktSP SessionReceiver::GetUnreliableSessionMsg()
{
	SessionPktSP pkt;

	for (auto i = m_wait_entries.begin(); i != m_wait_entries.end(); ++i) {
		if (!i->complete) {
			continue;
		}

		auto iter = m_recv_wait_map.find(i->msn);
		if (iter == m_recv_wait_map.end()) {
			LOG_ERR("Cannot find entry in recv wait map, msn:{}", i->msn);
			m_wait_entries.erase(i);
			break;
		}

		if (iter->second.empty()) {
			LOG_ERR("No session pkt, msn:{}", i->msn);
		}
		else if (iter->second.size() == 1) {
			if (iter->second.front()->head.pos != PKT_POS_ONLY) {
				LOG_ERR("Single pkt but not PKT_POS_ONLY, msn:{}, psn:{}", 
					i->msn, iter->second.front()->head.psn);
			}
			else {
				pkt = iter->second.front();
			}
		}
		else {
			pkt = SessionPktSP(new SessionPkt(GetReassembledLen(iter->second)));
			pkt->head = iter->second.front()->head;
			pkt->head.len = 0;

			uint32_t copy_pos = 0;

			for (auto j = iter->second.begin(); j != iter->second.end(); j++) {
				memcpy(DP(pkt->buf) + copy_pos, DP((*j)->buf), (*j)->buf.data_len);

				pkt->buf.data_len += (*j)->buf.data_len;
				copy_pos += (*j)->buf.data_len;
			}
		}

		assert(m_wait_pkt_size >= (uint32_t)iter->second.size());
		m_wait_pkt_size -= (uint32_t)iter->second.size();

		m_recv_wait_map.erase(iter);
		m_wait_entries.erase(i);

		break;
	}

	return pkt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionPktSP SessionReceiver::GetSessionMsg()
{
	SessionPktSP pkt;

	if (m_sess_param.session_type == SessionType::RELIABLE) {
		pkt = GetReliableSessionMsg();
	}
	else if (m_sess_param.session_type == SessionType::UNRELIABLE) {
		pkt = GetUnreliableSessionMsg();
	}
	else {
		LOG_ERR("[session:{}] Invalid session type:{}", m_sess_param.local_sid, 
			m_sess_param.session_type);
	}

	if (pkt) {
		m_data_stats->OnData(m_t_recv_pkt, 1);
		m_data_stats->OnData(m_i_recv_pkt, 1);
	}

	return pkt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::SendReport(const ReportData& report_data)
{
	com::Buffer pkt = SessionProtocol::BuildReportPkt(
		m_sess_param.local_sid,
		m_sess_param.remote_sid,
		report_data);

	if (ERR_CODE_OK != m_pkt_sender->SendPkt(SESSION_PKT_REPORT, pkt)) {
		LOG_ERR("[session:{}] Send report failed!", m_sess_param.local_sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionReceiver::OnSessionAck2(const SessionPktSP& pkt)
{
	// Calc srtt
	uint32_t srtt = m_rtt_filter->UpdateRtt((uint32_t)(Now()) - pkt->head.ts);
	m_data_stats->OnData(m_i_aver_rtt, srtt);

	// Calc session loss rate
	uint32_t ses_loss_rate = 0;
	uint32_t recv_pkt_count = m_recv_pkt_tracer->GetPktCount();
	PktLostInfo lost_info = m_lost_pkt_tracer->GetInfo();
	if (recv_pkt_count != 0) {
		ses_loss_rate = lost_info.lost_count * 1000
			/ (recv_pkt_count + lost_info.lost_count);
	}

	// Calc fec loss rate
	uint32_t fec_loss_rate = 0;
	AssemblerInfo asmb_info = m_fec_assembler->GetInfo();
	if (asmb_info.recv_pkt_count != 0) {
		fec_loss_rate = asmb_info.loss_pkt_count * 1000
			/ (asmb_info.recv_pkt_count + asmb_info.loss_pkt_count);
	}

	ReportData report;
	report.rtt = srtt;
	report.rtt_var = 0;
	report.recv_rate = m_recv_rate_caculator.GetPktRate();
	report.link_cap = m_link_cap_estimator->GetLinkCap();
	report.ses_loss_rate = ses_loss_rate;
	report.fec_loss_rate = fec_loss_rate;

	// Calc remain window size
	uint32_t que_size = 0;
	if (m_sess_param.session_type == SessionType::RELIABLE) {
		que_size = m_wait_que_size + m_cache_que_size;
	}
	else {
		que_size = m_wait_pkt_size;
	}

	if ((uint32_t)que_size >= RECV_WAIT_QUEUE_MAX_SIZE) {
		report.wnd = 0;
	}
	else {
		report.wnd = RECV_WAIT_QUEUE_MAX_SIZE - (uint32_t)que_size;
	}
	
	SendReport(report); // send report immediately

	m_last_recv_ack2_msn = pkt->head.msn;
}

}