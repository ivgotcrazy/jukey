#include "net-common.h"
#include "session-protocol.h"
#include "log.h"

using namespace jukey::util;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CheckBuffer(const com::Buffer& buf)
{
	if (buf.data_len == 0 || buf.total_len == 0) {
		LOG_ERR("Zero length, data_len: {}, total_len: {}",
			buf.data_len, buf.total_len);
		return false;
	}

	if (buf.total_len < buf.data_len) {
		LOG_ERR("total_len: {} smaller than data_len: {}",
			buf.total_len, buf.data_len);
		return false;
	}

	if (buf.data_len + buf.start_pos > buf.total_len) {
		LOG_ERR("Unexpected data_len {}, start_pos {}, total_len {}",
			buf.data_len, buf.start_pos, buf.total_len);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CheckSessionBuffer(const com::Buffer& buf)
{
	if (!CheckBuffer(buf)) {
		return false;
	}

	if (buf.data_len < SES_PKT_HDR_LEN) {
		LOG_ERR("Invalid packet len: {}", buf.data_len);
		return false;
	}

	SesPktHdr* head = (SesPktHdr*)(buf.data.get());
	if (buf.data_len != head->len + SES_PKT_HDR_LEN) {
		LOG_WRN("Invalid packet, head len:{}, data len:{}", head->len, buf.data_len);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CheckEndpoint(const com::Endpoint& ep)
{
	// TODO: 合法IP地址检查
	if (ep.host.empty()) {
		LOG_ERR("Empty ip address!");
		return false;
	}

	if (ep.port == 0) {
		LOG_ERR("Invalid port!")
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CheckAddress(const com::Address& addr)
{
	if (!CheckEndpoint(addr.ep)) {
		return false;
	}

	if (addr.type == com::AddrType::INVALID) {
		LOG_ERR("Invalid transport type!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
unsigned long GetSinAddr(const com::Endpoint& ep)
{
	return (ep.host.length() == 0) ? 0 : inet_addr(ep.host.c_str());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t Upbound(uint32_t value, uint32_t up_limit)
{
	return value <= up_limit ? value : up_limit;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t CalcCount(uint32_t total, uint32_t unit)
{
	return (total <= unit) ? 1 : (total + unit - 1) / unit;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t LowboundAndUpbound(uint32_t value, uint32_t min, uint32_t max)
{
	if (value < min) {
		return min;
	}
	else if (value > max) {
		return max;
	}
	else {
		return value;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Endpoint GetAddress(sockaddr_in client_addr)
{
	return com::Endpoint(inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Endpoint GetAddress(SocketId sock)
{
	int len = 0;
	struct sockaddr_in addr;
	
#ifdef _WINDOWS
	if (0 != getsockname(sock, (struct sockaddr*)&addr, &len)) {
#else
	if (0 != getsockname(sock, (struct sockaddr*)&addr, (socklen_t*)&len)) {
#endif
		return com::Endpoint();
	}
	else {
		return GetAddress(addr);
	}
}

}
