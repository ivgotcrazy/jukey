#include "data-splitter.h"
#include "session-protocol.h"
#include "common-config.h"
#include "fec-protocol.h"


using namespace jukey::com;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DataSplitter::DataSplitter(const SessionParam& param) : m_sess_param(param)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t DataSplitter::DoSplitData(const Buffer& buf, uint32_t fix_data_size,
	uint32_t rsv_hdr_size, std::list<com::Buffer>& session_pkts)
{
	uint32_t split_count = 0;
	uint32_t data_len = buf.data_len;
	uint32_t frag_count = CalcCount(buf.data_len, MAX_FRAG_SIZE);
	PktPos pos = PKT_POS_ONLY;
	char* data_pos = (char*)buf.data.get() + buf.start_pos;

	for (uint32_t i = 0; i < frag_count; i++) {
		uint32_t read_len = Upbound(data_len, MAX_FRAG_SIZE);

		if (frag_count > 1) {
			if (i == 0) {
				pos = PKT_POS_FIRST;
			}
			else if (i == frag_count - 1) {
				pos = PKT_POS_LAST;
			}
			else {
				pos = PKT_POS_MIDDLE;
			}
		}

		com::Buffer pkt = SessionProtocol::BuildSessionDataPkt(false,
			m_sess_param.local_sid,
			m_sess_param.remote_sid,
			pos,
			m_next_send_psn++,
			m_next_send_msn,
			data_pos,
			read_len,
			rsv_hdr_size,
			fix_data_size);

		session_pkts.push_back(pkt);
		split_count++;

		data_len -= read_len;
		data_pos += read_len;
	}

	++m_next_send_msn;

	return split_count;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t DataSplitter::SplitSessionData(const Buffer& buf,
	std::list<com::Buffer>& session_pkts)
{
	return DoSplitData(buf, 0, 0, session_pkts);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t DataSplitter::SplitFecData(const Buffer& buf,
	std::list<com::Buffer>& session_pkts)
{
	return DoSplitData(buf, MAX_FRAG_SIZE, FEC_PKT_HDR_LEN, session_pkts);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t DataSplitter::GetNextPSN()
{
	return m_next_send_psn;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t DataSplitter::GetNextMSN()
{
	return m_next_send_msn;
}

}