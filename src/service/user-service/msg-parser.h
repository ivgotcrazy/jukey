#pragma once

#include <optional>

#include "user-common.h"

namespace jukey::srv
{

UserLoginReqPairOpt ParseUserLoginReq(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

UserLogoutReqPairOpt ParseUserLogoutReq(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

ClientOfflineNotifyPairOpt ParseClientOfflineNotify(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

ServicePingPairOpt ParseServicePing(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

}