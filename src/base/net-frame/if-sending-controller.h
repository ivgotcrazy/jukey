#pragma once

#include "common-struct.h"
#include "net-common.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
struct SendResult
{
	uint32_t send_count = 0;
	uint32_t cache_count = 0;
	uint32_t send_size = 0;
};

//==============================================================================
// 
//==============================================================================
class ISendingController
{
public:
	virtual ~ISendingController() {}

	virtual void Update() = 0;

	virtual bool PushSessionData(const com::Buffer& buf) = 0;

	virtual uint64_t GetNextSendTime() = 0;

	virtual SendResult SendSessionData(CacheSessionPktList& cache_list) = 0;

	virtual void SetFecParam(const FecParam& param) = 0;
};
typedef std::unique_ptr<ISendingController> ISendingControllerUP;

}