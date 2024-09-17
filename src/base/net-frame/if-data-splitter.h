#pragma once

#include <list>
#include "common-struct.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class IDataSplitter
{
public:
	virtual uint32_t SplitSessionData(const com::Buffer& buf, 
		std::list<com::Buffer>& session_pkts) = 0;

	virtual uint32_t SplitFecData(const com::Buffer& buf,
		std::list<com::Buffer>& session_pkts) = 0;

	virtual uint32_t GetNextPSN() = 0;

	virtual uint32_t GetNextMSN() = 0;
};
typedef std::unique_ptr<IDataSplitter> IDataSplitterUP;

}
