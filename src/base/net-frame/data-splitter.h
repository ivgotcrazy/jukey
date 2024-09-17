#pragma once 

#include "if-data-splitter.h"
#include "if-session.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class DataSplitter : public IDataSplitter
{
public:
	DataSplitter(const SessionParam& param);

	// ISessionDataSplitter
	virtual uint32_t SplitSessionData(const com::Buffer& buf, 
		std::list<com::Buffer>& session_pkts) override;
	virtual uint32_t SplitFecData(const com::Buffer& buf,
		std::list<com::Buffer>& session_pkts) override;
	virtual uint32_t GetNextPSN() override;
	virtual uint32_t GetNextMSN() override;

private:
	uint32_t DoSplitData(const com::Buffer& buf, 
		uint32_t fix_data_size,
		uint32_t rsv_hdr_size,
		std::list<com::Buffer>& session_pkts);

private:
	const SessionParam& m_sess_param;

	uint32_t m_next_send_psn = 1;
	uint32_t m_next_send_msn = 1;
};

}