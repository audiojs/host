/*
 * gain.c — Minimal CLAP gain plugin for testing host-clap
 */
#include "../native/clap.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct { double gain; } gain_data_t;

static bool gain_init(const clap_plugin_t* p) { (void)p; return true; }
static void gain_destroy(const clap_plugin_t* p) { free(p->plugin_data); free((void*)p); }
static bool gain_activate(const clap_plugin_t* p, double sr, uint32_t mn, uint32_t mx) { (void)p; (void)sr; (void)mn; (void)mx; return true; }
static void gain_deactivate(const clap_plugin_t* p) { (void)p; }
static bool gain_start(const clap_plugin_t* p) { (void)p; return true; }
static void gain_stop(const clap_plugin_t* p) { (void)p; }
static void gain_reset(const clap_plugin_t* p) { (void)p; }
static void gain_main(const clap_plugin_t* p) { (void)p; }

static clap_process_status gain_process(const clap_plugin_t* p, const clap_process_t* proc) {
  gain_data_t* d = (gain_data_t*)p->plugin_data;
  for (uint32_t i = 0; i < proc->audio_inputs_count && i < proc->audio_outputs_count; i++) {
    uint32_t ch = proc->audio_inputs[i].channel_count;
    if (ch > proc->audio_outputs[i].channel_count) ch = proc->audio_outputs[i].channel_count;
    for (uint32_t c = 0; c < ch; c++) {
      float* in = proc->audio_inputs[i].data32[c];
      float* out = proc->audio_outputs[i].data32[c];
      for (uint32_t s = 0; s < proc->frames_count; s++)
        out[s] = in[s] * (float)d->gain;
    }
  }
  return CLAP_PROCESS_CONTINUE;
}

/* Params */
static uint32_t params_count(const clap_plugin_t* p) { (void)p; return 1; }
static bool params_info(const clap_plugin_t* p, uint32_t idx, clap_param_info_t* info) {
  (void)p;
  if (idx != 0) return false;
  memset(info, 0, sizeof(*info));
  info->id = 0;
  strncpy(info->name, "Gain", 255);
  info->min_value = 0.0; info->max_value = 2.0; info->default_value = 1.0;
  return true;
}
static bool params_value(const clap_plugin_t* p, clap_id id, double* v) {
  (void)id; *v = ((gain_data_t*)p->plugin_data)->gain; return true;
}
static bool params_text(const clap_plugin_t* p, clap_id id, double v, char* buf, uint32_t sz) {
  (void)p; (void)id; snprintf(buf, sz, "%.2f", v); return true;
}
static bool params_parse(const clap_plugin_t* p, clap_id id, const char* t, double* v) {
  (void)p; (void)id; (void)t; (void)v; return false;
}
static void params_flush(const clap_plugin_t* p, const clap_input_events_t* in, const clap_output_events_t* out) {
  (void)p; (void)in; (void)out;
}

static const clap_plugin_params_t s_params = { params_count, params_info, params_value, params_text, params_parse, params_flush };

static const void* gain_ext(const clap_plugin_t* p, const char* id) {
  (void)p;
  if (strcmp(id, CLAP_EXT_PARAMS) == 0) return &s_params;
  return NULL;
}

static const char* s_features[] = { "audio-effect", NULL };
static const clap_plugin_descriptor_t s_desc = {
  CLAP_VERSION_INIT, "com.audiojs.test-gain", "Test Gain", "audiojs",
  NULL, NULL, NULL, "0.1.0", "Simple gain for testing", s_features
};

/* Factory */
static uint32_t factory_count(const clap_plugin_factory_t* f) { (void)f; return 1; }
static const clap_plugin_descriptor_t* factory_desc(const clap_plugin_factory_t* f, uint32_t i) { (void)f; return i == 0 ? &s_desc : NULL; }
static const clap_plugin_t* factory_create(const clap_plugin_factory_t* f, const clap_host_t* h, const char* id) {
  (void)f; (void)h;
  if (strcmp(id, s_desc.id) != 0) return NULL;
  clap_plugin_t* p = (clap_plugin_t*)calloc(1, sizeof(clap_plugin_t));
  gain_data_t* d = (gain_data_t*)calloc(1, sizeof(gain_data_t));
  d->gain = 1.0;
  p->desc = &s_desc; p->plugin_data = d;
  p->init = gain_init; p->destroy = gain_destroy;
  p->activate = gain_activate; p->deactivate = gain_deactivate;
  p->start_processing = gain_start; p->stop_processing = gain_stop;
  p->reset = gain_reset; p->process = gain_process;
  p->get_extension = gain_ext; p->on_main_thread = gain_main;
  return p;
}

static const clap_plugin_factory_t s_factory = { factory_count, factory_desc, factory_create };

/* Entry */
static bool entry_init(const char* path) { (void)path; return true; }
static void entry_deinit(void) {}
static const void* entry_get_factory(const char* id) {
  if (strcmp(id, CLAP_PLUGIN_FACTORY_ID) == 0) return &s_factory;
  return NULL;
}

__attribute__((visibility("default")))
const clap_plugin_entry_t clap_entry = { CLAP_VERSION_INIT, entry_init, entry_deinit, entry_get_factory };
