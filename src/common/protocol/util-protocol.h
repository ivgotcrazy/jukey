#pragma once

#include <string>

#include "log/spdlog-wrapper.h"
#include "protocol.h"
#include "protoc/group.pb.h"

namespace jukey::prot::util
{

std::string MSG_TYPE_STR(uint32_t msg_type);

void DumpMqHeader(jukey::util::SpdlogWrapperSP logger,
	prot::MqMsgHdr* mq_hdr);

void DumpSignalHeader(jukey::util::SpdlogWrapperSP logger,
	prot::SigMsgHdr* sig_hdr);

com::MediaStream ToMediaStream(const prot::MediaEntry& entry);

}