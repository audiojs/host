/*
 * host.h — C API for the CLAP host
 */
#pragma once
#include <stdint.h>

typedef struct clap_host_plugin clap_host_plugin_t;

typedef struct {
  uint32_t id;
  char name[256];
  double min;
  double max;
  double defaultValue;
} clap_host_param_info_t;

clap_host_plugin_t* clap_host_open(const char* path, double sampleRate, int channels, int blockSize);
void clap_host_close(clap_host_plugin_t* handle);  /* idempotent deactivate */
void clap_host_destroy(clap_host_plugin_t* handle); /* close + free */
void clap_host_process(clap_host_plugin_t* handle, float** inputs, float** outputs, int numChannels, int numSamples);

const char* clap_host_get_name(clap_host_plugin_t* handle);
const char* clap_host_get_vendor(clap_host_plugin_t* handle);

int clap_host_get_param_count(clap_host_plugin_t* handle);
int clap_host_get_param_info(clap_host_plugin_t* handle, int index, clap_host_param_info_t* info);
double clap_host_get_param(clap_host_plugin_t* handle, uint32_t id);

const char* clap_host_get_error(void);
