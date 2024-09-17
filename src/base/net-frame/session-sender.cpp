#include <sstream>

#include "session-sender.h"
#include "congestion-controller.h"
#include "common/util-time.h"
#include "if-sending-queue.h"
#include "common-config.h"
#include "sending-controller.h"
#include "fec-protocol.h"
#include "simple-fec-controller.h"
#include "log.h"


using namespace jukey::com;
using namespace jukey::util;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionSender::SessionSender(base::IComFactory* factory, const SessionParam& param,
	const ISessionPktSenderSP& sender, const IRttFilterSP& filter,
	const LinkCapEstimatorSP& estimator)
	: m_sess_param(param)
	, m_pkt_sender(sender)
	, m_rtt_filter(filter)
	, m_link_cap_estimator(estimator)
{
	m_cong_ctrl.reset(new CongestionController(param));

	m_send_ctrl.reset(new SendingController(m_sess_param, m_rtt_filter,
		m_pkt_sender, m_link_cap_estimator));

	m_fec_ctrl.reset(new SimpleFecController());

	InitStats(factory);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionSender::~SessionSender()
{
	m_data_stats->Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionSender::InitStats(base::IComFactory* factory)
{
	std::stringstream fmt;
	fmt << "[session:" << m_sess_param.local_sid << "] Session sender,";

	m_data_stats.reset(new util::DataStats(factory, g_net_logger, fmt.str(), false));
	m_data_stats->Start();

	StatsParam i_rto_rtx("rto-rtx", StatsType::IACCU, 5000);
	m_i_rto_rtx = m_data_stats->AddStats(i_rto_rtx);

	StatsParam i_nack_rtx("nack-rtx", StatsType::IACCU, 5000);
	m_i_nack_rtx = m_data_stats->AddStats(i_nack_rtx);

	StatsParam i_send_pkt("send-pkt", StatsType::IACCU, 5000);
	m_i_send_pkt = m_data_stats->AddStats(i_send_pkt);

	StatsParam t_send_pkt("total-send-pkt", StatsType::TACCU, 5000);
	m_t_send_pkt = m_data_stats->AddStats(t_send_pkt);

	StatsParam i_recv_ack("ack-pkt", StatsType::IACCU, 5000);
	m_i_ack_pkt = m_data_stats->AddStats(i_recv_ack);

	StatsParam total_send("total-send-bytes", StatsType::TACCU, 5000);
	m_t_send_bytes = m_data_stats->AddStats(total_send);

	StatsParam send_rate;
	send_rate.name       = "send-bitrate";
	send_rate.stats_type = StatsType::IAVER;
	send_rate.interval   = 5000;
	send_rate.unit       = "kbps";
	send_rate.mul_factor = 8;
	send_rate.div_factor = 1000;
	m_i_send_bitrate     = m_data_stats->AddStats(send_rate);

	StatsParam i_aver_rtt("rtt", StatsType::ICAVG, 5000);
	m_i_aver_rtt = m_data_stats->AddStats(i_aver_rtt);

	StatsParam i_recv_pkt_rate("pkt-rate", StatsType::ICAVG, 5000);
	m_i_recv_pkt_rate = m_data_stats->AddStats(i_recv_pkt_rate);

	StatsParam i_link_cap("link-cap", StatsType::ICAVG, 5000);
	m_i_link_cap = m_data_stats->AddStats(i_link_cap);

	StatsParam i_ses_loss("ses-loss", StatsType::ICAVG, 5000);
	m_i_ses_loss = m_data_stats->AddStats(i_ses_loss);

	StatsParam i_fec_loss("fec-loss", StatsType::ICAVG, 5000);
	m_i_fec_loss = m_data_stats->AddStats(i_fec_loss);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionSender::OnSessionNegotiateComplete()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionSender::OnUpdate()
{
	if (m_send_ctrl) {
		m_send_ctrl->Update();
	}
}

//------------------------------------------------------------------------------
// Call from sending thread
//------------------------------------------------------------------------------
void SessionSender::SendSessionData()
{
	// Congestion control
	if (m_send_cache_que_size >= m_cong_ctrl->GetCCParam().cwnd) {
		LOG_DBG("[session:{}] inflight:{} is larger than cwnd:{}",
			m_sess_param.local_sid,
			m_send_cache_que_size,
			m_cong_ctrl->GetCCParam().cwnd);
		return;
	}

	if (!m_send_ctrl) {
		LOG_DBG("[session:{}] Invalid send controller!", m_sess_param.local_sid);
		return;
	}

	m_send_cache_que_mtx.lock();
	SendResult send_result = m_send_ctrl->SendSessionData(m_send_cache_que);
	if (send_result.send_count == 0) {
		LOG_WRN("[session:{}] Send no data!", m_sess_param.local_sid);
	}
	m_send_cache_que_size += send_result.cache_count;

	LOG_DBG("[session:{}] Add cache queue count:{}, total size:{}",
		m_sess_param.local_sid,
		send_result.send_count,
		m_send_cache_que.size());

	m_send_cache_que_mtx.unlock();

	m_data_stats->OnData(m_i_send_pkt, send_result.send_count);
	m_data_stats->OnData(m_t_send_pkt, send_result.send_count);
	m_data_stats->OnData(m_t_send_bytes, send_result.send_size);
	m_data_stats->OnData(m_i_send_bitrate, send_result.send_size);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint64_t SessionSender::NextSendTime()
{
	if (m_send_ctrl) {
		return m_send_ctrl->GetNextSendTime();
	}
	else {
		LOG_DBG("[session:{}] Invalid send controller!", m_sess_param.local_sid);
		return INVALID_SEND_TIME;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionSender::PushSessionData(const com::Buffer& buf)
{
	if (m_send_ctrl) {
		return m_send_ctrl->PushSessionData(buf);
	}
	else {
		LOG_DBG("[session:{}] Invalid send controller!", m_sess_param.local_sid);
		return false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionSender::OnSessionReport(const SessionPktSP& pkt)
{
	ReportData data = SessionProtocol::ParseReportData(pkt->buf);

	LOG_DBG("[session:{}] Received report, rtt:{}, rtt-var:{}, wnd:{}, "
		"recv-rate:{}, link-cap:{}, ses-loss-rate:{}, fec-loss-rate:{}",
		m_sess_param.local_sid, data.rtt, data.rtt_var, data.wnd,
		data.recv_rate, data.link_cap, data.ses_loss_rate, data.fec_loss_rate);

	m_cong_ctrl->UpdateRtt(data.rtt);
	m_cong_ctrl->UpdateRemoteWnd(data.wnd);

	m_fec_ctrl->UpdateLossInfo(LossInfo(data.ses_loss_rate, data.fec_loss_rate));

	if (m_sess_param.fec_type != FecType::NONE) {
		m_send_ctrl->SetFecParam(m_fec_ctrl->GetFecParam());
	}

	m_data_stats->OnData(m_i_aver_rtt, data.rtt);
	m_data_stats->OnData(m_i_recv_pkt_rate, data.recv_rate);
	m_data_stats->OnData(m_i_link_cap, data.link_cap);
	m_data_stats->OnData(m_i_ses_loss, data.ses_loss_rate);
	m_data_stats->OnData(m_i_fec_loss, data.fec_loss_rate);
}

//------------------------------------------------------------------------------
// TODO: update inflight
//------------------------------------------------------------------------------
void SessionSender::ProcNack(const SessionPktSP& pkt)
{
	// Parse nack psn
	AckData ack_data = SessionProtocol::ParseAckData(pkt->buf);
	if (ack_data.sns.empty()) {
		LOG_DBG("[session:{}] No append sn", m_sess_param.local_sid);
		return;
	}

	m_send_cache_que_mtx.lock();
	for (auto lost_psn : ack_data.sns) {
		LOG_DBG("[session:{}] Lost packet:{}", m_sess_param.local_sid, lost_psn);
		for (auto& cache_pkt : m_send_cache_que) {
			if (lost_psn == cache_pkt->psn) { // found lost packet
				LOG_DBG("[session:{}] NACK retransmit packet:{}",
					m_sess_param.local_sid, lost_psn);

				// Set retransmit flag
				if ((*cache_pkt->buf.data.get() & 0x3) == 0) { // session packet
					((SesPktHdr*)cache_pkt->buf.data.get())->rtx = 1;
				}
				else { // fec packet
					((SesPktHdr*)(cache_pkt->buf.data.get() + FEC_PKT_HDR_LEN))->rtx = 1;

					// Only retransmit sesion packet
					cache_pkt->buf.start_pos = FEC_PKT_HDR_LEN;
				}

				m_pkt_sender->SendPkt(SESSION_PKT_DATA, cache_pkt->buf);

				// Update last send time
				cache_pkt->ts = (uint32_t)util::Now();
				m_data_stats->OnData(m_i_nack_rtx, 1);
				break;
			}
		}
	}
	m_send_cache_que_mtx.unlock();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionSender::OnSessionAck(const SessionPktSP& pkt)
{
	LOG_DBG("[session:{}] Received ack, psn:{}, msn:{}, append len:{}",
		m_sess_param.local_sid, pkt->head.psn, pkt->head.msn, pkt->buf.data_len);

	// Response ack2 immediately
	SendAck2(pkt->head.msn, pkt->head.ts);

	m_cong_ctrl->OnRecvAck(pkt->head.psn, pkt->head.psn - m_last_ack_psn);

	// 统计 ACK 报文数
	m_data_stats->OnData(m_i_ack_pkt, pkt->head.psn - m_last_ack_psn);

	// Update received sn
	m_last_ack_psn = pkt->head.psn;

	// Remove acked packet from send cache queue
	m_send_cache_que_mtx.lock();
	for (auto i = m_send_cache_que.begin(); i != m_send_cache_que.end();) {
		if ((*i)->psn < pkt->head.psn) {
			i = m_send_cache_que.erase(i);
			m_send_cache_que_size--;
		}
		else {
			++i;
		}
	}
	m_send_cache_que_mtx.unlock();

	if (pkt->buf.data_len > 0 
		&& m_sess_param.session_type == SessionType::RELIABLE) {
		ProcNack(pkt);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionSender::SendAck2(uint32_t ack_sn, uint64_t ts)
{
	Buffer pkt = SessionProtocol::BuildAck2Pkt(m_sess_param.local_sid,
		m_sess_param.remote_sid, ack_sn, ts);

	if (ERR_CODE_OK != m_pkt_sender->SendPkt(SESSION_PKT_ACK2, pkt)) {
		LOG_ERR("[session:{}] Send ack2 failed!", m_sess_param.local_sid);
	}
}

}