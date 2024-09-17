#include "device-manager.h"
#include "log.h"
#include "util-mf.h"
#include "common-struct.h"

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>  // 导入包含设备属性键的头文件

using namespace jukey::util;
using namespace jukey::com;

namespace jukey::dev
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DeviceManager::DeviceManager(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_DEV_MGR, owner)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* DeviceManager::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_DEV_MGR) == 0) {
		return new DeviceManager(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* DeviceManager::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_DEV_MGR)) {
		return static_cast<IDevMgr*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode DeviceManager::EnumAudioInputDevices()
{
	IMFAttributes* pAttributes = nullptr;
	HRESULT hr = MFCreateAttributes(&pAttributes, 1);
	if (FAILED(hr)) {
		LOG_ERR("MFCreateAttributes failed!");
		return ERR_CODE_FAILED;
	}

	hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
	if (FAILED(hr)) {
		LOG_ERR("SetGUID failed!");
		return ERR_CODE_FAILED;
	}

	IMFActivate** ppDevices;
	UINT32 count;

	hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
	if (FAILED(hr)) {
		LOG_ERR("MFEnumDeviceSources failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Enum audio device count {}", count);

	USES_CONVERSION;
	for (UINT32 i = 0; i < count; i++) {

		WCHAR* szFriendlyName = NULL;
		UINT32 cchName = 0;

		// Try to get the display name.
		hr = ppDevices[i]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
			&szFriendlyName,
			&cchName);
		if (FAILED(hr)) {
			LOG_ERR("Cannot get device:{} name!", i);
			continue;
		}

		MediaDevice dev;
		dev.dev_id = i;
		dev.dev_name = CW2A(szFriendlyName, CP_UTF8);
		dev.dev_ptr = ppDevices[i];

		m_audio_input_dev_list.push_back(dev);

		LOG_INF("Add audio input device:{}", dev.dev_name);
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode DeviceManager::EnumVideoInputDevices()
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
	UINT32 count;

	hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
	if (FAILED(hr)) {
		LOG_ERR("MFEnumDeviceSources failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Enum video device count {}", count);

	USES_CONVERSION;
	for (UINT32 i = 0; i < count; i++) {

		WCHAR* szFriendlyName = NULL;
		UINT32 cchName = 0;

		// Try to get the display name.
		hr = ppDevices[i]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
			&szFriendlyName,
			&cchName);
		if (FAILED(hr)) {
			LOG_ERR("Cannot get device:{} name!", i);
			continue;
		}

		MediaDevice dev;
		dev.dev_id = i;
		dev.dev_name = CW2A(szFriendlyName, CP_UTF8);
		dev.dev_ptr = ppDevices[i];

		m_video_input_dev_list.push_back(dev);

		LOG_INF("Add video input device:{}", dev.dev_name);
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode DeviceManager::EnumAudioOutputDevices()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr)) {
		IMMDeviceEnumerator* pEnumerator = NULL;
		IMMDeviceCollection* pCollection = NULL;
		IMMDevice* pDevice = NULL;

		// 创建设备枚举器
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator), (LPVOID*)&pEnumerator);
		if (SUCCEEDED(hr)) {
			// 获取所有音频渲染设备（扬声器）
			hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
			if (SUCCEEDED(hr)) {
				UINT count = 0;
				hr = pCollection->GetCount(&count);
				if (SUCCEEDED(hr)) {
					USES_CONVERSION;
					for (UINT i = 0; i < count; i++) {
						hr = pCollection->Item(i, &pDevice);
						if (SUCCEEDED(hr)) {
							IPropertyStore* pPropertyStore;
							hr = pDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
							if (SUCCEEDED(hr)) {
								PROPVARIANT varName;
								PropVariantInit(&varName);
								// 获取设备友好名称
								hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &varName);
								if (SUCCEEDED(hr)) {
									MediaDevice dev;
									dev.dev_id = i;
									dev.dev_name = CW2A(varName.pwszVal, CP_UTF8);
									dev.dev_ptr = nullptr;
									m_audio_output_dev_list.push_back(dev);

									CoTaskMemFree(varName.pwszVal);
									varName.pwszVal = NULL;
								}
								PropVariantClear(&varName);
								pPropertyStore->Release();
							}

							pDevice->Release();
						}
					}
				}
				pCollection->Release();
			}
			pEnumerator->Release();
		}
		CoUninitialize();
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode DeviceManager::Init(IDevEventHandler* handler)
{
#ifdef _WINDOWS
	HRESULT hr = S_OK;
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

	if (ERR_CODE_OK != EnumVideoInputDevices()) {
		LOG_ERR("Enumerate video input devices failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != EnumAudioInputDevices()) {
		LOG_ERR("Enumerate audio input devices failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != EnumAudioOutputDevices()) {
		LOG_ERR("Enumerate audio output devices failed!");
		return ERR_CODE_FAILED;
	}
#endif

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<AudioInputDev> DeviceManager::GetAudioInputDevList()
{
	std::vector<AudioInputDev> dev_list;

	for (const auto& item : m_audio_input_dev_list) {
		if (!item.dev_ptr) {
			LOG_ERR("Invalid device object");
			break;
		}

		IMFMediaSource* media_source = nullptr;
		HRESULT hr = ((IMFActivate*)item.dev_ptr)->ActivateObject(IID_PPV_ARGS(&media_source));
		if (FAILED(hr)) {
			LOG_ERR("Get media source failed!");
			break;
		}

		AudioInputDev dev;
		dev.dev_id = item.dev_id;
		dev.dev_name = item.dev_name;
		dev.dev_desc = "unknown";

		media::util::EnumAudioInputDevAttr(media_source, dev.attr);

		dev_list.push_back(dev);
	}

	return dev_list;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<VideoInputDev> DeviceManager::GetVideoInputDevList()
{
	std::vector<VideoInputDev> dev_list;

	for (const auto& item : m_video_input_dev_list) {
		if (!item.dev_ptr) {
			LOG_ERR("Invalid device object");
			break;
		}

		IMFMediaSource* media_source = nullptr;
		HRESULT hr = ((IMFActivate*)item.dev_ptr)->ActivateObject(IID_PPV_ARGS(&media_source));
		if (FAILED(hr)) {
			LOG_ERR("Get media source failed!");
			break;
		}

		VideoInputDev dev;
		dev.dev_id = item.dev_id;
		dev.dev_name = item.dev_name;
		dev.dev_desc = "unknown";

		media::util::EnumVideoInputDevAttr(media_source, dev.attr);

		dev_list.push_back(dev);
	}

	return dev_list;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<AudioOutputDev> DeviceManager::GetAudioOutputDevList()
{
	std::vector<AudioOutputDev> dev_list;

	for (const auto& item : m_audio_output_dev_list) {
		AudioOutputDev dev;
		dev.dev_id = item.dev_id;
		dev.dev_name = item.dev_name;
		dev.dev_desc = "unknown";

		dev_list.push_back(dev);
	}

	return dev_list;
}

}