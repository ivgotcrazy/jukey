#pragma once

#include <memory>
#include <map>

#include "common-struct.h"
#include "fec/if-fec-decoder.h"
#include "fec-protocol.h"
#include "if-session.h"
#include "common/util-stats.h"
#include "lost-pkt-tracer.h"
#include "recv-pkt-tracer.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
struct AssemblerInfo
{
	uint32_t recv_pkt_count;
	uint32_t loss_pkt_count;
};

//==============================================================================
// 
//==============================================================================
class FecPktAssembler
{
public:
	FecPktAssembler(base::IComFactory* factory, const SessionParam& param);
	~FecPktAssembler();

	void Update();
	void InputFecData(const com::Buffer& buf);
	bool GetNextSourceData(com::Buffer& buf);
	AssemblerInfo GetInfo();

private:
	void InitStats(base::IComFactory* factory);
	void ProcSingleFecData(const com::Buffer& buf);
	void ProcNormalFecData(const com::Buffer& buf);
	void TryRebuildFecDecoder(const FecPktHdr* fec_hdr);
	void TryDecodeCachedPkts();

private:
	const SessionParam& m_sess_param;

	LostPktTracerUP m_lost_pkt_tracer;
	RecvPktTracerUP m_recv_pkt_tracer;

	util::IFecDecodeUP m_fec_decoder;
	FecParam m_fec_param;

	// Current process group
	uint32_t m_fec_group = 0;

	uint32_t m_fec_next_sn = 1;

	// key:group sequence
	std::map<uint32_t, FecPktSP> m_cache_fec_pkts;
	uint32_t m_put_wait_count = 0;

	// Wait app to receive
	std::list<com::Buffer> m_wait_source_data;

	// Receive packet statistics
	uint64_t m_last_stats_ts = 0;
	uint32_t m_recv_pkt_count = 0;

	// Statistics
	util::DataStatsSP m_data_stats;
	util::StatsId m_i_decode_fail = INVALID_STATS_ID;
	util::StatsId m_i_fec_group = INVALID_STATS_ID;
	util::StatsId m_i_single_fec = INVALID_STATS_ID;
	util::StatsId m_i_fec_pkt = INVALID_STATS_ID;
	util::StatsId m_i_fec_lost = INVALID_STATS_ID;
	util::StatsId m_i_recv_bitrate = INVALID_STATS_ID;
};
typedef std::shared_ptr<FecPktAssembler> FecPktAssemblerSP;

}