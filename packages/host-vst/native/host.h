/*
 * host.h — C API for the VST3 host
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct vst3_plugin vst3_plugin_t;

typedef struct {
  uint32_t id;
  char name[256];
  double min;
  double max;
  double defaultValue;
  int32_t stepCount;
} vst3_param_info_t;

vst3_plugin_t* vst3_open(const char* path, double sampleRate, int channels, int blockSize);
void vst3_close(vst3_plugin_t* handle);  /* idempotent deactivate */
void vst3_destroy(vst3_plugin_t* handle); /* close + free */
void vst3_process(vst3_plugin_t* handle, float** inputs, float** outputs, int numChannels, int numSamples);

const char* vst3_get_name(vst3_plugin_t* handle);
const char* vst3_get_vendor(vst3_plugin_t* handle);
int vst3_get_channels(vst3_plugin_t* handle, int dir); /* 0=in, 1=out */

int vst3_get_param_count(vst3_plugin_t* handle);
int vst3_get_param_info(vst3_plugin_t* handle, int index, vst3_param_info_t* info);
double vst3_get_param(vst3_plugin_t* handle, uint32_t id);
void vst3_set_param(vst3_plugin_t* handle, uint32_t id, double value);

int vst3_get_state(vst3_plugin_t* handle, void** data, int* size);
int vst3_set_state(vst3_plugin_t* handle, const void* data, int size);
void vst3_free_state(void* data);

const char* vst3_get_error(void);

#ifdef __cplusplus
}
#endif
