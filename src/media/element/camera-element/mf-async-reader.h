#pragma once

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmsdkidl.h>
#include <shlwapi.h>

#include "if-pin.h"

namespace jukey::stmr
{

//==============================================================================
//
//==============================================================================
class MFAsyncReader : public IMFSourceReaderCallback
{
public:
	MFAsyncReader(ISrcPin* src_pin, HANDLE hEvent);

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFSourceReaderCallback
	virtual HRESULT STDMETHODCALLTYPE OnReadSample(
		HRESULT hrStatus,
		DWORD dwStreamIndex,
		DWORD dwStreamFlags,
		LONGLONG llTimestamp,
		IMFSample* pSample) override;
	virtual HRESULT STDMETHODCALLTYPE OnFlush(
		DWORD dwStreamIndex) override;
	virtual HRESULT STDMETHODCALLTYPE OnEvent(
		DWORD dwStreamIndex,
		IMFMediaEvent* pEvent) override;

private:
	ISrcPin* m_src_pin = nullptr;

	long                m_nRefCount;        // Reference count.
	CRITICAL_SECTION    m_critsec;
	HANDLE              m_hEvent;
	BOOL                m_bEOS;
	HRESULT             m_hrStatus;
};

}