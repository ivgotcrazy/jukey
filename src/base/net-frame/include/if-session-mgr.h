#pragma once

#include "if-unknown.h"
#include "common-struct.h"
#include "common-enum.h"
#include "com-factory.h"
#include "net-public.h"
#include "thread/if-thread.h"


namespace jukey::net
{

// Component ID and interface ID
#define CID_SESSION_MGR "cid-session-mgr"
#define IID_SESSION_MGR "iid-session-mgr"

//==============================================================================
// Session manager parameters
//==============================================================================
struct SessionMgrParam
{
	uint32_t thread_count = 0;
	uint32_t ka_interval = 0; // second
	bool reliable = true;     // support reliable session
	bool unreliable = true;   // support unreliable session
};

//==============================================================================
// Create session parameters
//==============================================================================
struct CreateParam
{
	com::Address     remote_addr;
	com::ServiceType service_type = com::ServiceType::INVALID;
	com::FecType     fec_type     = com::FecType::NONE;
	SessionType      session_type = SessionType::INVALID;
	uint32_t         ka_interval  = 0; // in second
	util::IThread*   thread       = nullptr;
};

//==============================================================================
// Listen parameters
//==============================================================================
struct ListenParam
{
	com::Address     listen_addr;
	com::ServiceType listen_srv = com::ServiceType::INVALID;
	util::IThread*   thread = nullptr;
};

//==============================================================================
// Session manager
//==============================================================================
class ISessionMgr : public base::IUnknown
{
public:
	//
	// @brief Initialize session manager
	// @param factory		component factory
	// @param handler		event handler
	// @param thread_count	session manager thread count
	// @return ERR_CODE_OK:success, other:fail
	//
	virtual com::ErrCode Init(const SessionMgrParam& param) = 0;

	//
	// @brief Create session asynchronously
	// @param param Create session parameters
	// @return ERR_CODE_OK:success, other:fail
	//
	virtual SessionId CreateSession(const CreateParam& param) = 0;

	//
	// @brief close session
	// @param sid session ID
	// @return ERR_CODE_OK:success, other:fail
	//
	virtual com::ErrCode CloseSession(SessionId sid) = 0;

	//
	// @brief Add listen address synchronously
	// @param addr	addrss
	// @return ERR_CODE_OK:success, other:fail
	//
	virtual ListenId AddListen(const ListenParam& param) = 0;

	//
	// @brief Remove listen address synchronously
	// @param addr	addrss
	// @return ERR_CODE_OK:success, other:fail
	//
	virtual com::ErrCode RemoveListen(ListenId lid) = 0;

	//
	// @brief Send session data
	// @param sid session ID
	// @param buf session data，max-length:1024 * 1024 * 8
	// @return ERR_CODE_OK:success, other:fail
	//
	virtual com::ErrCode SendData(SessionId sid, const com::Buffer& buf) = 0;

	//
	// @brief Set log level
	// @param level 0:trace, 1:debug, 2:info, 3:warn, 4:error, 5:critical
	//
	virtual void SetLogLevel(uint8_t level) = 0;
};

}