#include <set>
#include <sstream>

#include "fec-pkt-assembler.h"
#include "fec-protocol.h"
#include "common/util-time.h"
#include "fec/luigi-fec-decoder.h"
#include "log.h"


using namespace jukey::com;
using namespace jukey::util;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FecPktAssembler::FecPktAssembler(base::IComFactory* factory,
	const SessionParam& param) : m_sess_param(param)
{
	m_lost_pkt_tracer.reset(new LostPktTracer(5000000));
	m_recv_pkt_tracer.reset(new RecvPktTracer(5000000));

	m_last_stats_ts = Now();

	InitStats(factory);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FecPktAssembler::~FecPktAssembler()
{
	m_data_stats->Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecPktAssembler::InitStats(base::IComFactory* factory)
{
	std::stringstream fmt;
	fmt << "[session:" << m_sess_param.local_sid << "] FEC assembler,";

	m_data_stats.reset(new util::DataStats(factory, g_net_logger, fmt.str(), false));
	m_data_stats->Start();

	StatsParam i_decode_fail("decode-fail", StatsType::IACCU, 5000);
	m_i_decode_fail = m_data_stats->AddStats(i_decode_fail);

	StatsParam i_fec_group("group-count", StatsType::IACCU, 5000);
	m_i_fec_group = m_data_stats->AddStats(i_fec_group);

	StatsParam i_single_fec("single-group", StatsType::IACCU, 5000);
	m_i_single_fec = m_data_stats->AddStats(i_single_fec);

	StatsParam i_fec_pkt("recv-pkt", StatsType::IACCU, 5000);
	m_i_fec_pkt = m_data_stats->AddStats(i_fec_pkt);

	StatsParam i_fec_lost("lost-pkt", StatsType::ISNAP, 5000);
	m_i_fec_lost = m_data_stats->AddStats(i_fec_lost);

	StatsParam i_recv_bitrate;
	i_recv_bitrate.name = "recv-bitrate";
	i_recv_bitrate.stats_type = StatsType::IAVER;
	i_recv_bitrate.interval = 5000;
	i_recv_bitrate.unit = "kbps";
	i_recv_bitrate.mul_factor = 8;
	i_recv_bitrate.div_factor = 1000;
	m_i_recv_bitrate = m_data_stats->AddStats(i_recv_bitrate);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecPktAssembler::Update()
{
	uint64_t now = jukey::util::Now();

	m_lost_pkt_tracer->Update();
	m_data_stats->OnData(m_i_fec_lost, m_lost_pkt_tracer->GetInfo().lost_count);

	m_recv_pkt_tracer->AddPktCount(m_recv_pkt_count, now - m_last_stats_ts);
	m_last_stats_ts = now;
	m_recv_pkt_count = 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecPktAssembler::ProcSingleFecData(const Buffer& buf)
{
	LOG_DBG("Single fec process");

	assert(m_cache_fec_pkts.empty());

	m_wait_source_data.push_back(buf);
	m_fec_group++;

	m_data_stats->OnData(m_i_single_fec, 1);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecPktAssembler::TryRebuildFecDecoder(const FecPktHdr* fec_hdr)
{
	if ((m_fec_param.k != fec_hdr->k || m_fec_param.r != fec_hdr->r)) {
		LOG_INF("[session:{}] Create new fec decoder, [{}:{}] -> [{}:{}]",
			m_sess_param.local_sid, m_fec_param.k, m_fec_param.r,
			(uint8_t)fec_hdr->k, (uint8_t)fec_hdr->r);

		m_fec_param.k = fec_hdr->k;
		m_fec_param.r = fec_hdr->r;

		m_fec_decoder.reset(new LuigiFecDecoder(m_fec_param.k, m_fec_param.r));

		// TODO:
		m_cache_fec_pkts.clear();
		m_put_wait_count = 0;
		LOG_DBG("Clear cached fec packets");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecPktAssembler::TryDecodeCachedPkts()
{
	// Not enough cached packets
	if (m_cache_fec_pkts.size() < m_fec_param.k) {
		return;
	}

	void** fec_data = new void* [m_fec_param.k];
	int* data_index = new int[m_fec_param.k];

	// Prepare source data index array
	std::set<uint32_t> put_wait_indexes;
	for (uint32_t i = 0; i < m_fec_param.k; i++) {
		put_wait_indexes.insert(i);
	}

	// Fill data array and index array
	uint32_t index = 0, data_len = 0;
	for (auto i = m_cache_fec_pkts.begin(); i != m_cache_fec_pkts.end(); ++i, index++) {
		data_index[index] = i->first;
		// Source data or redundant data
		fec_data[index] = i->second->buf.data.get() + FEC_PKT_HDR_LEN;

		// Remove source data index of received
		put_wait_indexes.erase(i->first);

		// Set and check data length
		if (data_len == 0) {
			data_len = i->second->buf.data_len;
		}
		else {
			if (data_len != 0 && i->second->buf.data_len != data_len) {
				LOG_ERR("Different fec data length:{}, data_len:{}",
					i->second->buf.data_len, data_len);
				goto EXIT;
			}
		}
	}

	// FEC decode
	m_fec_decoder->Decode(fec_data, data_index, data_len);

	// Post decoded data
	for (auto index : put_wait_indexes) {
		auto iter = m_cache_fec_pkts.find(data_index[index]);
		if (iter != m_cache_fec_pkts.end()) {
			m_wait_source_data.push_back(iter->second->buf);
		}
		else {
			LOG_ERR("[session:{}] Cannot find data by index:{}",
				m_sess_param.local_sid, index);
		}
	}

EXIT:
	m_cache_fec_pkts.clear();
	m_put_wait_count = 0;
	m_fec_group++;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecPktAssembler::ProcNormalFecData(const Buffer& buf)
{
	LOG_DBG("Normal fec process");

	FecPktHdr* fec_hdr = (FecPktHdr*)DP(buf);

	TryRebuildFecDecoder(fec_hdr);

	// Source data can be post to upper layer immediately without decoding
	if (fec_hdr->gsn < m_fec_param.k) {
		m_wait_source_data.push_back(buf);
		m_put_wait_count++;
		LOG_DBG("[session:{}] Add source data directly, fec group:{}, gsn:{},"
			" wait size:{}", m_sess_param.local_sid, fec_hdr->grp, fec_hdr->gsn,
			m_wait_source_data.size());
	}

	// All source data of current group has been received, then goto next group
	if (m_put_wait_count >= m_fec_param.k) {
		assert(m_put_wait_count == m_fec_param.k);

		m_fec_group++;
		m_put_wait_count = 0;
		m_cache_fec_pkts.clear();

		LOG_DBG("[session:{}] All source data of fec group:{} has been received",
			m_sess_param.local_sid, fec_hdr->grp);
	}
	else {
		// Make fec packet and cache it
		FecPktSP pkt(new FecPkt(*fec_hdr, buf, util::Now()));
		m_cache_fec_pkts.insert(std::make_pair(fec_hdr->gsn, pkt));

		TryDecodeCachedPkts();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecPktAssembler::InputFecData(const Buffer& buf)
{
	FecPktHdr* fec_hdr = (FecPktHdr*)buf.data.get();

	if (fec_hdr->k == 0) {
		LOG_ERR("Invalid fec k value!");
		return;
	}

	m_data_stats->OnData(m_i_fec_pkt, 1);
	m_data_stats->OnData(m_i_recv_bitrate, buf.data_len);
	m_recv_pkt_count++;

	// Lost stats
	if (fec_hdr->sn > m_fec_next_sn) {
		m_lost_pkt_tracer->AddLostPkt(m_fec_next_sn, fec_hdr->sn);
	}
	else if (fec_hdr->sn < m_fec_next_sn) {
		m_lost_pkt_tracer->RemoveLostPkt(fec_hdr->sn);
	}

	m_fec_next_sn = fec_hdr->sn + 1;

	// Outedated group data
	if (fec_hdr->grp < m_fec_group) {
		LOG_DBG("[session:{}] Drop outdated fec group:{} data, gsn:{}",
			m_sess_param.local_sid, fec_hdr->grp, fec_hdr->gsn);
		return;
	}

	// TODO: unordered fec packet, the process logic is a bit rude
	if (fec_hdr->grp > m_fec_group) {
		LOG_DBG("[session:{}] Incoming fec group:{}, clear fec group:{}",
			m_sess_param.local_sid, fec_hdr->grp, m_fec_group);

		m_cache_fec_pkts.clear();
		m_put_wait_count = 0;

		m_fec_group = fec_hdr->grp;

		m_data_stats->OnData(m_i_decode_fail, 1);
	}

	if (m_cache_fec_pkts.empty()) {
		m_data_stats->OnData(m_i_fec_group, 1);
	}
	else {
		if (m_cache_fec_pkts.find(fec_hdr->gsn) != m_cache_fec_pkts.end()) {
			LOG_WRN("[session:{}] Received repeat fec packet, group:{}, gsn:{}",
				m_sess_param.local_sid, m_fec_group, fec_hdr->gsn);
			return;
		}
	}

	fec_hdr->r == 0 ? ProcSingleFecData(buf) : ProcNormalFecData(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool FecPktAssembler::GetNextSourceData(com::Buffer& buf)
{
	if (m_wait_source_data.empty())
		return false;

	buf = m_wait_source_data.front();
	buf.start_pos = FEC_PKT_HDR_LEN;

	m_wait_source_data.pop_front();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AssemblerInfo FecPktAssembler::GetInfo()
{
	AssemblerInfo info;
	info.recv_pkt_count = m_recv_pkt_tracer->GetPktCount();
	info.loss_pkt_count = m_lost_pkt_tracer->GetInfo().lost_count;

	return info;
}

}