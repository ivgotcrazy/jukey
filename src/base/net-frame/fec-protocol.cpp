#include "fec-protocol.h"
#include "session-protocol.h"
#include "log.h"


namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecProtocol::BuildFecPkt(uint8_t k, uint8_t r, uint32_t sn, uint16_t group_id,
	uint8_t gsn, uint8_t pt, com::Buffer& buf)
{
	FecPktHdr* hdr = (FecPktHdr*)buf.data.get();

	hdr->flg = 1;
	hdr->ver = 1;
	hdr->k = k;
	hdr->r = r;
	hdr->grp = group_id;
	hdr->gsn = gsn;
	hdr->len = buf.total_len;
	hdr->pt = pt;
	hdr->sn = sn;

	buf.start_pos = 0;
	buf.data_len = buf.total_len;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecProtocol::DumpFecPktHdr(const FecPktHdr& pkt_hdr)
{
	LOG_DBG("FEC header, flg:{}, ver:{}, k:{}, r:{}, len:{}, sn:{}, grp:{}, "
		"gsn:{}, pt:{}",
		(uint8_t)pkt_hdr.flg,
		(uint8_t)pkt_hdr.ver,
		(uint8_t)pkt_hdr.k,
		(uint8_t)pkt_hdr.r,
		pkt_hdr.len,
		pkt_hdr.sn,
		pkt_hdr.grp,
		pkt_hdr.gsn,
		pkt_hdr.pt);
}

}