/*
 * binding.cpp — NAPI bindings for VST3 host
 */
#include <node_api.h>
#include "host.h"

#define NAPI_CALL(env, call) do { \
  napi_status s = (call); \
  if (s != napi_ok) { napi_throw_error(env, NULL, #call " failed"); return NULL; } \
} while(0)

static void destructor(napi_env env, void* data, void* hint) {
  (void)env; (void)hint;
  vst3_destroy((vst3_plugin_t*)data);
}

/* open(path, sampleRate, channels, blockSize) → external */
static napi_value node_open(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value argv[4];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

  char path[2048];
  size_t len;
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &len));

  double sampleRate = 44100; int channels = 2, blockSize = 128;
  if (argc > 1) napi_get_value_double(env, argv[1], &sampleRate);
  if (argc > 2) napi_get_value_int32(env, argv[2], &channels);
  if (argc > 3) napi_get_value_int32(env, argv[3], &blockSize);

  vst3_plugin_t* plugin = vst3_open(path, sampleRate, channels, blockSize);
  if (!plugin) {
    const char* err = vst3_get_error();
    napi_throw_error(env, NULL, err[0] ? err : "Failed to load VST3 plugin");
    return NULL;
  }

  napi_value ext;
  NAPI_CALL(env, napi_create_external(env, plugin, destructor, NULL, &ext));
  return ext;
}

/* close(handle) */
static napi_value node_close(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  vst3_close(p);
  return NULL;
}

/* getName(handle) → string */
static napi_value node_getName(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  napi_value result;
  napi_create_string_utf8(env, vst3_get_name(p), NAPI_AUTO_LENGTH, &result);
  return result;
}

/* getVendor(handle) → string */
static napi_value node_getVendor(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  napi_value result;
  napi_create_string_utf8(env, vst3_get_vendor(p), NAPI_AUTO_LENGTH, &result);
  return result;
}

/* getChannels(handle, dir) → number */
static napi_value node_getChannels(napi_env env, napi_callback_info info) {
  size_t argc = 2; napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  int dir = 0; if (argc > 1) napi_get_value_int32(env, argv[1], &dir);
  napi_value result;
  napi_create_int32(env, vst3_get_channels(p, dir), &result);
  return result;
}

/* getParamCount(handle) → number */
static napi_value node_getParamCount(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  napi_value result;
  napi_create_int32(env, vst3_get_param_count(p), &result);
  return result;
}

/* getParamInfo(handle, index) → { id, name, min, max, defaultValue, stepCount } */
static napi_value node_getParamInfo(napi_env env, napi_callback_info info) {
  size_t argc = 2; napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  int index; napi_get_value_int32(env, argv[1], &index);

  vst3_param_info_t pi;
  if (vst3_get_param_info(p, index, &pi) != 0) return NULL;

  napi_value obj, val;
  napi_create_object(env, &obj);

  napi_create_uint32(env, pi.id, &val); napi_set_named_property(env, obj, "id", val);
  napi_create_string_utf8(env, pi.name, NAPI_AUTO_LENGTH, &val); napi_set_named_property(env, obj, "name", val);
  napi_create_double(env, pi.min, &val); napi_set_named_property(env, obj, "min", val);
  napi_create_double(env, pi.max, &val); napi_set_named_property(env, obj, "max", val);
  napi_create_double(env, pi.defaultValue, &val); napi_set_named_property(env, obj, "defaultValue", val);
  napi_create_int32(env, pi.stepCount, &val); napi_set_named_property(env, obj, "stepCount", val);

  return obj;
}

/* getParam(handle, id) → number */
static napi_value node_getParam(napi_env env, napi_callback_info info) {
  size_t argc = 2; napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  uint32_t id; napi_get_value_uint32(env, argv[1], &id);
  napi_value result;
  napi_create_double(env, vst3_get_param(p, id), &result);
  return result;
}

/* setParam(handle, id, value) */
static napi_value node_setParam(napi_env env, napi_callback_info info) {
  size_t argc = 3; napi_value argv[3];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  uint32_t id; napi_get_value_uint32(env, argv[1], &id);
  double val; napi_get_value_double(env, argv[2], &val);
  vst3_set_param(p, id, val);
  return NULL;
}

/* process(handle, inputs, outputs) — inputs/outputs are arrays of Float32Array */
static napi_value node_process(napi_env env, napi_callback_info info) {
  size_t argc = 3; napi_value argv[3];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);

  float* inPtrs[8] = {};
  float* outPtrs[8] = {};
  uint32_t numCh = 0;
  size_t numSamples = 0;

  /* Extract input channel pointers */
  bool hasInputs = false;
  napi_valuetype inputType;
  napi_typeof(env, argv[1], &inputType);
  if (inputType != napi_null && inputType != napi_undefined) {
    hasInputs = true;
    napi_get_array_length(env, argv[1], &numCh);
    for (uint32_t i = 0; i < numCh && i < 8; i++) {
      napi_value el; napi_get_element(env, argv[1], i, &el);
      void* data; size_t len;
      napi_get_typedarray_info(env, el, NULL, &len, &data, NULL, NULL);
      inPtrs[i] = (float*)data;
      if (i == 0) numSamples = len;
    }
  }

  /* Extract output channel pointers */
  uint32_t outCh;
  napi_get_array_length(env, argv[2], &outCh);
  for (uint32_t i = 0; i < outCh && i < 8; i++) {
    napi_value el; napi_get_element(env, argv[2], i, &el);
    void* data;
    napi_get_typedarray_info(env, el, NULL, NULL, &data, NULL, NULL);
    outPtrs[i] = (float*)data;
  }
  if (!numCh) numCh = outCh;
  if (!numSamples) {
    /* Get length from output if no input */
    napi_value el; napi_get_element(env, argv[2], 0, &el);
    size_t len; napi_get_typedarray_info(env, el, NULL, &len, NULL, NULL, NULL);
    numSamples = len;
  }

  vst3_process(p, hasInputs ? inPtrs : NULL, outPtrs, numCh, (int)numSamples);
  return NULL;
}

/* getState(handle) → Buffer */
static napi_value node_getState(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);

  void* data; int size;
  if (vst3_get_state(p, &data, &size) != 0) return NULL;

  napi_value buf;
  void* bufData;
  napi_create_buffer_copy(env, size, data, &bufData, &buf);
  vst3_free_state(data);
  return buf;
}

/* setState(handle, buffer) */
static napi_value node_setState(napi_env env, napi_callback_info info) {
  size_t argc = 2; napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  vst3_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);

  void* data; size_t len;
  napi_get_buffer_info(env, argv[1], &data, &len);
  vst3_set_state(p, data, (int)len);
  return NULL;
}

/* Module init */
static napi_value init(napi_env env, napi_value exports) {
  napi_property_descriptor props[] = {
    { "open",          NULL, node_open,          NULL, NULL, NULL, napi_default, NULL },
    { "close",         NULL, node_close,         NULL, NULL, NULL, napi_default, NULL },
    { "getName",       NULL, node_getName,       NULL, NULL, NULL, napi_default, NULL },
    { "getVendor",     NULL, node_getVendor,     NULL, NULL, NULL, napi_default, NULL },
    { "getChannels",   NULL, node_getChannels,   NULL, NULL, NULL, napi_default, NULL },
    { "getParamCount", NULL, node_getParamCount, NULL, NULL, NULL, napi_default, NULL },
    { "getParamInfo",  NULL, node_getParamInfo,  NULL, NULL, NULL, napi_default, NULL },
    { "getParam",      NULL, node_getParam,      NULL, NULL, NULL, napi_default, NULL },
    { "setParam",      NULL, node_setParam,      NULL, NULL, NULL, napi_default, NULL },
    { "process",       NULL, node_process,       NULL, NULL, NULL, napi_default, NULL },
    { "getState",      NULL, node_getState,      NULL, NULL, NULL, napi_default, NULL },
    { "setState",      NULL, node_setState,      NULL, NULL, NULL, napi_default, NULL },
  };
  napi_define_properties(env, exports, 12, props);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
