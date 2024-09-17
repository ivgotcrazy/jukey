#include "media-file-processor.h"
#include "rtc-common.h"
#include "log.h"
#include "rtc-engine-impl.h"


using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaFileProcessor::MediaFileProcessor(RtcEngineImpl* engine)
	: m_rtc_engine(engine)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaFileProcessor::Open(const std::string& file, void* wnd)
{
	LOG_INF("Open media file:{}", file);

	if (ERR_CODE_OK != G(m_media_engine)->OpenMediaFile(file)) {
		LOG_ERR("Open media file failed");
		return ERR_CODE_FAILED;
	}

	m_media_file = file;
	m_wnd = wnd;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaFileProcessor::Close(const std::string& file)
{
	LOG_INF("Close media file:{}", file);

	if (file != m_media_file) {
		LOG_ERR("Invalid media file:{}, file:{}", file, m_media_file);
		return ERR_CODE_FAILED;
	}

	if (G(m_media_engine)->CloseMediaFile(file) != ERR_CODE_OK) {
		LOG_ERR("Close camera failed");
	}

	if (m_audio_stream) {
		com::MediaStream stream = *m_audio_stream.get();

		if (G(m_media_engine)->StopPlayStream(stream) != ERR_CODE_OK) {
			LOG_ERR("Stop play stream failed!");
		}

		if (m_rtc_engine->UnpublishGroupStream(stream) != ERR_CODE_OK) {
			LOG_ERR("Unpublish group stream failed");
		}

		m_audio_stream.reset();
	}

	if (m_video_stream) {
		com::MediaStream stream = *m_video_stream.get();

		if (G(m_media_engine)->StopRenderStream(stream, m_wnd) != ERR_CODE_OK) {
			LOG_ERR("Stop render stream failed!");
		}

		if (m_rtc_engine->UnpublishGroupStream(stream) != ERR_CODE_OK) {
			LOG_ERR("Unpublish group stream failed");
		}

		m_video_stream.reset();
		m_wnd = nullptr;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaFileProcessor::OnAddMediaStream(const MediaStream& stream)
{
	LOG_INF("Add camera stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type, stream.src.src_id,
		stream.stream.stream_type, stream.stream.stream_id);

	if (stream.src.src_type != MediaSrcType::FILE
		|| stream.src.src_id != m_media_file) {
		LOG_ERR("Invalid media stream!");
		return;
	}

	// TODO: Auto publish after opening camera, appropriate?
	if (ERR_CODE_OK != m_rtc_engine->PublishGroupStream(stream)) {
		LOG_ERR("Publish group stream failed");
	}

	if (stream.stream.stream_type == StreamType::AUDIO) {
		if (ERR_CODE_OK != G(m_media_engine)->StartPlayStream(stream)) {
			LOG_ERR("Start play stream failed");
		}
		m_audio_stream.reset(new MediaStream(stream));
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		if (ERR_CODE_OK != G(m_media_engine)->StartRenderStream(stream, m_wnd)) {
			LOG_ERR("Start render stream failed");
		}
		m_video_stream.reset(new MediaStream(stream));
	}
	else {
		LOG_ERR("Invalid stream type:{}", stream.stream.stream_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaFileProcessor::OnDelMediaStream(const MediaStream& stream)
{
	LOG_INF("Remove camera stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type, stream.src.src_id,
		stream.stream.stream_type, stream.stream.stream_id);

	if (stream.stream.stream_type == StreamType::AUDIO) {
		if (m_audio_stream) {
			if (ERR_CODE_OK != m_rtc_engine->UnpublishGroupStream(stream)) {
				LOG_ERR("Unpublish group stream failed");
			}
			if (ERR_CODE_OK != G(m_media_engine)->StopPlayStream(stream)) {
				LOG_ERR("Stop play stream failed");
			}
			m_audio_stream.reset();
		}
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		if (m_video_stream) {
			if (ERR_CODE_OK != m_rtc_engine->UnpublishGroupStream(stream)) {
				LOG_ERR("Unpublish group stream failed");
			}
			if (ERR_CODE_OK != G(m_media_engine)->StopRenderStream(stream, m_wnd)) {
				LOG_ERR("Stop render stream failed");
			}
			m_video_stream.reset();
		}
	}
	else {
		LOG_ERR("Invalid stream type:{}", stream.stream.stream_type);
	}
}

}