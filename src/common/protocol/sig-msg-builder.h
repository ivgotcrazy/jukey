#pragma once

#include <string>

#include "common-struct.h"
#include "protocol.h"

namespace jukey::prot::util
{

template <class T>
void ConstructSigMsg(uint32_t msg_type, com::Buffer& buf, T& msg,
	const com::SigHdrParam& hdr_param)
{
	// Write header
	SigMsgHdr* header = (SigMsgHdr*)buf.data.get();
	header->app = hdr_param.app_id;
	header->grp = hdr_param.group_id;
	header->usr = hdr_param.user_id;
	header->clt = hdr_param.client_id;
	header->ver = 1;
	header->g = hdr_param.clear_group ? 1 : 0;
	header->u = hdr_param.clear_user ? 1 : 0;
	header->c = hdr_param.clear_client ? 1 : 0;
	header->mt = msg_type;
	header->len = (uint16_t)msg.ByteSizeLong(); // TODO:
	header->seq = hdr_param.seq;

	// Write body
	msg.SerializeToArray(DP(buf) + sizeof(SigMsgHdr), (int)msg.ByteSizeLong());

	// Set buffer data length
	buf.data_len = (uint32_t)(msg.ByteSizeLong() + sizeof(SigMsgHdr));
}

}