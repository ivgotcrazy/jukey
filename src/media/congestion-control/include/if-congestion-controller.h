#pragma once

#include <map>
#include <inttypes.h>

#include "if-unknown.h"
#include "common-struct.h"

#define CID_GCC_CONGESTION_CONTROLLER "cid-gcc-congestion-controller"
#define IID_GCC_CONGESTION_CONTROLLER "iid-gcc-congestion-controller"

#define CID_WEBRTC_TFB_ADAPTER "cid-webrtc-tfb-adapter"
#define IID_WEBRTC_TFB_ADAPTER "iid-webrtc-tfb-adapter"

namespace jukey::cc
{

//==============================================================================
// 目标码率变化通知
//==============================================================================
class IBandwidthObserver
{
public:
	virtual void OnBandwidthUpdate(uint32_t bw_kbps) = 0;
};

//==============================================================================
// 
//==============================================================================
struct PacingInfo
{
	uint32_t send_bps = 0;
	int probe_cluster_id = -1;
	int probe_cluster_min_probes = -1;
	int probe_cluster_min_bytes = -1;
	int probe_cluster_bytes_sent = 0;
};

//==============================================================================
// 发送带宽探测报文
//==============================================================================
class IPacketSender
{
public:
	virtual void SendPacket(const com::Buffer& buf, const PacingInfo& info) = 0;

	virtual std::vector<com::Buffer> GeneratePadding(uint32_t data_size) = 0;
};

//==============================================================================
// 
//==============================================================================
struct LossReport
{
	uint32_t loss_count = 0;
	uint32_t recv_count = 0;
	uint32_t recv_time = 0;
	uint32_t start_time = 0;
	uint32_t end_time = 0;
};

//==============================================================================
// 
//==============================================================================
struct PktSendInfo
{
	uint16_t seq = 0;
	uint32_t ts = 0;
	uint32_t len = 0;
	com::PktMediaType pmt = com::PktMediaType::PMT_PADDING;
	PacingInfo pacing_info;
};

//==============================================================================
// 
//==============================================================================
struct  SentPktInfo
{
	int64_t seq = -1;
	int64_t send_time_ms = -1;
	uint32_t size_bytes = 0;
};

//==============================================================================
// 
//==============================================================================
struct BitrateConfig
{
	uint32_t min_bitrate_kbps = 0;
	uint32_t max_bitrate_kbps = 0;
	uint32_t start_bitrate_kbps = 0;
};

//==============================================================================
// 拥塞控制器接口
//==============================================================================
class ICongetionController : public base::IUnknown
{
public:
	virtual void Init(IBandwidthObserver* observer, IPacketSender* sender) = 0;
	virtual void SetBitrateConfig(const BitrateConfig& config) = 0;
	virtual void OnTransportConnected() = 0;
	virtual void OnTransportDisconnected() = 0;
	virtual void OnRttUpdate(uint32_t rtt_ms) = 0;
	virtual void OnLossReport(const LossReport& report) = 0;
	virtual void OnTransportFeedback(const com::Buffer& buf) = 0;
	virtual void OnAddPacket(const PktSendInfo& info) = 0;
	virtual void OnSentPacket(const SentPktInfo& info) = 0;
};

//==============================================================================
// WebRTC Transport Feedback Adapter
//==============================================================================
class IWebrtcTfbAdapter : public base::IUnknown
{
public:
	virtual void Init(uint16_t base_seq, uint32_t base_ts_ms, uint8_t fb_sn) = 0;
	virtual bool AddReceivedPacket(uint16_t seq, uint32_t ts_ms) = 0;
	virtual com::Buffer Serialize() = 0;
};

}
