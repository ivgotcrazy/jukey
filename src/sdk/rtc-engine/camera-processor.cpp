#include "camera-processor.h"
#include "rtc-common.h"
#include "log.h"
#include "rtc-engine-impl.h"


using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CameraProcessor::CameraProcessor(RtcEngineImpl* engine): m_rtc_engine(engine)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraProcessor::OpenCamera(const CamParam& param, void* wnd)
{
	LOG_INF("Open camera, device:{}, frame rate:{}, pixel format:{}, res:{}",
		param.dev_id, param.frame_rate, param.pixel_format, param.resolution);

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_camera_entries.find(param.dev_id) != m_camera_entries.end()) {
		LOG_ERR("Camera is opening, device:{}", param.dev_id);
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != G(m_media_engine)->OpenCamera(param)) {
		LOG_ERR("Open camera failed");
		return ERR_CODE_FAILED;
	}

	m_camera_entries.insert(std::make_pair(param.dev_id, CameraEntry(param, wnd)));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraProcessor::CloseCamera(uint32_t dev_id)
{
	LOG_INF("Close camera:{}", dev_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_camera_entries.find(dev_id);
	if (iter == m_camera_entries.end()) {
		LOG_ERR("Cannot find camera entry!");
		return ERR_CODE_FAILED;
	}

	ErrCode result = G(m_media_engine)->CloseCamera(dev_id);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Close camera failed");
	}

	MediaStream stream = iter->second.stream;

	if (stream.src.src_type != MediaSrcType::INVALID) {
		result = G(m_media_engine)->StopRenderStream(stream, iter->second.wnd);
		if (result != ERR_CODE_OK) {
			LOG_ERR("Stop render stream failed!");
		}

		result = m_rtc_engine->UnpublishGroupStream(stream);
		if (result != ERR_CODE_OK) {
			LOG_ERR("Unpublish group stream failed");
		}
	}

	m_camera_entries.erase(iter);

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CameraProcessor::OnAddCamStream(const MediaStream& stream)
{
	LOG_INF("Add camera stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type, stream.src.src_id,
		stream.stream.stream_type, stream.stream.stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);
	
	auto iter = m_camera_entries.find(atoi(stream.src.src_id.c_str()));
	if (iter == m_camera_entries.end()) {
		LOG_ERR("Cannot find camera entry, device:{}", stream.src.src_id);
		return;
	}

	// TODO: Auto publish after opening camera, appropriate?
	if (ERR_CODE_OK != m_rtc_engine->PublishGroupStream(stream)) {
		LOG_ERR("Publish group stream failed");
	}

	iter->second.stream = stream;

	if (ERR_CODE_OK != G(m_media_engine)->StartRenderStream(stream,
		iter->second.wnd)) {
		LOG_ERR("Start render stream failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CameraProcessor::OnDelCamStream(const MediaStream& stream)
{
	LOG_INF("Remove camera stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type, stream.src.src_id,
		stream.stream.stream_type, stream.stream.stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	int dev_id = atoi(stream.src.src_id.c_str());

	auto iter = m_camera_entries.find(dev_id);
	if (iter == m_camera_entries.end()) {
		LOG_ERR("Cannot find camera entry, device:{}", stream.src.src_id);
		return;
	}

	if (ERR_CODE_OK != m_rtc_engine->UnpublishGroupStream(stream)) {
		LOG_ERR("Unpublish group stream failed");
	}

	if (ERR_CODE_OK != G(m_media_engine)->StopRenderStream(stream,
		iter->second.wnd)) {
		LOG_ERR("Start render stream failed, stream:{}", stream.stream.stream_id);
	}

	m_camera_entries.erase(dev_id);
}

}