#pragma once

#include "common-struct.h"

namespace jukey::net
{

#define FEC_PKT_TYPE 28 // magic

/*******************************************************************************
 * FEC header
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | f | v |   pt  |       k       |       r       |      sn       |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |           group id            |             length            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                        sequence number                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 |                         data(optional)                        |
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * f(2bits): flag, 00:session protocol, 01:fec protocol, others:undefined
 * v(2bits): version
 * pt(4bits): payload type
*******************************************************************************/

//==============================================================================
// 
//==============================================================================
struct FecPktHdr
{
	uint8_t  flg:2; // flag
	uint8_t  ver:2; // version
	uint8_t  pt:4;  // payload type
	uint8_t  k;     // source data count
	uint8_t  r;     // repair data count
	uint8_t  gsn;   // sequence number in FEC group
	uint16_t grp;   // fec group ID
	uint16_t len;   // data length(without padding)
	uint32_t sn;    // global sequence number
};

#define FEC_PKT_HDR_LEN sizeof(FecPktHdr)

//==============================================================================
// 
//==============================================================================
enum FecDataType
{
	FEC_DATA_TYPE_UNKNOWN = 0,
	FEC_DATA_TYPE_AUDIO = 1,
	FEC_DATA_TYPE_VIDEO = 2
};

//==============================================================================
// 
//==============================================================================
struct FecPkt
{
	FecPkt()
	{
		memset(&head, 0, sizeof(head));
	}

	FecPkt(uint32_t buf_len) : buf(buf_len)
	{
		memset(&head, 0, sizeof(head));
	}

	FecPkt(const FecPktHdr& h, const com::Buffer& b)
		: head(h), buf(b) {}

	FecPkt(const FecPktHdr& h, const com::Buffer& b, uint64_t t)
		: head(h), buf(b), ts(t) {}

	FecPktHdr head;
	com::Buffer buf;
	uint64_t ts = 0;
};
typedef std::shared_ptr<FecPkt> FecPktSP;

//==============================================================================
// 
//==============================================================================
class FecProtocol
{
public:
	static void BuildFecPkt(uint8_t k, uint8_t r, uint32_t sn, uint16_t group_id, 
		uint8_t gsn, uint8_t pt, com::Buffer& buf);

	static void DumpFecPktHdr(const FecPktHdr& pkt_hdr);
};

}