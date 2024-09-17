#include "stream-processor.h"
#include "log.h"
#include "rtc-common.h"
#include "rtc-engine-impl.h"
#include "protoc/stream.pb.h"
#include "stream-msg-builder.h"
#include "common/util-pb.h"
#include "msg-builder.h"
#include "common/util-property.h"

using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamProcessor::StreamProcessor(RtcEngineImpl* engine): m_rtc_engine(engine)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamProcessor::StartRecvVideo(const MediaStream& stream, void* wnd)
{
	LOG_INF("Start recv video, app:{}, user:{}, media:{}|{}, stream:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	if (m_video_entries.find(stream.stream.stream_id) != m_video_entries.end()) {
		LOG_ERR("Video stream already exists, stream:{}|{}",
			STRM_TYPE(stream), STRM_ID(stream));
		return ERR_CODE_FAILED;
	}

	SubscribeStream(stream);

	VideoEntry entry;
	entry.param = stream;
	entry.wnd = wnd;
	m_video_entries.insert(std::make_pair(STRM_ID(stream), entry));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamProcessor::StopRecvVideo(const MediaStream& stream, void* wnd)
{
	LOG_INF("Stop recv audio, app:{}, user:{}, media:{}|{}, stream:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	auto iter = m_video_entries.find(stream.stream.stream_id);
	if (iter == m_video_entries.end() || iter->second.wnd != wnd) {
		LOG_ERR("Cannot find video stream:{}|{}", STRM_TYPE(stream), 
			STRM_ID(stream));
		return ERR_CODE_FAILED;
	}

	UnsubscribeStream(stream);

	// TODO: 这个逻辑不严谨
	if (ERR_CODE_OK != G(m_media_engine)->CloseNetStream(stream)) {
		LOG_ERR("Close net stream failed");
	}

	ErrCode result = G(m_media_engine)->StopRenderStream(stream,
		iter->second.wnd);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Stop render stream failed");
	}

	m_video_entries.erase(iter);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode StreamProcessor::StartRecvAudio(const com::MediaStream& stream)
{
	LOG_INF("Start recv audio, app:{}, user:{}, media:{}|{}, stream:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	return SubscribeStream(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode StreamProcessor::StopRecvAudio(const com::MediaStream& stream)
{
	LOG_INF("Stop recv audio, app:{}, user:{}, media:{}|{}, stream:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	UnsubscribeStream(stream);

	// TODO: 这个逻辑不严谨
	if (ERR_CODE_OK != G(m_media_engine)->CloseNetStream(stream)) {
		LOG_ERR("Close net stream failed");
	}

	ErrCode result = G(m_media_engine)->StopPlayStream(stream);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Stop play stream failed");
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamProcessor::OnAddMediaStream(const MediaStream& stream)
{
	LOG_INF("Add media stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	if (stream.stream.stream_type == StreamType::VIDEO) {
		auto iter = m_video_entries.find(stream.stream.stream_id);
		if (iter == m_video_entries.end()) {
			LOG_ERR("Cannot find video entry");
			return;
		}

		ErrCode result = G(m_media_engine)->StartRenderStream(stream,
			iter->second.wnd);
		if (result != ERR_CODE_OK) {
			LOG_ERR("Start render stream failed");
		}
	}
	else if (stream.stream.stream_type == StreamType::AUDIO) {
		ErrCode result = G(m_media_engine)->StartPlayStream(stream);
		if (result != ERR_CODE_OK) {
			LOG_ERR("Start play stream failed");
		}
	}
	else {
		LOG_ERR("Invalid stream type:{}", stream.stream.stream_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamProcessor::OnSubStreamRsp(const MediaStream& stream, const Buffer& buf)
{
	prot::SubscribeStreamRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse subscribe stream response failed!");
		return;
	}

	LOG_INF("Received subscribe stream response:{}", util::PbMsgToJson(rsp));

	// Notify subscribe result
	if (rsp.result() != 0) {
		G(m_engine_param).handler->OnSubStreamResult(stream, ERR_CODE_FAILED);
	}
	else {
		LOG_INF("Start to open remote stream, stream:{}|{}", STRM_TYPE(stream),
			STRM_ID(stream));

		// Start receiving stream
		ErrCode result = G(m_media_engine)->OpenNetStream(stream, rsp.stream_addr());
		if (ERR_CODE_OK != result) {
			LOG_ERR("Open remote stream failed");
			G(m_engine_param).handler->OnSubStreamResult(stream, ERR_CODE_FAILED);
		}
		else {
			G(m_engine_param).handler->OnSubStreamResult(stream, ERR_CODE_OK);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamProcessor::OnSubStreamTimeout(const MediaStream& stream)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamProcessor::OnSubStreamError(const MediaStream& stream, ErrCode ec)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamProcessor::SubscribeStream(const com::MediaStream& stream)
{
	LOG_INF("Subscribe stream:{}|{}", STRM_TYPE(stream), STRM_ID(stream));

	std::lock_guard<std::recursive_mutex> lock(G(m_mutex));

	if (G(m_engine_state) != RtcEngineState::JOINED) {
		LOG_ERR("Invalid state:{} to subscribe stream!", G(m_engine_state));
		return ERR_CODE_FAILED;
	}

	Buffer buf = G(m_msg_builder)->BuildSubStreamReq(stream, ++G(m_cur_seq));

	G(m_async_proxy)->SendSessionMsg(G(m_proxy_session), buf,
		G(m_cur_seq), prot::MSG_SUBSCRIBE_STREAM_RSP)
		.OnResponse([this, stream](net::SessionId sid, const com::Buffer& buf) {
			OnSubStreamRsp(stream, buf);
		})
		.OnTimeout([this, stream]() {
			OnSubStreamTimeout(stream);
		})
		.OnError([this, stream](const std::string& err) {
			OnSubStreamError(stream, ERR_CODE_FAILED);
		});

	LOG_INF("Send subscribe stream request, app:{}, user:{}, client:{}, seq:{}, "
		"stream:{}|{}",
		G(m_engine_param).app_id,
		G(m_engine_data.user_id),
		G(m_engine_param).client_id,
		G(m_cur_seq),
		STRM_TYPE(stream),
		STRM_ID(stream));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamProcessor::UnsubscribeStream(const MediaStream& stream)
{
	LOG_INF("Unsubscribe stream:{}|{}", STRM_TYPE(stream), STRM_ID(stream));

	std::lock_guard<std::recursive_mutex> lock(G(m_mutex));

	if (G(m_engine_state) != RtcEngineState::JOINED) {
		LOG_ERR("Invalid state:{} to unsubscribe stream!", G(m_engine_state));
		return ERR_CODE_FAILED;
	}

	Buffer buf = G(m_msg_builder)->BuildUnsubStreamReq(stream, ++G(m_cur_seq));

	G(m_async_proxy)->SendSessionMsg(G(m_proxy_session), buf,
		G(m_cur_seq), prot::MSG_UNSUBSCRIBE_STREAM_RSP)
		.OnResponse([this, stream](net::SessionId sid, const Buffer& buf) {
			OnSubStreamRsp(stream, buf);
		})
		.OnTimeout([this, stream]() {
			OnSubStreamTimeout(stream);
		})
		.OnError([this, stream](const std::string& err) {
			OnSubStreamError(stream, ERR_CODE_FAILED);
		});

	LOG_INF("Send unsubscribe stream request, app:{}, user:{}, client:{}, "
		"seq:{}, stream:{}|{}",
		G(m_engine_param).app_id,
		G(m_engine_data.user_id),
		G(m_engine_param).client_id,
		G(m_cur_seq),
		STRM_TYPE(stream),
		STRM_ID(stream));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamProcessor::OnUnsubStreamRsp(const com::MediaStream& stream, 
	const com::Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamProcessor::OnUnsubStreamTimeout(const com::MediaStream& stream)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamProcessor::OnUnsubStreamError(const com::MediaStream& stream, 
	com::ErrCode ec)
{
	LOG_INF("{}", __FUNCTION__);
}

}