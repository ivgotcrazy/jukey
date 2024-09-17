#include "camera-element.h"
#include "util-streamer.h"
#include "util-enum.h"
#include "pipeline-msg.h"
#include "util-mf.h"
#include "util-streamer.h"
#include "common/util-time.h"
#include "common/util-common.h"
#include "log.h"
#include "common/media-common-define.h"
#include "streamer/streamer-common.h"
#include "yaml-cpp/yaml.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmsdkidl.h>

#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"Mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"evr.lib")
#pragma comment(lib,"shlwapi.lib")


using namespace jukey::com;
using namespace jukey::media::util;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CameraElement::CameraElement(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_CAMERA, owner)
	, ElementBase(factory)
	, CommonThread("Camera element", true)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("camera-");
	m_main_type  = EleMainType::SRC;
	m_sub_type   = EleSubType::CAPTURER;
	m_media_type = EleMediaType::VIDEO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CameraElement::~CameraElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* CameraElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_CAMERA) == 0) {
		return new CameraElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* CameraElement::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_ELEMENT)) {
		return static_cast<IElement*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::NotifyAddVideoStream()
{
	m_stream_id = util::GenerateGUID();

	if (ERR_CODE_OK != SRC_PIN->OnPinMsg(nullptr,
		PinMsg(PinMsgType::SET_STREAM, m_stream_id))) {
		LOG_ERR("Set src pin:{} stream failed!", SRC_PIN->Name());
		return ERR_CODE_FAILED;
	}

	ElementStream* es = new ElementStream();
	es->stream.src.src_type = MediaSrcType::CAMERA;
	es->stream.src.src_id = std::to_string(m_device_id);
	es->stream.stream.stream_type = StreamType::VIDEO;
	es->stream.stream.stream_id = m_stream_id;
	es->pin.ele_name = m_ele_name;
	es->pin.pin_name = m_src_pins[0]->Name();
	es->cap = m_src_pins[0]->Cap();

	com::CommonMsg msg;
	msg.msg_type = (uint32_t)PlMsgType::ADD_ELEMENT_STREAM;
	msg.src = m_ele_name;
	msg.msg_data.reset(es);
	m_pipeline->PostPlMsg(msg);

	LOG_INF("Notify add video stream, stream:{}", m_stream_id);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::NotifyDelVideoStream()
{
	//if (m_stream_id.empty()) {
	//	LOG_INF("Stream has not been added");
	//	return ERR_CODE_OK;
	//}

	//MediaSrc src(MediaSrcType::CAMERA, std::to_string(m_device_id));

	//com::CommonMsg msg;
	//msg.msg_type = (uint32_t)PlMsgType::DEL_ELEMENT_STREAM;
	//msg.src = m_ele_name;
	//msg.msg_data.reset(new EleStreamData(media::MediaType::VIDEO,
	//	m_stream_id, m_ele_name, SRC_PIN->Name(), src.ToStr()));

	//m_pipeline->PostPlMsg(msg);

	//LOG_INF("Notify remove video stream, stream:{}", m_stream_id);

	//m_stream_id.clear();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::CreateSrcPin()
{
	media::com::VideoCaps vcaps;
	vcaps.AddCap(media::VideoCodec::RAW);
	vcaps.AddCap(media::PixelFormat::I420);
	vcaps.AddCap(media::PixelFormat::YUY2);
	vcaps.AddCap(media::PixelFormat::NV12);
	vcaps.AddCap(media::PixelFormat::RGB24);
	vcaps.AddCap(media::VideoRes::RES_1920x1080);
	vcaps.AddCap(media::VideoRes::RES_1280x720);
	vcaps.AddCap(media::VideoRes::RES_640x480);
	vcaps.AddCap(media::VideoRes::RES_640x360);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-src-pin-"),
			media::util::ToVideoCapsStr(vcaps),
			this)) {
		LOG_ERR("Init src pin failed!");
		return ERR_CODE_FAILED;
	}

	m_src_pin_index = static_cast<uint32_t>(m_src_pins.size());
	m_src_pins.push_back(src_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CameraElement::ParseProperties(com::IProperty* props)
{
	if (!props) {
		LOG_ERR("Invalid props!");
		return false;
	}

	uint32_t* device = props->GetU32Value("device-id");
	if (!device) {
		LOG_ERR("Cannot find device-id property!");
		return false;
	}
	else {
		m_device_id = *device;
		LOG_INF("Read device-id:{}", m_device_id);
	}
	

	uint32_t* resolution = props->GetU32Value("resolution");
	if (!resolution) {
		LOG_ERR("Cannot find 'resolution' property!");
		return false;
	}
	else {
		m_src_pin_cap.res = (media::VideoRes)(*resolution);
		LOG_INF("Read resolution:{}", media::util::VIDEO_RES_STR(m_src_pin_cap.res));
	}
	

	uint32_t* pixel_format = props->GetU32Value("pixel-format");
	if (!pixel_format) {
		LOG_ERR("Cannot find 'device-pixel-format' property!");
		return false;
	}
	else {
		m_src_pin_cap.format = (media::PixelFormat)(*pixel_format);
		LOG_INF("Read format:{}", media::util::VIDEO_FMT_STR(m_src_pin_cap.format));
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::EnumerateVideoDevice()
{
	IMFAttributes* pAttributes = nullptr;
	HRESULT hr = MFCreateAttributes(&pAttributes, 1);
	if (FAILED(hr)) {
		LOG_ERR("MFCreateAttributes failed!");
		return ERR_CODE_FAILED;
	}

	hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr)) {
		LOG_ERR("SetGUID failed!");
		return ERR_CODE_FAILED;
	}

	IMFActivate** ppDevices;
	UINT32 count = 0;

	hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
	if (FAILED(hr)) {
		LOG_ERR("MFEnumDeviceSources failed!");
		return ERR_CODE_FAILED;
	}

	if (m_device_id + 1 > count) {
		LOG_ERR("Unexpected device count:{}, device id:{}", count, m_device_id);
		return ERR_CODE_FAILED;
	}

	m_video_device = ppDevices[m_device_id];

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::InitMediaFoundation()
{
	HRESULT hr = S_OK;

	// TODO: where to call initialize?
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) {
		LOG_ERR("CoInitializeEx failed!");
		return ERR_CODE_FAILED;
	}

	hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	if (FAILED(hr)) {
		LOG_ERR("MFStartup failed!");
		return ERR_CODE_FAILED;
	}

	m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_event == NULL) {
		LOG_ERR("MFEnumDeviceSources failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != CreateMediaSource()) {
		LOG_ERR("Create media source failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CameraElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["camera-element"];
		if (!node) {
			return;
		}

		if (node["dump-camera-data"]) {
			if (node["dump-camera-data"].as<bool>()) {
				m_cam_dumper.reset(new StreamDumper(m_ele_name + ".data"));
			}
		}
	}
	catch (const std::exception& e) {
		LOG_WRN("Error:{}", e.what());
	}
}

//------------------------------------------------------------------------------
// TODO: 资源释放
//------------------------------------------------------------------------------
ErrCode CameraElement::DoInit(com::IProperty* props)
{
	LOG_INF("DoInit, props:{}", props->Dump());

	m_logger = g_logger;

	if (!ParseProperties(props)) {
		LOG_ERR("Parse properties failed!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (ERR_CODE_OK != EnumerateVideoDevice()) {
		LOG_ERR("Enumerate video device failed!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (!UpdateAvaiCaps()) {
		LOG_ERR("Update capabilities failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != InitMediaFoundation()) {
		LOG_ERR("Init media foundation failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != NotifyAddVideoStream()) {
		LOG_ERR("Notify video stream failed!");
		return ERR_CODE_FAILED;
	}

	ParseElementConfig();

	m_pipeline->SubscribeMsg(PlMsgType::NEGOTIATE, this);

	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::CreateSourceReader()
{
	if (m_source_reader) {
		LOG_INF("Source reader exists");
		return ERR_CODE_OK; // TODO:
	}

	IMFAttributes* attrs = NULL;
	HRESULT result = S_OK;

	if (!m_sync_mode) {
		result = MFCreateAttributes(&attrs, 1);
		if (FAILED(result)) {
			LOG_ERR("MFEnumDeviceSources failed!");
			return ERR_CODE_FAILED;
		}

		m_async_reader = new MFAsyncReader(SRC_PIN, m_event);

		result = attrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, m_async_reader);
		if (FAILED(result)) {
			LOG_ERR("MFEnumDeviceSources failed!");
			return ERR_CODE_FAILED;
		}
	}
	
	result = MFCreateSourceReaderFromMediaSource(m_media_source, 
		attrs, &m_source_reader);
	if (FAILED(result)) {
		LOG_ERR("MFCreateSourceReaderFromMediaSource failed!");
		return ERR_CODE_FAILED;
	}

	IMFMediaType* media_type = nullptr;
	result = MFCreateMediaType(&media_type);
	if (FAILED(result)) {
		LOG_ERR("MFCreateMediaType failed!");
		return ERR_CODE_FAILED;
	}
	media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);

	GUID sub_type;
	if (!media::util::ToMfSubType(m_src_pin_cap.format, sub_type)) {
		LOG_ERR("Get media foundation sub type failed!");
		return ERR_CODE_FAILED;
	}
	media_type->SetGUID(MF_MT_SUBTYPE, sub_type);

  LOG_INF("Set subtype, pixel format:{}", 
		media::util::VIDEO_FMT_STR(m_src_pin_cap.format));

	result = MFSetAttributeSize(media_type, MF_MT_FRAME_SIZE, 
		media::util::GetWidth(m_src_pin_cap.res),
		media::util::GetHeight(m_src_pin_cap.res));
	if (FAILED(result)) {
		LOG_ERR("MFSetAttributeSize failed!");
		return ERR_CODE_FAILED;
	}

  LOG_INF("Set frame size, width:{}, height:{}", 
    media::util::GetWidth(m_src_pin_cap.res),
		media::util::GetHeight(m_src_pin_cap.res));

	result = m_source_reader->SetCurrentMediaType(
		MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, media_type);
	if (FAILED(result)) {
		LOG_ERR("SetCurrentMediaType failed, result:{}, error:{}", result,
			GetLastError());
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::DestroySourceReader()
{
	LOG_INF("Destroy source reader");

	std::lock_guard<std::mutex> lock(m_mutex);

	// TODO:
	m_source_reader->Release();
	m_source_reader = nullptr;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::CreateMediaSource()
{
	HRESULT hr = m_video_device->ActivateObject(IID_PPV_ARGS(&m_media_source));
	if (FAILED(hr)) {
		LOG_ERR("MFEnumDeviceSources failed!");
		return ERR_CODE_FAILED;
	}
	m_media_source->AddRef();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CameraElement::UpdateAvaiCaps()
{
	media::com::VideoCap vcap;
	vcap.codec = media::VideoCodec::RAW;
	vcap.res = m_src_pin_cap.res;
	vcap.format = m_src_pin_cap.format;

	PinCaps avai_caps;
	avai_caps.push_back(media::util::ToVideoCapStr(vcap));

	LOG_INF("Update available caps:");

	if (ERR_CODE_OK != SRC_PIN->UpdateAvaiCaps(avai_caps)) {
		LOG_ERR("Update src pin available caps failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::DoStart()
{
	LOG_INF("DoStart");

	if (ERR_CODE_OK != CreateSourceReader()) {
		LOG_ERR("Create source reader failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::DoPause()
{
	LOG_INF("DoPause");

	DestroySourceReader();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::DoResume()
{
	LOG_INF("DoResume");

	if (ERR_CODE_OK != CreateSourceReader()) {
		LOG_ERR("Create source reader failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::DoStop()
{
	LOG_INF("DoStop");

	// FIXEE: Object may released before notify
	//NotifyDelVideoStream();

	StopThread();

	DestroySourceReader();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CameraElement::ProcSampleSync()
{
	DWORD stream_index, flags;
	LONGLONG timestamp;
	IMFSample* sample = NULL;

	static uint64_t time_start = util::Now();

	//std::lock_guard<std::recursive_mutex> lock(m_mutex);

	m_mutex.lock();

	if (!m_source_reader) return;

	HRESULT hr = m_source_reader->ReadSample(
		MF_SOURCE_READER_ANY_STREAM, // Stream index.
		0,                           // Flags.
		&stream_index,               // Receives the actual stream index. 
		&flags,                      // Receives status flags.
		&timestamp,                  // Receives the time stamp.
		&sample                      // Receives the sample or NULL.
	);
	if (FAILED(hr)) {
		LOG_ERR("Read sample failed, hr:{}, error:{}", hr, GetLastError());
		m_mutex.unlock();
		return;
	}

	if (!sample) {
		LOG_ERR("Invalid sample");
		m_mutex.unlock();
		return;
	}

	IMFMediaBuffer* media_buf = nullptr;
	sample->GetBufferByIndex(0, &media_buf);

	BYTE* buf;
	DWORD max_len, cur_len;
	if (FAILED(media_buf->Lock(&buf, &max_len, &cur_len))) {
		LOG_ERR("Lock buffer failed!");
		m_mutex.unlock();
		return;
	}

	// Copy frame data
	PinData pin_data(media::MediaType::VIDEO, buf, cur_len);

	media_buf->Unlock();
	media_buf->Release();
	sample->Release();

	m_mutex.unlock();

	pin_data.dts = util::Now() - time_start;
	pin_data.pts = pin_data.dts;
	pin_data.drt = 0;
	pin_data.pos = 0;
	pin_data.syn = 0;
	pin_data.tbn = 1;
	pin_data.tbd = 1000;
	pin_data.data_count = 1;

	auto para = SPC<media::VideoFramePara>(pin_data.media_para);
	para->codec  = m_src_pin_cap.codec;
	para->key    = 0;
	para->width  = media::util::GetWidth(m_src_pin_cap.res);
	para->height = media::util::GetHeight(m_src_pin_cap.res);
	para->ts     = static_cast<uint32_t>(pin_data.dts);
	para->seq    = ++m_frame_seq;

	if (m_ele_state == EleState::RUNNING) {
		SRC_PIN->OnPinData(pin_data);
		if (m_cam_dumper) {
			m_cam_dumper->WriteStreamData(pin_data);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CameraElement::ProcSampleAsync()
{
	HRESULT hr = m_source_reader->ReadSample(
		MF_SOURCE_READER_FIRST_VIDEO_STREAM,
		0,
		NULL,
		NULL,
		NULL,
		NULL);
	if (SUCCEEDED(hr)) {
		DWORD dwResult = WaitForSingleObject(m_event, INFINITE);
		if (dwResult == WAIT_TIMEOUT)
		{
			LOG_ERR("Timeout");
		}
		else if (dwResult != WAIT_OBJECT_0)
		{
			LOG_ERR("Error {}", GetLastError());
		}
	}
	else {
		LOG_ERR("Read sample failed, error:{}", GetLastError());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CameraElement::ProcFrameRateStats()
{
	static uint64_t frame_count = 0;
	static uint64_t last_stats_time = util::Now();

	++frame_count;

	uint64_t now = util::Now();

	if (now - last_stats_time >= 1000) {
		LOG_DBG("Capture frame count:{}", frame_count);
		last_stats_time = now;
		frame_count = 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CameraElement::ThreadProc()
{
	uint64_t read_time = 0;
	uint64_t over_time = 0;

	while (!m_stop) {
		if (m_ele_state != EleState::RUNNING || !m_source_reader) {
			util::Sleep(10);
			continue;
		}

		uint64_t now = util::Now();

		// Frame rate my change
		uint32_t read_interval = 1000 / m_frame_rate;

		if (read_time == 0) { // first time
			read_time = now;
			over_time = read_time + read_interval;
		}
		else {
			if (now < over_time) { // sleep a while
				util::Sleep(over_time - now);
			}
			else {
				read_time = now;
				over_time = read_time + read_interval;
			}
		}

		m_sync_mode ? ProcSampleSync() : ProcSampleAsync();

		ProcFrameRateStats();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::OnPipelineNegotiateMsg(const com::CommonMsg& msg)
{
	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Negotiate failed!");
		if (msg.result) {
			msg.result->set_value(ERR_CODE_FAILED);
		}
	}
	else {
		LOG_INF("Negotiate success");
		if (msg.result) {
			msg.result->set_value(ERR_CODE_OK);
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::PreProcPipelineMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case PlMsgType::NEGOTIATE:
		return OnPipelineNegotiateMsg(msg);
	default:
		return ERR_CODE_MSG_NO_PROC;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode CameraElement::OnSrcPinNegotiated(ISrcPin* src_pin, const std::string& cap)
{
	LOG_INF("OnSrcPinNegotiated, pin:{}, cap:{}", src_pin->Name(), cap);

	return ErrCode::ERR_CODE_OK;
}

}