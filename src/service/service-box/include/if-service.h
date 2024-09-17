#pragma once

#include <string>

#include "if-unknown.h"
#include "com-factory.h"
#include "if-session-mgr.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
#define CID_GROUP_SERVICE     "cid-group-service"
#define CID_PROXY_SERVICE     "cid-proxy-service"
#define CID_ROUTE_SERVICE     "cid-route-service"
#define CID_STREAM_SERVICE    "cid-stream-service"
#define CID_TERMINAL_SERVICE  "cid-terminal-service"
#define CID_TRANSPORT_SERVICE "cid-transport-service"
#define CID_USER_SERVICE      "cid-user-service"

//==============================================================================
// 
//==============================================================================
#define IID_SERVICE "iid-service"

//==============================================================================
// 
//==============================================================================
class IService : public base::IUnknown
{
public:
	/**
	 * @brief Initialize service 
	 */
	virtual bool Init(net::ISessionMgr* mgr, const std::string& cfg_file) = 0;

	/**
	 * @brief Start service
	 */
	virtual bool Start() = 0;

	/**
	 * @brief Stop service
	 */
	virtual void Stop() = 0;

	/**
	 * @brief Reload configure
	 */
	virtual bool Reload() = 0;
};

}
