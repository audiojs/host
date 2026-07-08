/*
 * host.c — CLAP plugin host implementation
 */
#include "clap.h"
#include "host.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  static void* lib_open(const char* p) { return (void*)LoadLibraryA(p); }
  static void* lib_sym(void* l, const char* n) { return (void*)GetProcAddress((HMODULE)l, n); }
  static void  lib_close(void* l) { FreeLibrary((HMODULE)l); }
  static const char* lib_error(void) { static char b[128]; snprintf(b, sizeof(b), "error %lu", GetLastError()); return b; }
#else
  #include <dlfcn.h>
  #include <dirent.h>
  static void* lib_open(const char* p) { return dlopen(p, RTLD_NOW); }
  static void* lib_sym(void* l, const char* n) { return dlsym(l, n); }
  static void  lib_close(void* l) { dlclose(l); }
  static const char* lib_error(void) { return dlerror(); }
#endif

static _Thread_local char s_error[512] = {0};

/* --- Empty events --- */

static uint32_t empty_size(const clap_input_events_t* l) { (void)l; return 0; }
static const clap_event_header_t* empty_get(const clap_input_events_t* l, uint32_t i) { (void)l; (void)i; return NULL; }
static bool empty_push(const clap_output_events_t* l, const clap_event_header_t* e) { (void)l; (void)e; return true; }

static const clap_input_events_t s_in_events = { NULL, empty_size, empty_get };
static const clap_output_events_t s_out_events = { NULL, empty_push };

/* --- Host callbacks --- */

static const void* host_get_ext(const clap_host_t* h, const char* id) { (void)h; (void)id; return NULL; }
static void host_noop(const clap_host_t* h) { (void)h; }

static const clap_host_t s_host = {
  CLAP_VERSION_INIT,
  NULL,
  "audio-host", "audiojs", "https://github.com/audiojs/host", "0.1.0",
  host_get_ext, host_noop, host_noop, host_noop
};

/* --- Plugin handle --- */

struct clap_host_plugin {
  void* lib;
  const clap_plugin_entry_t* entry;
  const clap_plugin_t* plugin;
  const clap_plugin_params_t* params;
  char name[256];
  char vendor[256];
};

/* Find binary inside .clap bundle (macOS) or use path directly */
static const char* find_clap_binary(const char* path, char* out, size_t outSize) {
#ifndef _WIN32
  char macosDir[2048];
  snprintf(macosDir, sizeof(macosDir), "%s/Contents/MacOS", path);
  DIR* dir = opendir(macosDir);
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir))) {
      if (entry->d_name[0] == '.') continue;
      snprintf(out, outSize, "%s/%s", macosDir, entry->d_name);
      closedir(dir);
      return out;
    }
    closedir(dir);
  }
#endif
  strncpy(out, path, outSize - 1);
  out[outSize - 1] = 0;
  return out;
}

/* --- C API --- */

const char* clap_host_get_error(void) { return s_error; }

clap_host_plugin_t* clap_host_open(const char* path, double sampleRate, int channels, int blockSize) {
  s_error[0] = 0;

  char binPath[2048];
  find_clap_binary(path, binPath, sizeof(binPath));

  void* lib = lib_open(binPath);
  if (!lib) { snprintf(s_error, sizeof(s_error), "dlopen: %s", lib_error()); return NULL; }

  const clap_plugin_entry_t* entry = (const clap_plugin_entry_t*)lib_sym(lib, "clap_entry");
  if (!entry) { strcpy(s_error, "No clap_entry symbol"); lib_close(lib); return NULL; }

  if (!entry->init(path)) { strcpy(s_error, "clap_entry.init failed"); lib_close(lib); return NULL; }

  const clap_plugin_factory_t* factory = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!factory || factory->get_plugin_count(factory) == 0) {
    strcpy(s_error, "No plugins in factory");
    entry->deinit(); lib_close(lib); return NULL;
  }

  const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, 0);
  const clap_plugin_t* plugin = factory->create_plugin(factory, &s_host, desc->id);
  if (!plugin) { strcpy(s_error, "create_plugin failed"); entry->deinit(); lib_close(lib); return NULL; }

  if (!plugin->init(plugin)) { plugin->destroy(plugin); entry->deinit(); lib_close(lib); strcpy(s_error, "plugin.init failed"); return NULL; }
  if (!plugin->activate(plugin, sampleRate, 1, blockSize)) { plugin->destroy(plugin); entry->deinit(); lib_close(lib); strcpy(s_error, "plugin.activate failed"); return NULL; }
  plugin->start_processing(plugin);

  clap_host_plugin_t* p = (clap_host_plugin_t*)calloc(1, sizeof(clap_host_plugin_t));
  p->lib = lib; p->entry = entry; p->plugin = plugin;
  p->params = (const clap_plugin_params_t*)plugin->get_extension(plugin, CLAP_EXT_PARAMS);
  if (desc->name) strncpy(p->name, desc->name, 255);
  if (desc->vendor) strncpy(p->vendor, desc->vendor, 255);

  return p;
}

void clap_host_close(clap_host_plugin_t* h) {
  if (!h || !h->lib) return; /* already closed */
  if (h->plugin) {
    h->plugin->stop_processing(h->plugin);
    h->plugin->deactivate(h->plugin);
    h->plugin->destroy(h->plugin);
    h->plugin = NULL;
  }
  if (h->entry) { h->entry->deinit(); h->entry = NULL; }
  lib_close(h->lib);
  h->lib = NULL;
  h->params = NULL;
}

void clap_host_destroy(clap_host_plugin_t* h) {
  if (!h) return;
  clap_host_close(h);
  free(h);
}

void clap_host_process(clap_host_plugin_t* h, float** inputs, float** outputs, int numChannels, int numSamples) {
  if (!h || !h->plugin) return;

  clap_audio_buffer_t in_buf = { inputs, NULL, (uint32_t)numChannels, 0, 0 };
  clap_audio_buffer_t out_buf = { outputs, NULL, (uint32_t)numChannels, 0, 0 };

  clap_process_t proc = {
    -1, (uint32_t)numSamples,
    &s_in_events, &s_out_events,
    inputs ? &in_buf : NULL, &out_buf,
    inputs ? 1u : 0u, 1u,
    NULL
  };

  h->plugin->process(h->plugin, &proc);
}

const char* clap_host_get_name(clap_host_plugin_t* h) { return h ? h->name : ""; }
const char* clap_host_get_vendor(clap_host_plugin_t* h) { return h ? h->vendor : ""; }

int clap_host_get_param_count(clap_host_plugin_t* h) {
  return (h && h->params) ? (int)h->params->count(h->plugin) : 0;
}

int clap_host_get_param_info(clap_host_plugin_t* h, int index, clap_host_param_info_t* out) {
  if (!h || !h->params) return -1;
  clap_param_info_t info;
  if (!h->params->get_info(h->plugin, index, &info)) return -1;
  out->id = info.id;
  strncpy(out->name, info.name, 255); out->name[255] = 0;
  out->min = info.min_value;
  out->max = info.max_value;
  out->defaultValue = info.default_value;
  return 0;
}

double clap_host_get_param(clap_host_plugin_t* h, uint32_t id) {
  if (!h || !h->params) return 0;
  double val = 0;
  h->params->get_value(h->plugin, id, &val);
  return val;
}
