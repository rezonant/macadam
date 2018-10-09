/* Copyright 2018 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/* -LICENSE-START-
 ** Copyright (c) 2010 Blackmagic Design
 **
 ** Permission is hereby granted, free of charge, to any person or organization
 ** obtaining a copy of the software and accompanying documentation covered by
 ** this license (the "Software") to use, reproduce, display, distribute,
 ** execute, and transmit the Software, and to prepare derivative works of the
 ** Software, and to permit third-parties to whom the Software is furnished to
 ** do so, all subject to the following:
 **
 ** The copyright notices in the Software and this entire statement, including
 ** the above license grant, this restriction and the following disclaimer,
 ** must be included in all copies of the Software, in whole or in part, and
 ** all derivative works of the Software, unless such copies or derivative
 ** works are solely in the form of machine-executable object code generated by
 ** a source language processor.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 ** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 ** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 ** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ** DEALINGS IN THE SOFTWARE.
 ** -LICENSE-END-
 */

#ifndef CAPTURE_PROMISE_H
#define CAPTURE_PROMISE_H

#define NAPI_EXPERIMENTAL
#include "macadam_util.h"
#include "node_api.h"
#include "DeckLinkAPI.h"

napi_value capture(napi_env env, napi_callback_info info);
napi_value nop(napi_env env, napi_callback_info info);
napi_value framePromise(napi_env env, napi_callback_info info);
napi_value stopStreams(napi_env env, napi_callback_info info);
void frameResolver(napi_env env, napi_value jsCb, void* context, void* data);
void captureTsFnFinalize(napi_env env, void* data, void* hint);

// Carrier used to create a capture instance off-thread
struct captureCarrier : carrier {
  IDeckLinkInput* deckLinkInput = nullptr;
  uint32_t deviceIndex = 0;
  BMDDisplayMode requestedDisplayMode;
  BMDPixelFormat requestedPixelFormat;
  BMDAudioSampleRate requestedSampleRate = bmdAudioSampleRate48kHz;
  BMDAudioSampleType requestedSampleType = bmdAudioSampleType16bitInteger;
  uint32_t channels = 0; // Set to zero for no channels
  IDeckLinkDisplayMode* selectedDisplayMode = nullptr;
  ~captureCarrier() {
    if (deckLinkInput != nullptr) { deckLinkInput->Release(); }
    if (selectedDisplayMode != nullptr) { selectedDisplayMode->Release(); }
  }
};

struct frameCarrier : carrier {
  ~frameCarrier() { }
};

struct captureThreadsafe : IDeckLinkInputCallback {
  HRESULT VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioPacket);
  HRESULT VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
    IDeckLinkDisplayMode *newDisplayMode,
    BMDDetectedVideoInputFormatFlags detectedSignalFlags);
  HRESULT	QueryInterface (REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
  ULONG AddRef() { return 1; }
  ULONG Release() { return 1; }
  napi_threadsafe_function tsFn;
  bool started = false;
  IDeckLinkInput* deckLinkInput = nullptr;
  IDeckLinkDisplayMode* displayMode = nullptr;
  BMDPixelFormat pixelFormat;
  BMDTimeScale timeScale;
  BMDAudioSampleRate sampleRate;
  BMDAudioSampleType sampleType;
  uint32_t channels = 0; // Set to zero for no channels
  frameCarrier* waitingPromise = nullptr;
  ~captureThreadsafe() {
    if (deckLinkInput != nullptr) { deckLinkInput->Release(); }
    if (displayMode != nullptr) { displayMode->Release(); }
  };
};

struct frameData {
  IDeckLinkVideoInputFrame* videoFrame;
  IDeckLinkAudioInputPacket* audioPacket;
};

 #endif // CAPTURE_PROMISE_H