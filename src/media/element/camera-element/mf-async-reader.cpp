#include "mf-async-reader.h"
#include "log.h"

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MFAsyncReader::MFAsyncReader(ISrcPin* src_pin, HANDLE hEvent)
	: m_src_pin(src_pin)
	, m_nRefCount(1)
	, m_bEOS(FALSE)
	, m_hEvent(hEvent)
	, m_hrStatus(S_OK)
{
	InitializeCriticalSection(&m_critsec);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
STDMETHODIMP MFAsyncReader::QueryInterface(REFIID iid, void** ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(MFAsyncReader, IMFSourceReaderCallback),
		{ 0 },
	};
	return QISearch(this, qit, iid, ppv);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) MFAsyncReader::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) MFAsyncReader::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	return uCount;
}

//------------------------------------------------------------------------------
// 异步采集，系统回调视频帧
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFAsyncReader::OnReadSample(HRESULT hrStatus,
	DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, 
	IMFSample* pSample) {
	EnterCriticalSection(&m_critsec);
	if (SUCCEEDED(hrStatus)) {
		if (pSample) {
			IMFMediaBuffer* pMB = nullptr;
			if (FAILED(pSample->GetBufferByIndex(0, &pMB))) {
				LOG_ERR("GetBufferByIndex failed!");
				goto exit;
			}

			BYTE* pBuf;
			DWORD dwMaxLen, dwCurLen;
			if (FAILED(pMB->Lock(&pBuf, &dwMaxLen, &dwCurLen))) {
				LOG_ERR("Lock buffer failed!");
				goto exit;
			}

			PinData pin_data(media::MediaType::VIDEO, pBuf, dwCurLen);
			m_src_pin->OnPinData(pin_data);

			pMB->Unlock();
			pMB->Release();
		}
	}
	else {
		LOG_ERR("Streaming error!");
	}

	if (MF_SOURCE_READERF_ENDOFSTREAM & dwStreamFlags)
	{
		// Reached the end of the stream.
		m_bEOS = TRUE;
	}
	m_hrStatus = hrStatus;

exit:
	LeaveCriticalSection(&m_critsec);
	SetEvent(m_hEvent);

	return S_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFAsyncReader::OnFlush(DWORD dwStreamIndex)
{
	LOG_INF("MFAsyncReader::OnFlush");
	return S_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFAsyncReader::OnEvent(DWORD dwStreamIndex,
	IMFMediaEvent* pEvent)
{
	LOG_INF("MFAsyncReader::OnEvent");
	return S_OK;
}

}