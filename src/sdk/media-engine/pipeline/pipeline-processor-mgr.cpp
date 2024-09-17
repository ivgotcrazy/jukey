#include "pipeline-processor-mgr.h"
#include "log.h"
#include "if-element.h"
#include "camera-src-processor.h"
#include "microphone-src-processor.h"
#include "media-file-processor.h"
#include "audio-stream-src-processor.h"
#include "video-stream-src-processor.h"
#include "encoded-audio-play-processor.h"
#include "raw-audio-play-processor.h"
#include "encoded-video-render-processor.h"
#include "raw-video-render-processor.h"
#include "raw-audio-send-processor.h"
#include "raw-video-send-processor.h"
#include "encoded-audio-send-processor.h"
#include "encoded-video-send-processor.h"
#include "media-engine-impl.h"
#include "pipeline-msg.h"

using namespace jukey::sdk;
using namespace jukey::com;
using namespace jukey::stmr;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PipelineProcessorMgr::~PipelineProcessorMgr()
{
	if (m_br_alloc_mgr) {
		m_br_alloc_mgr->Release();
		m_br_alloc_mgr = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::Init(base::IComFactory* factory, 
	net::ISessionMgr* sm, com::MainThreadExecutor* executor,
	MediaEngineImpl* engine)
{
	m_factory = factory;
	m_sess_mgr = sm;
	m_executor = executor;
	m_engine = engine;

	m_br_alloc_mgr = (IBitrateAllocateMgr*)QI(CID_BITRATE_ALLOCATE_MGR, 
		IID_BITRATE_ALLOCATE_MGR, "");
	if (!m_br_alloc_mgr) {
		LOG_ERR("Create bitrate allocate manager failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::OpenCamera(const CamParam& param)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto processor : m_processors) {
		if (processor->SubType() == PPST_CAMERA) {
			CameraSrcProcessorSP cp = 
				std::dynamic_pointer_cast<CameraSrcProcessor>(processor);
			if (cp->GetCamParam().dev_id == param.dev_id) {
				LOG_ERR("Camera is opened!");
				return ERR_CODE_FAILED;
			}
		}
	}

	CameraSrcProcessorSP processor = std::make_shared<CameraSrcProcessor>(this);
	if (ERR_CODE_OK != processor->Init(param)) {
		LOG_ERR("Init camera processor failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != processor->Start()) {
		LOG_ERR("Start camera processor failed!");
		return ERR_CODE_FAILED;
	}

	m_processors.push_back(processor);

	LOG_INF("Open camera success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::CloseCamera(uint32_t dev_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_processors.begin();
	for (; iter != m_processors.end(); ++iter) {
		if ((*iter)->SubType() == PPST_CAMERA) {
			CameraSrcProcessorSP cp = 
				std::dynamic_pointer_cast<CameraSrcProcessor>(*iter);
			if (cp->GetCamParam().dev_id == dev_id) {
				break;
			}
		}
	}

	if (iter == m_processors.end()) {
		LOG_ERR("Cannot find camera!");
		return ERR_CODE_FAILED;
	}

	// TODO: 检查依赖关系


	// 关闭摄像头
	(*iter)->Stop();

	// 移除 processor
	m_processors.erase(iter);

	LOG_INF("Close camera:{} success, processors:{}", dev_id, m_processors.size());

	for (auto iter = m_streams.begin(); iter != m_streams.end(); ) {
		if (iter->second.stream.stream.src.src_type == MediaSrcType::CAMERA
			&& iter->second.stream.stream.src.src_id == std::to_string(dev_id)) {
			iter = m_streams.erase(iter);
			LOG_INF("Remove stream of camera");
		}
		else {
			++iter;
		}
	}

	return ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::OpenMicrophone(const MicParam& param)
{
	LOG_INF("Open microphone:{}", param.dev_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto processor : m_processors) {
		if (processor->SubType() == PPST_MICROPHONE) {
			MicrophoneSrcProcessorSP cp = 
				std::dynamic_pointer_cast<MicrophoneSrcProcessor>(processor);
			if (cp->GetMicParam().dev_id == param.dev_id) {
				LOG_ERR("Microphone is opened!");
				return ERR_CODE_FAILED;
			}
		}
	}

	MicrophoneSrcProcessorSP processor = 
		std::make_shared<MicrophoneSrcProcessor>(this);
	if (ERR_CODE_OK != processor->Init(param)) {
		LOG_ERR("Init microphone processor failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != processor->Start()) {
		LOG_ERR("Start microphone processor failed!");
		return ERR_CODE_FAILED;
	}

	m_processors.push_back(processor);

	LOG_INF("Open microphone success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::CloseMicrophone(uint32_t dev_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_processors.begin();
	for (; iter != m_processors.end(); ++iter) {
		if ((*iter)->SubType() == PPST_MICROPHONE) {
			MicrophoneSrcProcessorSP cp =
				std::dynamic_pointer_cast<MicrophoneSrcProcessor>(*iter);
			if (cp->GetMicParam().dev_id == dev_id) {
				break;
			}
		}
	}

	if (iter == m_processors.end()) {
		LOG_ERR("Cannot find microphone!");
		return ERR_CODE_FAILED;
	}

	// TODO: 检查依赖关系


	// 关闭摄像头
	(*iter)->Stop();

	// 移除 processor
	m_processors.erase(iter);

	LOG_INF("Close microphone success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::OpenMediaFile(const std::string& file)
{
	LOG_INF("Open media file:{}", file);

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto processor : m_processors) {
		if (processor->SubType() == PPST_MEDIA_FILE) {
			MediaFileProcessorSP cp = 
				std::dynamic_pointer_cast<MediaFileProcessor>(processor);
			if (cp->GetFile() == file) {
				LOG_ERR("Media file is opened!");
				return ERR_CODE_FAILED;
			}
		}
	}

	MediaFileProcessorSP processor 
		= std::make_shared<MediaFileProcessor>(this);
	if (ERR_CODE_OK != processor->Init(file)) {
		LOG_ERR("Init media file processor failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != processor->Start()) {
		LOG_ERR("Start media file processor failed!");
		return ERR_CODE_FAILED;
	}

	m_processors.push_back(processor);

	LOG_INF("Open media file success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::CloseMediaFile(const std::string& file)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_processors.begin();
	for (; iter != m_processors.end(); ++iter) {
		if ((*iter)->SubType() == PPST_MEDIA_FILE) {
			MediaFileProcessorSP cp =
				std::dynamic_pointer_cast<MediaFileProcessor>(*iter);
			if (cp->GetFile() == file) {
				break;
			}
		}
	}

	if (iter == m_processors.end()) {
		LOG_ERR("Cannot find media file!");
		return ERR_CODE_FAILED;
	}

	// TODO: 检查依赖关系


	// 关闭摄像头
	(*iter)->Stop();

	// 移除 processor
	m_processors.erase(iter);

	LOG_INF("Close media file success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::OpenNetStream(const MediaStream& stream,
	const std::string& addr)
{
	if (stream.stream.stream_type == StreamType::AUDIO) {
		return OpenAudioStreamSrc(stream, addr);
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		return OpenVideoStreamSrc(stream, addr);
	}
	else {
		LOG_ERR("Invalid stream type:{}", stream.stream.stream_type);
		return ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::CloseNetStream(const MediaStream& stream)
{
	if (stream.stream.stream_type == StreamType::AUDIO) {
		return CloseAudioStreamSrc(stream);
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		return CloseVideoStreamSrc(stream);
	}
	else {
		LOG_ERR("Invalid stream type:{}", stream.stream.stream_type);
		return ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::OpenAudioStreamSrc(const MediaStream& stream,
	const std::string& addr)
{
	LOG_INF("Open audio stream:{}", addr);

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto processor : m_processors) {
		if (processor->SubType() == PPST_AUDIO_STREAM_SRC) {
			AudioStreamSrcProcessorSP cp =
				std::dynamic_pointer_cast<AudioStreamSrcProcessor>(processor);
			if (cp->GetMediaStream().stream.stream_id == stream.stream.stream_id) {
				LOG_ERR("Media stream is opened!");
				return ERR_CODE_FAILED;
			}
		}
	}

	AudioStreamSrcProcessorSP processor
		= std::make_shared<AudioStreamSrcProcessor>(this);
	if (ERR_CODE_OK != processor->Init(stream, addr)) {
		LOG_ERR("Init audio stream src processor failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != processor->Start()) {
		LOG_ERR("Start audio stream src processor failed!");
		return ERR_CODE_FAILED;
	}

	m_processors.push_back(processor);

	LOG_INF("Open audio stream src success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::CloseAudioStreamSrc(const MediaStream& stream)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_processors.begin();
	for (; iter != m_processors.end(); ++iter) {
		if ((*iter)->SubType() == PPST_AUDIO_STREAM_SRC) {
			AudioStreamSrcProcessorSP cp =
				std::dynamic_pointer_cast<AudioStreamSrcProcessor>(*iter);
			if (cp->GetMediaStream().stream.stream_id == stream.stream.stream_id) {
				break;
			}
		}
	}

	if (iter == m_processors.end()) {
		LOG_ERR("Cannot find audio strema src processor!");
		return ERR_CODE_FAILED;
	}

	// TODO: 检查依赖关系


	// 关闭摄像头并移除 processor
	(*iter)->Stop();
	m_processors.erase(iter);

	LOG_INF("Close audio stream src success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::OpenVideoStreamSrc(const MediaStream& stream,
	const std::string& addr)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto processor : m_processors) {
		if (processor->SubType() == PPST_VIDEO_STREAM_SRC) {
			VideoStreamSrcProcessorSP cp =
				std::dynamic_pointer_cast<VideoStreamSrcProcessor>(processor);
			if (cp->GetMediaStream().stream.stream_id == stream.stream.stream_id) {
				LOG_ERR("Media stream is opened!");
				return ERR_CODE_FAILED;
			}
		}
	}

	VideoStreamSrcProcessorSP processor
		= std::make_shared<VideoStreamSrcProcessor>(this);
	if (ERR_CODE_OK != processor->Init(stream, addr)) {
		LOG_ERR("Init video stream src processor failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != processor->Start()) {
		LOG_ERR("Start video stream src processor failed!");
		return ERR_CODE_FAILED;
	}

	m_processors.push_back(processor);

	LOG_INF("Open video stream src success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::CloseVideoStreamSrc(const MediaStream& stream)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_processors.begin();
	for (; iter != m_processors.end(); ++iter) {
		if ((*iter)->SubType() == PPST_VIDEO_STREAM_SRC) {
			VideoStreamSrcProcessorSP cp =
				std::dynamic_pointer_cast<VideoStreamSrcProcessor>(*iter);
			if (cp->GetMediaStream().stream.stream_id == stream.stream.stream_id) {
				break;
			}
		}
	}

	if (iter == m_processors.end()) {
		LOG_ERR("Cannot find video strema src processor!");
		return ERR_CODE_FAILED;
	}

	// TODO: 检查依赖关系


	// 关闭摄像头并移除 processor
	(*iter)->Stop();
	m_processors.erase(iter);

	LOG_INF("Close video stream src success, processsors:{}", m_processors.size());

	for (auto iter = m_streams.begin(); iter != m_streams.end(); ) {
		if (iter->second.stream.stream.stream.stream_id == stream.stream.stream_id) {
			iter = m_streams.erase(iter);
			LOG_INF("Remove remote video stream");
		}
		else {
			++iter;
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartPlayStream(const MediaStream& stream)
{
	if (stream.src.app_id != 0) { // remote stream
		return StartPlayEncodedStream(stream, false);
	}
	else {
		if (stream.src.src_type == MediaSrcType::FILE) {
			return StartPlayEncodedStream(stream, true);
		}
		else if (stream.src.src_type == MediaSrcType::MICROPHONE) {
			return StartPlayRawStream(stream);
		}
		else {
			LOG_ERR("Invalid media stream");
			return ERR_CODE_FAILED;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopPlayStream(const MediaStream& stream)
{
	if (stream.src.app_id != 0 || stream.src.src_type == MediaSrcType::FILE) {
		return StopPlayEncodedStream(stream);
	}
	else if (stream.src.app_id == 0 && stream.src.src_type == MediaSrcType::MICROPHONE) {
		return StopPlayRawStream(stream);
	}
	else {
		LOG_ERR("Invalid media stream");
		return ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ISrcPin* PipelineProcessorMgr::GetStreamSrcPin(const MediaStream& stream, 
	IPipelineProcessorSP processor, const com::ElementPin& pin)
{
	IPipeline* pipeline = processor->Pipeline();
	if (!pipeline) {
		LOG_ERR("Invalid pipeline!");
		return nullptr;
	}

	// 查找 element
	IElement* element = pipeline->GetElementByName(pin.ele_name);
	if (!element) {
		LOG_ERR("Cannot find element:{}", pin.ele_name);
		return nullptr;
	}

	// 查找 src pin
	for (auto src_pin : element->SrcPins()) {
		if (src_pin->StreamId() == stream.stream.stream_id) {
			return src_pin;
		}
	}

	LOG_INF("Get stream:{} src pin failed!", stream.stream.stream_id);

	return nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ISrcPin* PipelineProcessorMgr::GetStreamSrcPin(const std::string& stream_id)
{
	// 查找上报的媒体流
	auto iter = m_streams.find(stream_id);
	if (iter == m_streams.end()) {
		LOG_ERR("Cannot find stream:{}", stream_id);
		return nullptr;
	}

	// 查找媒体流所在的 pipeline
	if (!iter->second.processor) {
		LOG_ERR("Invalid processor!");
		return nullptr;
	}
	IPipeline* pipeline = iter->second.processor->Pipeline();
	if (!pipeline) {
		LOG_ERR("Invalid pipeline!");
		return nullptr;
	}

	// 查找 element
	IElement* element = pipeline->GetElementByName(iter->second.stream.pin.ele_name);
	if (!element) {
		LOG_ERR("Cannot find element:{}", iter->second.stream.pin.ele_name);
		return nullptr;
	}

	// 查找 src pin
	for (auto src_pin : element->SrcPins()) {
		if (src_pin->StreamId() == stream_id) {
			return src_pin;
		}
	}

	LOG_INF("Get stream:{} src pin failed!", stream_id);

	return nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ISinkPin* PipelineProcessorMgr::GetProcessorSinkPin(const com::ElementPin& pin)
{
	// 查找 pipeline
	IPipeline* pipeline = nullptr;
	for (auto iter = m_processors.begin(); iter != m_processors.end(); ++iter) {
		if ((*iter)->Pipeline()->Name() == pin.pl_name) {
			pipeline = (*iter)->Pipeline();
			break;
		}
	}
	if (!pipeline) {
		LOG_ERR("Invalid pipeline!");
		return nullptr;
	}

	// 查找 element
	IElement* element = pipeline->GetElementByName(pin.ele_name);
	if (!element) {
		LOG_ERR("Cannot find element:{}", pin.ele_name);
		return nullptr;
	}

	// 查找 sink pin
	for (auto sink_pin : element->SinkPins()) {
		if (sink_pin->Name() == pin.pin_name) {
			return sink_pin;
		}
	}

	LOG_INF("Get processor sink pin failed!");

	return nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::LinkProcessor(IPipelineProcessorSP src_processor,
	const ElementStream& stream, IPipelineProcessorSP sink_processor)
{
	// Pause src processor
	bool need_recover_src = false;
	ProcessorState src_state = src_processor->State();
	if (src_state == ProcessorState::RUNNING) {
		if (ERR_CODE_OK != src_processor->Pause()) {
			LOG_ERR("Pause src processor failed!");
			return ERR_CODE_FAILED;
		}
		need_recover_src = true;
	}

	// Pause sink processor
	bool need_recover_sink = false;
	ProcessorState sink_state = sink_processor->State();
	if (sink_state == ProcessorState::RUNNING) {
		if (ERR_CODE_OK != sink_processor->Pause()) {
			LOG_ERR("Pause sink processor failed!");
			return ERR_CODE_FAILED;
		}
		need_recover_sink = true;
	}

	// 获取媒体源 src pin
	ISrcPin* src_pin = GetStreamSrcPin(stream.stream.stream.stream_id);
	if (!src_pin) {
		LOG_ERR("Get stream src pin failed!");
		return ERR_CODE_FAILED;
	}

	// 获取处理器 sink pin
	com::ElementPin pin;
	if (!sink_processor->AllocSinkPin(stream.stream.stream.stream_id, pin)) {
		LOG_ERR("Alloc sink pin failed!");
		return ERR_CODE_FAILED;
	}
	ISinkPin* sink_pin = GetProcessorSinkPin(pin);
	if (!sink_pin) {
		LOG_ERR("Get processor sink pin failed!");
		return ERR_CODE_FAILED;
	}

	// 连接
	if (ERR_CODE_OK != src_pin->AddSinkPin(sink_pin)) {
		LOG_ERR("Add sink pin failed!");
		return ERR_CODE_FAILED;
	}

	// 恢复 sink processor 状态
	if (need_recover_sink) {
		if (ERR_CODE_OK != sink_processor->Resume()) {
			LOG_ERR("Resume sink processor failed!");
			return ERR_CODE_FAILED;
		}
	}

	// 恢复 src processor 状态
	if (need_recover_src) {
		if (ERR_CODE_OK != src_processor->Resume()) {
			LOG_ERR("Resume src processor failed!");
			return ERR_CODE_FAILED;
		}
	}
	
	DepEntry entry;
	entry.src_pin.pl_name = stream.pin.pl_name;
	entry.src_pin.ele_name = stream.pin.ele_name;
	entry.src_pin.pin_name = stream.pin.pin_name;
	entry.src_processor = src_processor;
	entry.sink_pin.pl_name = sink_pin->Element()->Pipeline()->Name();
	entry.sink_pin.ele_name = sink_pin->Element()->Name();
	entry.sink_pin.pin_name = sink_pin->Name();
	entry.sink_processor = sink_processor;
	entry.stream = stream.stream;

	m_deps.push_back(entry);

	LOG_INF("Add dependency entry success, deps:{}", m_deps.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::UnlinkProcessor(const MediaStream& stream,
	IPipelineProcessorSP processor)
{
	// 查找依赖
	auto iter = m_deps.end();
	for (iter = m_deps.begin(); iter != m_deps.end(); iter++) {
		if (iter->stream.stream.stream_id == stream.stream.stream_id
			&& iter->sink_processor == processor) {
			break;
		}
	}
	if (iter == m_deps.end()) {
		LOG_ERR("Cannot find dependency!");
		return ERR_CODE_FAILED;
	}

	// Pause src processor
	bool need_recover_src = false;
	ProcessorState src_state = iter->src_processor->State();
	if (src_state == ProcessorState::RUNNING) {
		if (ERR_CODE_OK != iter->src_processor->Pause()) {
			LOG_ERR("Pause src processor failed!");
			return ERR_CODE_FAILED;
		}
		need_recover_src = true;
	}

	// Pause sink processor
	bool need_recover_sink = false;
	ProcessorState sink_state = iter->sink_processor->State();
	if (sink_state == ProcessorState::RUNNING) {
		if (ERR_CODE_OK != iter->sink_processor->Pause()) {
			LOG_ERR("Pause sink processor failed!");
			return ERR_CODE_FAILED;
		}
		need_recover_sink = true;
	}

	// 获取媒体源 src pin
	ISrcPin* src_pin = GetStreamSrcPin(stream, iter->src_processor, iter->src_pin);
	if (!src_pin) {
		LOG_ERR("Get stream src pin failed!");
		return ERR_CODE_FAILED;
	}

	// 获取处理器 sink pin
	ISinkPin* sink_pin = GetProcessorSinkPin(iter->sink_pin);
	if (!sink_pin) {
		LOG_ERR("Get processor sink pin failed!");
		return ERR_CODE_FAILED;
	}

	// 移除连接
	if (ERR_CODE_OK != src_pin->RemoveSinkPin(sink_pin->Name())) {
		LOG_ERR("Remove sink pin failed!");
		return ERR_CODE_FAILED;
	}

	// 恢复 sink processor 状态
	if (need_recover_sink) {
		if (ERR_CODE_OK != iter->sink_processor->Resume()) {
			LOG_ERR("Resume sink processor failed!");
			return ERR_CODE_FAILED;
		}
	}

	// 恢复 src processor 状态
	if (need_recover_src) {
		if (ERR_CODE_OK != iter->src_processor->Resume()) {
			LOG_ERR("Resume src processor failed!");
			return ERR_CODE_FAILED;
		}
	}
	
	m_deps.erase(iter);

	LOG_INF("Remove dependency success, deps:{}", m_deps.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 音频播放，
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartPlayEncodedStream(const MediaStream& stream,
	bool sync)
{
	return StartProcessorTemplate(stream, 
		[this, stream, sync]() -> IPipelineProcessorSP {
			// 创建 processor
			EncodedAudioPlayProcessorSP processor
				= std::make_shared<EncodedAudioPlayProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream, sync)) {
				LOG_ERR("Init encoded audio play processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start encoded audio play processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartProcessorTemplate(const MediaStream& stream,
	ProcessorBuilder builder)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// 查找媒体源是否存在
	auto iter = m_streams.find(stream.stream.stream_id);
	if (iter == m_streams.end()) {
		LOG_ERR("Cannot find stream:{}", stream.stream.stream_id);
		return ERR_CODE_FAILED;
	}

	// 创建 processor
	IPipelineProcessorSP processor = builder();
	if (!processor) {
		LOG_ERR("Build processor failed!");
		return ERR_CODE_FAILED;
	}

	// 保存 processor（必须先保存再连接，连接时会使用 processor）
	m_processors.push_back(processor);

	// 连接媒体源
	if (ERR_CODE_OK != LinkProcessor(iter->second.processor, iter->second.stream,
		processor)) {
		m_processors.pop_back();
		LOG_ERR("Link processor failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Start processor success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopProcessor(const MediaStream& stream, 
	PipelineProcessorSubType pt)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// 查找 processor
	auto iter = m_processors.end();
	for (iter = m_processors.begin(); iter != m_processors.end(); ++iter) {
		if ((*iter)->SubType() == pt) {
			break;
		}
	}
	if (iter == m_processors.end()) {
		LOG_ERR("Cannot find processor!");
		return ERR_CODE_FAILED;
	}

	// 移除连接
	if (ERR_CODE_OK != UnlinkProcessor(stream, *iter)) {
		LOG_ERR("Unlink processor failed!");
		return ERR_CODE_FAILED;
	}

	(*iter)->Stop();
	m_processors.erase(iter);

	LOG_INF("Remove processor success, processors:{}", m_processors.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopPlayEncodedStream(const MediaStream& stream)
{
	return StopProcessor(stream, PPST_AUDIO_ENCODED_PLAYER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartPlayRawStream(const MediaStream& stream)
{
	return StartProcessorTemplate(stream, 
		[this, stream]() -> IPipelineProcessorSP {
			// 创建 processor
			RawAudioPlayProcessorSP processor
				= std::make_shared<RawAudioPlayProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream)) {
				LOG_ERR("Init raw audio play processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start raw audio play processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopPlayRawStream(const MediaStream& stream)
{
	return StopProcessor(stream, PPST_AUDIO_RAW_PLAYER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartRenderStream(const MediaStream& stream,
	void* wnd)
{
	if (stream.src.app_id != 0 || stream.src.src_type == MediaSrcType::FILE) {
		return StartRenderEncodedStream(stream, wnd);
	}
	else if (stream.src.app_id == 0 && stream.src.src_type == MediaSrcType::CAMERA) {
		return StartRenderRawStream(stream, wnd);
	}
	else {
		LOG_ERR("Invalid media stream");
		return ERR_CODE_FAILED;
	}	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartRenderEncodedStream(const MediaStream& stream,
	void* wnd)
{
	return StartProcessorTemplate(stream, 
		[this, stream, wnd]() -> IPipelineProcessorSP {
			// 创建 processor
			EncodedVideoRenderProcessorSP processor
				= std::make_shared<EncodedVideoRenderProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream, wnd)) {
				LOG_ERR("Init encoded video render processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start encoded video render processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartRenderRawStream(const MediaStream& stream,
	void* wnd)
{
	return StartProcessorTemplate(stream, 
		[this, stream, wnd]() -> IPipelineProcessorSP {
			// 创建 processor
			RawVideoRenderProcessorSP processor
				= std::make_shared<RawVideoRenderProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream, wnd)) {
				LOG_ERR("Init raw video render processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start raw video render processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopRenderStream(const MediaStream& stream,
	void* wnd)
{
	if (stream.src.app_id != 0 || stream.src.src_type == MediaSrcType::FILE) {
		return StopRenderEncodedStream(stream, wnd);
	}
	else if (stream.src.app_id == 0 && stream.src.src_type == MediaSrcType::CAMERA) {
		return StopRenderRawStream(stream, wnd);
	}
	else {
		LOG_ERR("Invalid media stream");
		return ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopRenderEncodedStream(const MediaStream& stream,
	void* wnd)
{
	// TODO: wnd
	return StopProcessor(stream, PPST_VIDEO_ENCODED_RENDER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopRenderRawStream(const MediaStream& stream,
	void* wnd)
{
	// TODO: wnd
	return StopProcessor(stream, PPST_VIDEO_RAW_RENDER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartSendStream(const MediaStream& stream,
	const std::string& addr)
{
	if (stream.stream.stream_type == StreamType::AUDIO) {
		if (stream.src.src_type == MediaSrcType::FILE) {
			return StartSendEncodedAudioStream(stream, addr);
		}
		else {
			return StartSendRawAudioStream(stream, addr);
		}
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		if (stream.src.src_type == MediaSrcType::FILE) {
			return StartSendEncodedVideoStream(stream, addr);
		}
		else {
			return StartSendRawVideoStream(stream, addr);
		}
	}
	else {
		LOG_ERR("Invalid media stream");
		return ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartSendRawAudioStream(const MediaStream& stream,
	const std::string& addr)
{
	return StartProcessorTemplate(stream, 
		[this, stream, addr]() -> IPipelineProcessorSP {
			// 创建 processor
			RawAudioSendProcessorSP processor
				= std::make_shared<RawAudioSendProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream, addr)) {
				LOG_ERR("Init raw audio send processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start raw audio send processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartSendEncodedAudioStream(const MediaStream& stream,
	const std::string& addr)
{
	return StartProcessorTemplate(stream,
		[this, stream, addr]() -> IPipelineProcessorSP {
			// 创建 processor
			EncodedAudioSendProcessorSP processor
				= std::make_shared<EncodedAudioSendProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream, addr)) {
				LOG_ERR("Init encoded audio send processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start encoded audio send processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartSendRawVideoStream(const MediaStream& stream,
	const std::string& addr)
{
	return StartProcessorTemplate(stream, 
		[this, stream, addr]() -> IPipelineProcessorSP {
			// 创建 processor
			RawVideoSendProcessorSP processor
				= std::make_shared<RawVideoSendProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream, addr)) {
				LOG_ERR("Init raw video send processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start raw video send processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartSendEncodedVideoStream(const MediaStream& stream,
	const std::string& addr)
{
	return StartProcessorTemplate(stream,
		[this, stream, addr]() -> IPipelineProcessorSP {
			// 创建 processor
			EncodedVideoSendProcessorSP processor
				= std::make_shared<EncodedVideoSendProcessor>(this);
			if (ERR_CODE_OK != processor->Init(stream, addr)) {
				LOG_ERR("Init encoded video send processor failed!");
				return nullptr;
			}

			// 启动 processor
			if (ERR_CODE_OK != processor->Start()) {
				LOG_ERR("Start encoded video send processor failed!");
				return nullptr;
			}

			return processor;
		});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopSendStream(const MediaStream& stream)
{
	if (stream.stream.stream_type == StreamType::AUDIO) {
		if (stream.src.src_type == MediaSrcType::FILE) {
			return StopSendEncodedAudioStream(stream);
		}
		else {
			return StopSendRawAudioStream(stream);
		}
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		if (stream.src.src_type == MediaSrcType::FILE) {
			return StopSendEncodedVideoStream(stream);
		}
		else {
			return StopSendRawVideoStream(stream);
		}
	}
	else {
		LOG_ERR("Invalid media stream");
		return ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopSendRawAudioStream(const MediaStream& stream)
{
	return StopProcessor(stream, PPST_AUDIO_RAW_SENDER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopSendEncodedAudioStream(const MediaStream& stream)
{
	return StopProcessor(stream, PPST_AUDIO_ENCODED_SENDER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopSendRawVideoStream(const MediaStream& stream)
{
	return StopProcessor(stream, PPST_VIDEO_RAW_SENDER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopSendEncodedVideoStream(const MediaStream& stream)
{
	return StopProcessor(stream, PPST_VIDEO_ENCODED_SENDER);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StartAudioTest(const MediaStream& stream)
{
	return ERR_CODE_FAILED;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorMgr::StopAudioTest(const MediaStream& stream)
{
	return ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorMgr::OnAddStream(const ElementStream& stream,
	IPipelineProcessorSP processor)
{
	auto iter = m_streams.find(stream.stream.stream.stream_id);
	if (iter != m_streams.end()) {
		LOG_ERR("stream:{} already exists", stream.stream.stream.stream_id);
		return;
	}

	StreamEntry entry;
	entry.stream = stream;
	entry.processor = processor;

	m_streams.insert(std::make_pair(stream.stream.stream.stream_id, entry));

	// Notify 
	CommonMsg msg(ENGINE_MSG_ADD_MEDIA_STREAM);
	msg.msg_data.reset(new MediaStream(stream.stream));
	m_engine->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorMgr::OnRemoveStream(const com::ElementStream& stream,
	IPipelineProcessorSP processor)
{
	// FIXME: remove stream from m_streams

	CommonMsg msg(ENGINE_MSG_REMOVE_MEDIA_STREAM);
	msg.msg_data.reset(new MediaStream(stream.stream));
	m_engine->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorMgr::OnRunState(const std::string& desc)
{
	CommonMsg msg(ENGINE_MSG_RUN_STATE);
	msg.msg_data.reset(new std::string(desc));
	m_engine->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorMgr::OnPlayProgress(const com::MediaSrc& media_src,
	uint32_t progress)
{
	CommonMsg msg(ENGINE_MSG_PLAY_PROGRESS);
	msg.msg_data.reset(new stmr::PlayProgressData(media_src, progress));
	m_engine->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorMgr::OnAudioStreamStats(const com::MediaStream& stream,
	const com::AudioStreamStats& stats)
{
	CommonMsg msg(ENGINE_MSG_AUDIO_STREAM_STATS);
	msg.msg_data.reset(new stmr::AudioStreamStatsData(stream, stats));
	m_engine->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorMgr::OnVideoStreamStats(const com::MediaStream& stream,
	const com::VideoStreamStats& stats)
{
	CommonMsg msg(ENGINE_MSG_VIDEO_STREAM_STATS);
	msg.msg_data.reset(new stmr::VideoStreamStatsData(stream, stats));
	m_engine->PostMsg(msg);
}