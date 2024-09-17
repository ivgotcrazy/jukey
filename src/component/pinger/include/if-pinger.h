#pragma once

#include <string>
#include <memory>

#include "common-define.h"
#include "component.h"
#include "common-struct.h"

namespace jukey::com
{

#define CID_PINGER "cid-pinger"
#define IID_PINGER "iid-pinger"

//==============================================================================
// 
//==============================================================================
struct ServiceParam
{
	uint32_t service_type = 0;
	std::string service_name;
	std::string instance_id;
};

//==============================================================================
// 
//==============================================================================
class IPingHandler
{
public:
	//
	// Send ping message
	//
	virtual void SendPing(const ServiceParam& param, const Buffer& buf) = 0;

	//
	// Notify ping result, true:success, false:failed
	//
	virtual void OnPingResult(const ServiceParam& param, bool result) = 0;
};

//==============================================================================
// 
//==============================================================================
class IPinger : public base::IUnknown
{
public:
	//
	// Initialize
	// 
	virtual bool Init(const ServiceParam& param, IPingHandler* handler, 
		uint32_t ping_interval/*second*/) = 0;

	//
	// Start
	//
	virtual void Start() = 0;
	
	//
	// Stop
	//
	virtual void Stop() = 0;

	//
	// Add service to ping
	//
	virtual void AddPingService(const ServiceParam& param) = 0;

	//
	// Received pong message
	//
	virtual void OnRecvPongMsg(const com::Buffer& sig_buf) = 0;
};

}