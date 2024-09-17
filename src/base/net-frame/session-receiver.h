#pragma once

#include <unordered_map>

#include "net-public.h"
#include "session-protocol.h"
#include "common/util-stats.h"
#include "if-session.h"
#include "com-factory.h"
#include "if-session-pkt-sender.h"
#include "link-cap-estimator.h"
#include "recv-rate-caculator.h"
#include "if-rtt-filter.h"
#include "lost-pkt-tracer.h"
#include "recv-pkt-tracer.h"
#include "fec-pkt-assembler.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class SessionReceiver
{
public:
	SessionReceiver(base::IComFactory* factory, 
		const SessionParam& param, 
		const ISessionPktSenderSP& sender,
		const IRttFilterSP& filter,
		const LinkCapEstimatorSP& estimator,
		const FecPktAssemblerSP& assembler);
	~SessionReceiver();

	void OnUpdate();
	void OnSessionData(const SessionPktSP& pkt);
	void OnSessionAck2(const SessionPktSP& pkt);
	SessionPktSP GetSessionMsg();

private:
	typedef std::list<SessionPktSP> SessionPktList;

	void InitStats(base::IComFactory* factory);
	void OnRecvCachingSessionPkt(const SessionPktSP& pkt);
	void OnRecvOrderedSessionPkt(const SessionPktSP& pkt);
	void SendReport(const ReportData& report_data);
	void SendAck();
	void FillLostPackets(std::vector<uint32_t>& sns);
	void OnReceivedUnorderedPkt(const SessionPktSP& pkt);

	uint32_t GetReassembledLen(const SessionPktList& pkt_list);

	SessionPktSP GetReliableSessionMsg();
	SessionPktSP GetUnreliableSessionMsg();

	void ProcReliableSessionPkt(const SessionPktSP& pkt);
	void ProcUnreliableSessionPkt(const SessionPktSP& pkt);

private:
	const SessionParam& m_sess_param;
	LinkCapEstimatorSP m_link_cap_estimator;
	ISessionPktSenderSP m_pkt_sender;
	RecvRateCaculator m_recv_rate_caculator;
	LostPktTracerUP m_lost_pkt_tracer;
	RecvPktTracerUP m_recv_pkt_tracer;
	IRttFilterSP m_rtt_filter;
	FecPktAssemblerSP m_fec_assembler;

	//////////////////// Reliable //////////////////////

	// Wait app to fetch
	SessionPktList m_recv_wait_que;
	uint32_t m_wait_que_size = 0;

	// Reorder and reassemble
	SessionPktList m_recv_cache_que;
	uint32_t m_cache_que_size = 0;

	uint32_t m_next_recv_psn = 1;
	uint32_t m_next_recv_msn = 1;

	uint32_t m_last_send_ack_psn = 1;
	uint32_t m_last_send_ack_msn = 1;

	uint64_t m_last_send_ack_ts = 0;

	uint32_t m_last_recv_ack2_msn = 1;

	// ACK own sequence number
	uint32_t m_next_ack_sn = 1;

	//////////////////// Unreliable //////////////////////

	std::unordered_map<uint32_t, SessionPktList> m_recv_wait_map;
	uint32_t m_wait_pkt_size = 0;

	struct WaitEntry
	{
		uint64_t ts;
		uint32_t msn;
		bool complete = false;
	};
	std::list<WaitEntry> m_wait_entries;

	// Receive packet statistics
	uint64_t m_last_stats_ts = 0;
	uint32_t m_recv_pkt_count = 0;

	// Statistics
	util::DataStatsSP m_data_stats;
	util::StatsId m_i_unor_frg = INVALID_STATS_ID;
	util::StatsId m_i_recv_frg = INVALID_STATS_ID;
	util::StatsId m_i_rtx_frg = INVALID_STATS_ID;
	util::StatsId m_i_ack_frg = INVALID_STATS_ID;
	util::StatsId m_t_recv_frg = INVALID_STATS_ID;
	util::StatsId m_t_recv_bytes = INVALID_STATS_ID;
	util::StatsId m_i_recv_br = INVALID_STATS_ID;
	util::StatsId m_i_recv_loss = INVALID_STATS_ID;
	util::StatsId m_i_aver_rtt = INVALID_STATS_ID;
	util::StatsId m_i_drop_frg = INVALID_STATS_ID;
	util::StatsId m_t_recv_pkt = INVALID_STATS_ID;
	util::StatsId m_i_recv_pkt = INVALID_STATS_ID;
};
typedef std::unique_ptr<SessionReceiver> SessionReceiverUP;

}