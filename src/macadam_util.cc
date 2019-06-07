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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <string>
#include "macadam_util.h"
#include "node_api.h"

#ifdef WIN32
#include <tchar.h>
#include <conio.h>
#include <objbase.h>		// Necessary for COM
#include <comdef.h>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CFString.h>
#endif

napi_status checkStatus(napi_env env, napi_status status,
  const char* file, uint32_t line) {

  napi_status infoStatus, throwStatus;
  const napi_extended_error_info *errorInfo;

  if (status == napi_ok) {
    // printf("Received status OK.\n");
    return status;
  }

  infoStatus = napi_get_last_error_info(env, &errorInfo);
  assert(infoStatus == napi_ok);
  printf("NAPI error in file %s on line %i. Error %i: %s\n", file, line,
    errorInfo->error_code, errorInfo->error_message);

  if (status == napi_pending_exception) {
    printf("NAPI pending exception. Engine error code: %i\n", errorInfo->engine_error_code);
    return status;
  }

  char errorCode[20];
  sprintf(errorCode, "%d", errorInfo->error_code);
  throwStatus = napi_throw_error(env, errorCode, errorInfo->error_message);
  assert(throwStatus == napi_ok);

  return napi_pending_exception; // Expect to be cast to void
}

long long microTime(std::chrono::high_resolution_clock::time_point start) {
  auto elapsed = std::chrono::high_resolution_clock::now() - start;
  return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

const char* getNapiTypeName(napi_valuetype t) {
  switch (t) {
    case napi_undefined: return "undefined";
    case napi_null: return "null";
    case napi_boolean: return "boolean";
    case napi_number: return "number";
    case napi_string: return "string";
    case napi_symbol: return "symbol";
    case napi_object: return "object";
    case napi_function: return "function";
    case napi_external: return "external";
    default: return "unknown";
  }
}

napi_status checkArgs(napi_env env, napi_callback_info info, char* methodName,
  napi_value* args, size_t argc, napi_valuetype* types) {

  napi_status status;

  size_t realArgc = argc;
  status = napi_get_cb_info(env, info, &realArgc, args, nullptr, nullptr);
  if (status != napi_ok) return status;

  if (realArgc != argc) {
    char errorMsg[100];
    sprintf(errorMsg, "For method %s, expected %zi arguments and got %zi.",
      methodName, argc, realArgc);
    napi_throw_error(env, nullptr, errorMsg);
    return napi_pending_exception;
  }

  napi_valuetype t;
  for ( size_t x = 0 ; x < argc ; x++ ) {
    status = napi_typeof(env, args[x], &t);
    if (status != napi_ok) return status;
    if (t != types[x]) {
      char errorMsg[100];
      sprintf(errorMsg, "For method %s argument %zu, expected type %s and got %s.",
        methodName, x + 1, getNapiTypeName(types[x]), getNapiTypeName(t));
      napi_throw_error(env, nullptr, errorMsg);
      return napi_pending_exception;
    }
  }

  return napi_ok;
};


void tidyCarrier(napi_env env, carrier* c) {
  napi_status status;
  if (c->passthru != nullptr) {
    status = napi_delete_reference(env, c->passthru);
    FLOATING_STATUS;
  }
  if (c->_request != nullptr) {
    status = napi_delete_async_work(env, c->_request);
    FLOATING_STATUS;
  }
  // printf("Tidying carrier %p %p\n", c->passthru, c->_request);
  delete c;
}

int32_t rejectStatus(napi_env env, carrier* c, char* file, int32_t line) {
  if (c->status != MACADAM_SUCCESS) {
    napi_value errorValue, errorCode, errorMsg;
    napi_status status;
    if (c->status < MACADAM_ERROR_START) {
      const napi_extended_error_info *errorInfo;
      status = napi_get_last_error_info(env, &errorInfo);
      FLOATING_STATUS;
      c->errorMsg = std::string(
        (errorInfo->error_message != nullptr) ? errorInfo->error_message : "(no message)");
    }
    char* extMsg = (char *) malloc(sizeof(char) * c->errorMsg.length() + 200);
    sprintf(extMsg, "In file %s on line %i, found error: %s", file, line, c->errorMsg.c_str());
    char errorCodeChars[20];
    sprintf(errorCodeChars, "%d", c->status);
    status = napi_create_string_utf8(env, errorCodeChars,
      NAPI_AUTO_LENGTH, &errorCode);
    FLOATING_STATUS;
    status = napi_create_string_utf8(env, extMsg, NAPI_AUTO_LENGTH, &errorMsg);
    FLOATING_STATUS;
    status = napi_create_error(env, errorCode, errorMsg, &errorValue);
    FLOATING_STATUS;
    status = napi_reject_deferred(env, c->_deferred, errorValue);
    FLOATING_STATUS;

    delete[] extMsg;
    tidyCarrier(env, c);
  }
  return c->status;
}

// Should never get called
napi_value nop(napi_env env, napi_callback_info info) {
  napi_value value;
  napi_status status;
  status = napi_get_undefined(env, &value);
  if (status != napi_ok) NAPI_THROW_ERROR("Failed to retrieve undefined in nop.");
  return value;
}

napi_status serializeDisplayMode(napi_env env, IDeckLinkOutput *deckLinkIO, IDeckLinkDisplayMode *displayMode, napi_value *result) {
  napi_status status;
  HRESULT hresult;
  #if defined(WIN32) || defined(__APPLE__)
  char modeName[64];
  #else
  char * modeName;
  #endif
  long modeWidth;
  long modeHeight;
  BMDTimeValue frameRateDuration;
  BMDTimeScale frameRateScale;
  napi_value modeobj, item, itemPart;
  uint32_t modeIndex = 0, partIndex = 0;
  int	pixelFormatIndex = 0; // index into the gKnownPixelFormats / gKnownFormatNames arrays
  BMDDisplayModeSupport	displayModeSupport;
  BMDFieldDominance fieldDominance;
  BMDDisplayMode displayModeID;
  BMDDisplayModeFlags displayModeFlags;

  status = napi_create_object(env, &modeobj);
  CHECK_BAIL;

  #ifdef WIN32
  BSTR			displayModeBSTR = NULL;
  hresult = displayMode->GetName(&displayModeBSTR);
  if (hresult == S_OK)
  {
    _bstr_t	modeNameWin(displayModeBSTR, false);
    strcpy(modeName, (const char*) modeNameWin);
  }
  #elif __APPLE__
  CFStringRef	displayModeCFString = NULL;
  hresult = displayMode->GetName(&displayModeCFString);
  if (hresult == S_OK) {
    CFStringGetCString(displayModeCFString, modeName, sizeof(modeName), kCFStringEncodingMacRoman);
    CFRelease(displayModeCFString);
  }
  #else
  hresult = displayMode->GetName((const char **) &modeName);
  #endif

  status = napi_create_string_utf8(env, modeName, NAPI_AUTO_LENGTH, &item);
  CHECK_BAIL;
  status = napi_set_named_property(env, modeobj, "name", item);
  CHECK_BAIL;

  modeWidth = displayMode->GetWidth();
  status = napi_create_int64(env, modeWidth, &item);
  CHECK_BAIL;
  status = napi_set_named_property(env, modeobj, "width", item);
  CHECK_BAIL;

  modeHeight = displayMode->GetHeight();
  status = napi_create_int64(env, modeHeight, &item);
  CHECK_BAIL;
  status = napi_set_named_property(env, modeobj, "height", item);
  CHECK_BAIL;

  napi_value param;

  displayModeID = displayMode->GetDisplayMode();
  displayModeFlags = displayMode->GetFlags();
  napi_create_int64(env, displayModeID, &param);
  napi_set_named_property(env, modeobj, "displayMode", param);
  napi_create_int64(env, displayModeFlags, &param);
  napi_set_named_property(env, modeobj, "displayModeFlags", param);

  fieldDominance = displayMode->GetFieldDominance();
  status = napi_create_int64(env, modeHeight, &item);
  CHECK_BAIL;
  status = napi_set_named_property(env, modeobj, "fieldDominance", item);
  CHECK_BAIL;

  displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);
  // printf(" %-20s \t %d x %d \t %7g FPS\t", displayModeString, modeWidth, modeHeight, (double)frameRateScale / (double)frameRateDuration);
  status = napi_create_array(env, &item);
  CHECK_BAIL;
  status = napi_create_int64(env, frameRateDuration, &itemPart);
  CHECK_BAIL;
  status = napi_set_element(env, item, 0, itemPart);
  CHECK_BAIL;
  status = napi_create_int64(env, frameRateScale, &itemPart);
  CHECK_BAIL;
  status = napi_set_element(env, item, 1, itemPart);
  CHECK_BAIL;
  status = napi_set_named_property(env, modeobj, "frameRate", item);
  CHECK_BAIL;

  status = napi_create_array(env, &item);
  CHECK_BAIL;

  partIndex = 0;
  pixelFormatIndex = 0;

  if (deckLinkIO != nullptr) {
    while ((gKnownPixelFormats[pixelFormatIndex] != 0) &&
        (gKnownPixelFormatNames[pixelFormatIndex] != NULL)) {
      if ((deckLinkIO->DoesSupportVideoMode(
        displayMode->GetDisplayMode(), gKnownPixelFormats[pixelFormatIndex],
        bmdVideoOutputFlagDefault, &displayModeSupport, NULL) == S_OK)
          && (displayModeSupport != bmdDisplayModeNotSupported)) {

        status = napi_create_string_utf8(env, gKnownPixelFormatNames[pixelFormatIndex],
          NAPI_AUTO_LENGTH, &itemPart);
        CHECK_BAIL;
        status = napi_set_element(env, item, partIndex++, itemPart);
        CHECK_BAIL;
      }

      pixelFormatIndex++;
    }
  }

  status = napi_set_named_property(env, modeobj, "videoModes", item);
  CHECK_BAIL;

  *result = modeobj;

  return napi_ok;
  
  bail:
    return napi_generic_failure;
}