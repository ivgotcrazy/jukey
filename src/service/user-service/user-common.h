#pragma once

#include <optional>

#include "common-struct.h"
#include "common-message.h"

#include "protoc/mq.pb.h"
#include "protoc/user.pb.h"
#include "protoc/terminal.pb.h"
#include "protoc/topo.pb.h"

namespace jukey::srv
{

//==============================================================================
// TODO: 
//==============================================================================
enum UserMsgType
{
	USER_MSG_INVALID = SERVICE_USER_MSG_START + 0,
	USER_MSG_MQ_MSG  = SERVICE_USER_MSG_START + 1
};

//==============================================================================
// TODO: 
//==============================================================================
struct UserEntry
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	uint64_t login_time = 0;
};

typedef std::shared_ptr<UserEntry> UserEntrySP;

typedef std::pair<prot::MqMsg, prot::UserLoginReq> UserLoginReqPair;
typedef std::optional<UserLoginReqPair> UserLoginReqPairOpt;

typedef std::pair<prot::MqMsg, prot::UserLogoutReq> UserLogoutReqPair;
typedef std::optional<UserLogoutReqPair> UserLogoutReqPairOpt;

typedef std::pair<prot::MqMsg, prot::ClientOfflineNotify> ClientOfflineNotifyPair;
typedef std::optional<ClientOfflineNotifyPair> ClientOfflineNotifyPairOpt;

typedef std::pair<prot::MqMsg, prot::Ping> ServicePingPair;
typedef std::optional<ServicePingPair> ServicePingPairOpt;

}