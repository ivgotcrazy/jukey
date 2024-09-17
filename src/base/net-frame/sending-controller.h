#pragma once 

#include "if-sending-controller.h"
#include "if-session.h"
#include "if-rtt-filter.h"
#include "if-session-pkt-sender.h"
#include "link-cap-estimator.h"
#include "if-data-splitter.h"
#include "fec/if-fec-encoder.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class SendingController : public ISendingController
{
public:
	SendingController(const SessionParam& param, 
		IRttFilterSP filter, 
		ISessionPktSenderSP sender,
		LinkCapEstimatorSP estimator);

	// ISendingController
	virtual void Update() override;
	virtual bool PushSessionData(const com::Buffer& buf) override;
	virtual uint64_t GetNextSendTime() override;
	virtual SendResult SendSessionData(CacheSessionPktList& cache_list) override;
	virtual void SetFecParam(const FecParam& param) override;

private:
	bool OnSessionDataPush(const com::Buffer& buf);
	bool OnFecDataPush(const com::Buffer& buf);
	void TryEncodingFecData();
	SendResult SendWithSingleMode(CacheSessionPktList& cache_list);
	SendResult SendWithGroupMode(CacheSessionPktList& cache_list);

private:
	const SessionParam& m_sess_param;
	std::mutex m_mutex;
	bool m_pending = false;

	// Send a group of fec data once a time
	bool m_fec_batch_send = true;

	util::IFecEncoderUP m_fec_encoder;
	FecParam m_fec_param;

	uint16_t m_fec_next_group = 1;
	uint64_t m_last_cache_ts = 0;
	uint32_t m_fec_next_sn = 1;

	// Wait for fec encode
	std::list<com::Buffer> m_send_cache_que;
	uint32_t m_cache_que_size = 0;

	std::list<com::Buffer> m_send_wait_que;
	uint32_t m_wait_que_size = 0;

	IRttFilterSP m_rtt_filter;
	ISessionPktSenderSP m_session_pkt_sender;
	LinkCapEstimatorSP m_link_cap_estimator;
	IDataSplitterUP m_data_splitter;
};

}