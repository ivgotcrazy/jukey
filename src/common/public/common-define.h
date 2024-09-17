#pragma once

//==============================================================================
// Query component interface
//==============================================================================
#define QI m_factory->QueryInterface

//==============================================================================
// Static Pointer Cast
//==============================================================================
#define SPC std::static_pointer_cast

//==============================================================================
// Pointer Cast CommonMsg Data
//==============================================================================
#define PCAST_COMMON_MSG_DATA(type) \
	std::shared_ptr<type> data = std::static_pointer_cast<type>(msg.msg_data);

//==============================================================================
// Const String Reference
//==============================================================================
#define CSTREF const std::string&

//==============================================================================
// Promise Error Code Shared Pointer
//==============================================================================
#define PRECSP std::shared_ptr<std::promise<jukey::com::ErrCode>>

//==============================================================================
// Promise
//==============================================================================
#define SYNC_ERRCODE_RESULT_WITH_PROMISE(msg, ec) \
  if (msg.result) {                               \
    msg.result->set_value(ec);                    \
  }

//==============================================================================
// 
//==============================================================================
#define SYNC_BOOL_RESULT_WITH_PROMISE(msg, re)                     \
  if (msg.result) {                                                \
    if (re) {                                                      \
      msg.result->set_value(jukey::com::ErrCode::ERR_CODE_OK);     \
    }                                                              \
    else {                                                         \
      msg.result->set_value(jukey::com::ErrCode::ERR_CODE_FAILED); \
    }                                                              \
  }

//==============================================================================
// 
//==============================================================================
#define CONSTRUCT_ELEMENT_NAME(prefix) \
	std::string(prefix).append(std::to_string((intptr_t)this))

#define CONSTRUCT_PIN_NAME(prefix) \
	std::string(prefix).append(std::to_string((intptr_t)this))

//==============================================================================
// 
//==============================================================================
#define QUERY_TIMER_MGR(factory) \
  (jukey::com::ITimerMgr*)(factory)->QueryInterface(CID_TIMER_MGR, \
    IID_TIMER_MGR, "unknown")

//==============================================================================
// DP: Data Pointer
//==============================================================================
#define DP(buf) ((buf).data.get() + (buf).start_pos)

//==============================================================================
// Protocol Header Length
//==============================================================================
#define SIG_HDR_LEN (sizeof(jukey::prot::SigMsgHdr))
#define FEC_HDR_LEN (sizeof(jukey::prot::FecHdr))
#define SEG_HDR_LEN (sizeof(jukey::prot::SegHdr))
#define TLV_HDR_LEN (sizeof(jukey::txp::TLV))

//==============================================================================
// FEC
//==============================================================================
#define FEC_K(hdr)  ((uint32_t)((hdr)->K == 15 ? 16 : (hdr)->K))
#define FEC_R(hdr)  ((uint32_t)((hdr)->R == 15 ? 16 : (hdr)->R))
#define GET_K(val)  ((val) == 16 ? 15 : (val))
#define GET_R(val)  ((val) == 16 ? 15 : (val))

//==============================================================================
// NetStream
//==============================================================================
#define MAPP_ID(media_stream)   (media_stream.src.app_id)
#define MUSER_ID(media_stream)  (media_stream.src.user_id)
#define STRM_ID(media_stream)   (media_stream.stream.stream_id)
#define STRM_TYPE(media_stream) (media_stream.stream.stream_type)
#define MSRC_ID(media_stream)   (media_stream.src.src_id)
#define MSRC_TYPE(media_stream) (media_stream.src.src_type)

//==============================================================================
// Log
//==============================================================================
#define LOGGER_DBG(...) \
	if (LOGGER && LOGGER->GetLogger()) { \
		SPDLOG_LOGGER_DEBUG(LOGGER->GetLogger(), __VA_ARGS__); \
	}

#define LOGGER_INF(...) \
	if (LOGGER && LOGGER->GetLogger()) { \
		SPDLOG_LOGGER_INFO(LOGGER->GetLogger(), __VA_ARGS__); \
	}

#define LOGGER_WRN(...) \
	if (LOGGER && LOGGER->GetLogger()) { \
		SPDLOG_LOGGER_WARN(LOGGER->GetLogger(), __VA_ARGS__); \
	}

#define LOGGER_ERR(...) \
	if (LOGGER && LOGGER->GetLogger()) { \
		SPDLOG_LOGGER_ERROR(LOGGER->GetLogger(), __VA_ARGS__); \
	}

#define LOGGER_CRT(...) \
	if (LOGGER && LOGGER->GetLogger()) { \
		SPDLOG_LOGGER_CRITICAL(LOGGER->GetLogger(), __VA_ARGS__); \
	}

//==============================================================================
// Note: cannot use this macro if has extend data
//==============================================================================
#define PB_PARSE_SIG_PARAM(buf) \
	buf.data.get() + buf.start_pos + sizeof(jukey::prot::SigMsgHdr), \
	buf.data_len - sizeof(jukey::prot::SigMsgHdr)

//==============================================================================
// 
//==============================================================================
#define PB_PARSE_MQ_PARAM(buf) \
	buf.data.get() + buf.start_pos + sizeof(jukey::prot::MqMsgHdr), \
	buf.data_len - sizeof(jukey::prot::MqMsgHdr)