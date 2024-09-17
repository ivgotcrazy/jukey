#pragma once

#include "common-struct.h"
#include "session-protocol.h"
#include "net-public.h"
#include "net-message.h"
#include "thread/if-thread.h"

namespace jukey::net
{

//==============================================================================
// Session parameters
// kai: keep alive interval
//==============================================================================
struct SessionParam
{
	com::Address     remote_addr;
	com::FecType     fec_type = com::FecType::NONE;
	com::ServiceType service_type = com::ServiceType::INVALID;
	SessionType      session_type = SessionType::INVALID;
	SessionRole      session_role = SessionRole::INVALID;
	uint32_t         local_kai = 0;  // in second
	uint32_t         remote_kai = 0; // in second
	uint32_t         peer_seen_ip = 0;
	SessionId        local_sid = INVALID_SESSION_ID;
	SessionId        remote_sid = INVALID_SESSION_ID;
	SocketId         sock = 0;
	util::IThread*   thread = nullptr;

	// ugly!!!
	uint8_t fec_k = 4;
	uint8_t fec_r = 1;
};

//==============================================================================
// 
//==============================================================================
class ISession
{
public:
	virtual ~ISession() {}

	//
	// Initialize
	//
	virtual void Init() = 0;

	//
	// Timer driven callback
	//
	virtual void OnUpdate() = 0;

	//
	// Received session data
	//
	virtual void OnRecvData(const com::Buffer& buf) = 0;

	//
	// Add session data to buffer
	//
	virtual bool OnSendData(const com::Buffer& buf) = 0;

	//
	// Close session
	//
	virtual void Close(bool active) = 0;

	//
	// Get session parameters
	//
	virtual const SessionParam& GetParam() = 0;
};
typedef std::shared_ptr<ISession> ISessionSP;

}