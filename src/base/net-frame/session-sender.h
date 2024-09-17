#pragma once

#include <map>
#include <atomic>

#include "net-public.h"
#include "common-struct.h"
#include "if-session.h"
#include "if-congestion-controller.h"
#include "if-session-pkt-sender.h"
#include "common/util-stats.h"
#include "com-factory.h"
#include "if-rtt-filter.h"
#include "link-cap-estimator.h"
#include "if-sending-controller.h"
#include "if-fec-controller.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class SessionSender
{
public:
	SessionSender(base::IComFactory* factory, 
		const SessionParam& param, 
		const ISessionPktSenderSP& sender,
		const IRttFilterSP& filter,
		const LinkCapEstimatorSP& estimator);
	~SessionSender();

	void OnUpdate();
	bool PushSessionData(const com::Buffer& buf);
	void SendSessionData();
	void OnSessionReport(const SessionPktSP& pkt);
	void OnSessionAck(const SessionPktSP& pkt);
	void OnSessionNegotiateComplete();
	uint64_t NextSendTime();

private:
	void InitStats(base::IComFactory* factory);
	void SendAck2(uint32_t ack_sn, uint64_t ts);
	void ProcNack(const SessionPktSP& pkt);

private:
	const SessionParam& m_sess_param;
	ICongestionControllerUP m_cong_ctrl;
	ISendingControllerUP m_send_ctrl;
	ISessionPktSenderSP m_pkt_sender;
	IRttFilterSP m_rtt_filter;
	LinkCapEstimatorSP m_link_cap_estimator;
	IFecControllerUP m_fec_ctrl;

	// unacked
	std::list<CacheSessionPktSP> m_send_cache_que;
	std::mutex m_send_cache_que_mtx;

	// Also is inflight
	std::atomic<uint32_t> m_send_cache_que_size = 0;

	uint32_t m_last_ack_psn = 1;

	uint32_t m_send_wnd = 32;

	// Remaining reception window size feedback from receiver
	uint32_t m_remote_wnd = 128;

	// Fast retransmission threshold
	uint32_t m_fast_rtx_threshold = 3;

	// Statistics
	util::DataStatsSP m_data_stats;
	util::StatsId m_i_rto_rtx = INVALID_STATS_ID;
	util::StatsId m_i_nack_rtx = INVALID_STATS_ID;
	util::StatsId m_i_send_pkt = INVALID_STATS_ID;
	util::StatsId m_t_send_pkt = INVALID_STATS_ID;
	util::StatsId m_i_ack_pkt = INVALID_STATS_ID;
	util::StatsId m_t_send_bytes = INVALID_STATS_ID;
	util::StatsId m_i_send_bitrate = INVALID_STATS_ID;
	util::StatsId m_i_aver_rtt = INVALID_STATS_ID;
	util::StatsId m_i_recv_pkt_rate = INVALID_STATS_ID;
	util::StatsId m_i_link_cap = INVALID_STATS_ID;
	util::StatsId m_i_ses_loss = INVALID_STATS_ID;
	util::StatsId m_i_fec_loss = INVALID_STATS_ID;
};
typedef std::unique_ptr<SessionSender> SessionSenderUP;

}