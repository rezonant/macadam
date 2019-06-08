#include <inttypes.h>
#include "DeckLinkAPI.h"

class ConvertedVideoFrame : public IDeckLinkVideoFrame {
    uint8_t *data;
    long width;
    long height;
    long rowSize;
    BMDPixelFormat pixelFormat;
    long referenceCount = 1;

    public:
        ConvertedVideoFrame(long width, long height, BMDPixelFormat pixelFormat, long rowSize);
        ~ConvertedVideoFrame();

        virtual long STDMETHODCALLTYPE GetWidth();
        virtual long STDMETHODCALLTYPE GetHeight();
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat();
        virtual long STDMETHODCALLTYPE GetRowBytes();
        virtual HRESULT STDMETHODCALLTYPE GetBytes(void **ptr);
        virtual HRESULT STDMETHODCALLTYPE GetTimecode( 
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode
        );
        virtual HRESULT STDMETHODCALLTYPE GetAncillaryData( 
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary
        );
        virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags();\
        virtual HRESULT QueryInterface (REFIID   riid, LPVOID * ppvObj);
        virtual ULONG Release();
        virtual ULONG AddRef();
};