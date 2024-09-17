#pragma once

#include <future>
#include <list>

#include "common-struct.h"
#include "net-public.h"
#include "common/util-stats.h"
#include "thread/if-thread.h"

#ifdef _WIN32
#include "Winsock2.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

namespace jukey::net
{

//==============================================================================
// packet position
//==============================================================================
enum PktPos
{
	PKT_POS_MIDDLE = 0, // 00
	PKT_POS_LAST   = 1, // 01
	PKT_POS_FIRST  = 2, // 10
	PKT_POS_ONLY   = 3, // 11
};

//==============================================================================
// 
//==============================================================================
struct FecParam
{
	uint8_t k = 0;
	uint8_t r = 0;
};

//==============================================================================
// 
//==============================================================================
struct CacheSessionPkt
{
	CacheSessionPkt() {}

	CacheSessionPkt(const com::Buffer& b, uint32_t t, uint32_t r, uint32_t p,
		uint32_t m, uint32_t x, uint32_t f)
		: buf(b), ts(t), rto(r), psn(p), msn(m), xmit(x), frtx(f) {}

	com::Buffer buf;
	uint32_t ts = 0;
	uint32_t rto = 0;
	uint32_t psn = 0;
	uint32_t msn = 0;
	uint32_t xmit = 0;
	uint32_t frtx = 0; // fast retransmit ack
	bool sack = false; // selective ack
};
typedef std::shared_ptr<CacheSessionPkt> CacheSessionPktSP;
typedef std::list<CacheSessionPktSP> CacheSessionPktList;

////////////////////////////////////////////////////////////////////////////////

bool CheckBuffer(const com::Buffer& buf);
bool CheckSessionBuffer(const com::Buffer& buf);
bool CheckEndpoint(const com::Endpoint& ep);
bool CheckAddress(const com::Address& addr);
unsigned long GetSinAddr(const com::Endpoint& ep);
uint32_t Upbound(uint32_t value, uint32_t up_limit);
uint32_t CalcCount(uint32_t total, uint32_t unit);
uint32_t LowboundAndUpbound(uint32_t value, uint32_t min, uint32_t max);
com::Endpoint GetAddress(sockaddr_in client_addr);
com::Endpoint GetAddress(SocketId sock);

}
