#include "DeckLinkAPI.h"
#include <assert.h>
#include "converted_frame.h"
#include <stdlib.h>
#include <memory.h>

ConvertedVideoFrame::ConvertedVideoFrame(long width, long height, BMDPixelFormat pixelFormat, long rowSize) {
    this->data = (uint8_t*)malloc(rowSize * height);
    this->width = width;
    this->height = height;
    this->pixelFormat = pixelFormat;
    this->rowSize = rowSize;
}

ConvertedVideoFrame::~ConvertedVideoFrame() {
    if (this->data) {
        free(this->data);
        this->data = nullptr;
    }
}

long STDMETHODCALLTYPE ConvertedVideoFrame::GetWidth() {
    return this->width;
}

long STDMETHODCALLTYPE ConvertedVideoFrame::GetHeight() {
    return this->height;
}

BMDPixelFormat STDMETHODCALLTYPE ConvertedVideoFrame::GetPixelFormat() {
    return this->pixelFormat;
}

long STDMETHODCALLTYPE ConvertedVideoFrame::GetRowBytes() {
    return this->rowSize;
}

HRESULT STDMETHODCALLTYPE ConvertedVideoFrame::GetBytes(void **ptr) {
    *ptr = this->data;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ConvertedVideoFrame::GetTimecode( 
    /* [in] */ BMDTimecodeFormat format,
    /* [out] */ IDeckLinkTimecode **timecode
) {
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE ConvertedVideoFrame::GetAncillaryData( 
    /* [out] */ IDeckLinkVideoFrameAncillary **ancillary
) {
    return S_FALSE;
}

BMDFrameFlags STDMETHODCALLTYPE ConvertedVideoFrame::GetFlags() {
    return bmdFrameFlagDefault;
}

HRESULT ConvertedVideoFrame::QueryInterface (
    REFIID   riid,
    LPVOID * ppvObj
) {
    // Always set out parameter to NULL, validating it first.
    if (!ppvObj)
        return E_INVALIDARG;
    
    *ppvObj = NULL;

    REFIID IUnknown = IID_IUnknown;

    if (0 == memcmp(&riid, &IUnknown, sizeof(REFIID))) {
        // Increment the reference count and return the pointer.
        *ppvObj = (LPVOID)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG ConvertedVideoFrame::Release() {
    ULONG ulRefCount;
    
    #ifdef WIN32
    ulRefCount = InterlockedDecrement(&referenceCount);
    #else
    ulRefCount = __sync_fetch_and_sub(&referenceCount, 1);
    #endif

    if (ulRefCount == 0)
        delete this;

    return ulRefCount;
}

ULONG ConvertedVideoFrame::AddRef() {
    #ifdef WIN32
    InterlockedIncrement(&referenceCount);
    #else 
    __sync_add_and_fetch(&referenceCount, 1);
    #endif

    return referenceCount;
}