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

/* process(handle, inputs, outputs) — planar Float32 buffers of equal length. */
static napi_value node_process(napi_env env, napi_callback_info info) {
  size_t argc = 3; napi_value argv[3];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
  if (argc != 3) {
    napi_throw_type_error(env, NULL, "process requires input and output arrays");
    return NULL;
  }
  vst3_plugin_t* p;
  NAPI_CALL(env, napi_get_value_external(env, argv[0], (void**)&p));
  float* ptrs[2][8] = {};
  uint32_t count[2] = {};
  size_t samples = 0;
  bool haveSamples = false;
  for (int bus = 0; bus < 2; bus++) {
    napi_valuetype kind;
    NAPI_CALL(env, napi_typeof(env, argv[bus + 1], &kind));
    if (bus == 0 && (kind == napi_null || kind == napi_undefined)) continue;
    bool array;
    NAPI_CALL(env, napi_is_array(env, argv[bus + 1], &array));
    if (!array) {
      napi_throw_type_error(env, NULL, "channels must be an array of Float32Array");
      return NULL;
    }
    NAPI_CALL(env, napi_get_array_length(env, argv[bus + 1], &count[bus]));
    if (count[bus] > 8 || (bus == 1 && count[bus] == 0)) {
      napi_throw_range_error(env, NULL, "expected at most 8 channels and at least one output");
      return NULL;
    }
    for (uint32_t i = 0; i < count[bus]; i++) {
      napi_value el; void* data; size_t len; napi_typedarray_type type;
      NAPI_CALL(env, napi_get_element(env, argv[bus + 1], i, &el));
      if (napi_get_typedarray_info(env, el, &type, &len, &data, NULL, NULL) != napi_ok || type != napi_float32_array) {
        napi_throw_type_error(env, NULL, "channels must be Float32Array");
        return NULL;
      }
      if (len > INT32_MAX || (haveSamples && len != samples)) {
        napi_throw_range_error(env, NULL, "channel lengths must match and fit i32");
        return NULL;
      }
      samples = len; haveSamples = true;
      ptrs[bus][i] = (float*)data;
    }
  }
  vst3_process_io(p, count[0] ? ptrs[0] : NULL, count[0], ptrs[1], count[1], (int)samples);
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
