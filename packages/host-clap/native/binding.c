/*
 * binding.c — NAPI bindings for CLAP host
 */
#include <node_api.h>
#include "host.h"

#define NAPI_CALL(env, call) do { \
  napi_status s = (call); \
  if (s != napi_ok) { napi_throw_error(env, NULL, #call " failed"); return NULL; } \
} while(0)

static void destructor(napi_env env, void* data, void* hint) {
  (void)env; (void)hint;
  clap_host_destroy((clap_host_plugin_t*)data);
}

static napi_value node_open(napi_env env, napi_callback_info info) {
  size_t argc = 4; napi_value argv[4];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

  char path[2048]; size_t len;
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &len));

  double sampleRate = 44100; int channels = 2, blockSize = 128;
  if (argc > 1) napi_get_value_double(env, argv[1], &sampleRate);
  if (argc > 2) napi_get_value_int32(env, argv[2], &channels);
  if (argc > 3) napi_get_value_int32(env, argv[3], &blockSize);

  clap_host_plugin_t* plugin = clap_host_open(path, sampleRate, channels, blockSize);
  if (!plugin) {
    const char* err = clap_host_get_error();
    napi_throw_error(env, NULL, err[0] ? err : "Failed to load CLAP plugin");
    return NULL;
  }

  napi_value ext;
  NAPI_CALL(env, napi_create_external(env, plugin, destructor, NULL, &ext));
  return ext;
}

static napi_value node_close(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  clap_host_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  clap_host_close(p);
  return NULL;
}

static napi_value node_getName(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  clap_host_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  napi_value r; napi_create_string_utf8(env, clap_host_get_name(p), NAPI_AUTO_LENGTH, &r);
  return r;
}

static napi_value node_getVendor(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  clap_host_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  napi_value r; napi_create_string_utf8(env, clap_host_get_vendor(p), NAPI_AUTO_LENGTH, &r);
  return r;
}

static napi_value node_getParamCount(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  clap_host_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  napi_value r; napi_create_int32(env, clap_host_get_param_count(p), &r);
  return r;
}

static napi_value node_getParamInfo(napi_env env, napi_callback_info info) {
  size_t argc = 2; napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  clap_host_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  int index; napi_get_value_int32(env, argv[1], &index);

  clap_host_param_info_t pi;
  if (clap_host_get_param_info(p, index, &pi) != 0) return NULL;

  napi_value obj, val;
  napi_create_object(env, &obj);
  napi_create_uint32(env, pi.id, &val); napi_set_named_property(env, obj, "id", val);
  napi_create_string_utf8(env, pi.name, NAPI_AUTO_LENGTH, &val); napi_set_named_property(env, obj, "name", val);
  napi_create_double(env, pi.min, &val); napi_set_named_property(env, obj, "min", val);
  napi_create_double(env, pi.max, &val); napi_set_named_property(env, obj, "max", val);
  napi_create_double(env, pi.defaultValue, &val); napi_set_named_property(env, obj, "defaultValue", val);
  return obj;
}

static napi_value node_getParam(napi_env env, napi_callback_info info) {
  size_t argc = 2; napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  clap_host_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);
  uint32_t id; napi_get_value_uint32(env, argv[1], &id);
  napi_value r; napi_create_double(env, clap_host_get_param(p, id), &r);
  return r;
}

static napi_value node_process(napi_env env, napi_callback_info info) {
  size_t argc = 3; napi_value argv[3];
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  clap_host_plugin_t* p; napi_get_value_external(env, argv[0], (void**)&p);

  float* inPtrs[8] = {0};
  float* outPtrs[8] = {0};
  uint32_t numCh = 0;
  size_t numSamples = 0;
  int hasInputs = 0;

  napi_valuetype inputType;
  napi_typeof(env, argv[1], &inputType);
  if (inputType != napi_null && inputType != napi_undefined) {
    hasInputs = 1;
    napi_get_array_length(env, argv[1], &numCh);
    for (uint32_t i = 0; i < numCh && i < 8; i++) {
      napi_value el; napi_get_element(env, argv[1], i, &el);
      void* data; size_t len;
      napi_get_typedarray_info(env, el, NULL, &len, &data, NULL, NULL);
      inPtrs[i] = (float*)data;
      if (i == 0) numSamples = len;
    }
  }

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
    napi_value el; napi_get_element(env, argv[2], 0, &el);
    size_t len; napi_get_typedarray_info(env, el, NULL, &len, NULL, NULL, NULL);
    numSamples = len;
  }

  clap_host_process(p, hasInputs ? inPtrs : NULL, outPtrs, numCh, (int)numSamples);
  return NULL;
}

static napi_value init(napi_env env, napi_value exports) {
  napi_property_descriptor props[] = {
    { "open",          NULL, node_open,          NULL, NULL, NULL, napi_default, NULL },
    { "close",         NULL, node_close,         NULL, NULL, NULL, napi_default, NULL },
    { "getName",       NULL, node_getName,       NULL, NULL, NULL, napi_default, NULL },
    { "getVendor",     NULL, node_getVendor,     NULL, NULL, NULL, napi_default, NULL },
    { "getParamCount", NULL, node_getParamCount, NULL, NULL, NULL, napi_default, NULL },
    { "getParamInfo",  NULL, node_getParamInfo,  NULL, NULL, NULL, napi_default, NULL },
    { "getParam",      NULL, node_getParam,      NULL, NULL, NULL, napi_default, NULL },
    { "process",       NULL, node_process,       NULL, NULL, NULL, napi_default, NULL },
  };
  napi_define_properties(env, exports, 8, props);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
