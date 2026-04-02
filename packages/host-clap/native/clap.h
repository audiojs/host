/*
 * clap.h — Minimal CLAP interface definitions for plugin hosting
 *
 * Based on CLAP SDK (https://github.com/free-audio/clap), MIT License.
 * Only types needed for basic audio hosting are included.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define CLAP_VERSION_MAJOR 1
#define CLAP_VERSION_MINOR 2
#define CLAP_VERSION_REVISION 2
#define CLAP_VERSION_INIT { CLAP_VERSION_MAJOR, CLAP_VERSION_MINOR, CLAP_VERSION_REVISION }

typedef struct { uint32_t major, minor, revision; } clap_version_t;

/* --- Entry point (exported from .clap binary) --- */

typedef struct clap_plugin_entry {
  clap_version_t clap_version;
  bool (*init)(const char* plugin_path);
  void (*deinit)(void);
  const void* (*get_factory)(const char* factory_id);
} clap_plugin_entry_t;

/* --- Plugin factory --- */

#define CLAP_PLUGIN_FACTORY_ID "clap.plugin-factory"

typedef struct clap_plugin_descriptor {
  clap_version_t clap_version;
  const char* id;
  const char* name;
  const char* vendor;
  const char* url;
  const char* manual_url;
  const char* support_url;
  const char* version;
  const char* description;
  const char* const* features;
} clap_plugin_descriptor_t;

typedef struct clap_plugin clap_plugin_t;
typedef struct clap_host clap_host_t;

typedef struct clap_plugin_factory {
  uint32_t (*get_plugin_count)(const struct clap_plugin_factory* factory);
  const clap_plugin_descriptor_t* (*get_plugin_descriptor)(const struct clap_plugin_factory* factory, uint32_t index);
  const clap_plugin_t* (*create_plugin)(const struct clap_plugin_factory* factory, const clap_host_t* host, const char* plugin_id);
} clap_plugin_factory_t;

/* --- Host (we implement this) --- */

struct clap_host {
  clap_version_t clap_version;
  void* host_data;
  const char* name;
  const char* vendor;
  const char* url;
  const char* version;
  const void* (*get_extension)(const clap_host_t* host, const char* extension_id);
  void (*request_restart)(const clap_host_t* host);
  void (*request_process)(const clap_host_t* host);
  void (*request_callback)(const clap_host_t* host);
};

/* --- Audio buffer --- */

typedef struct clap_audio_buffer {
  float** data32;
  double** data64;
  uint32_t channel_count;
  uint32_t latency;
  uint64_t constant_mask;
} clap_audio_buffer_t;

/* --- Events --- */

typedef struct clap_event_header {
  uint32_t size;
  uint32_t time;
  uint16_t space_id;
  uint16_t type;
  uint32_t flags;
} clap_event_header_t;

typedef struct clap_input_events {
  void* ctx;
  uint32_t (*size)(const struct clap_input_events* list);
  const clap_event_header_t* (*get)(const struct clap_input_events* list, uint32_t index);
} clap_input_events_t;

typedef struct clap_output_events {
  void* ctx;
  bool (*try_push)(const struct clap_output_events* list, const clap_event_header_t* event);
} clap_output_events_t;

/* --- Process --- */

typedef enum {
  CLAP_PROCESS_ERROR = 0,
  CLAP_PROCESS_CONTINUE = 1,
  CLAP_PROCESS_CONTINUE_IF_NOT_QUIET = 2,
  CLAP_PROCESS_TAIL = 3,
  CLAP_PROCESS_SLEEP = 4,
} clap_process_status;

typedef struct clap_process {
  int64_t steady_time;
  uint32_t frames_count;
  const clap_input_events_t* in_events;
  const clap_output_events_t* out_events;
  const clap_audio_buffer_t* audio_inputs;
  clap_audio_buffer_t* audio_outputs;
  uint32_t audio_inputs_count;
  uint32_t audio_outputs_count;
  const void* transport;
} clap_process_t;

/* --- Plugin interface --- */

struct clap_plugin {
  const clap_plugin_descriptor_t* desc;
  void* plugin_data;
  bool (*init)(const clap_plugin_t* plugin);
  void (*destroy)(const clap_plugin_t* plugin);
  bool (*activate)(const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames);
  void (*deactivate)(const clap_plugin_t* plugin);
  bool (*start_processing)(const clap_plugin_t* plugin);
  void (*stop_processing)(const clap_plugin_t* plugin);
  void (*reset)(const clap_plugin_t* plugin);
  clap_process_status (*process)(const clap_plugin_t* plugin, const clap_process_t* process);
  const void* (*get_extension)(const clap_plugin_t* plugin, const char* id);
  void (*on_main_thread)(const clap_plugin_t* plugin);
};

/* --- Params extension --- */

#define CLAP_EXT_PARAMS "clap.params"
typedef uint32_t clap_id;

typedef struct clap_param_info {
  clap_id id;
  uint32_t flags;
  void* cookie;
  char name[256];
  char module[1024];
  double min_value;
  double max_value;
  double default_value;
} clap_param_info_t;

typedef struct clap_plugin_params {
  uint32_t (*count)(const clap_plugin_t* plugin);
  bool (*get_info)(const clap_plugin_t* plugin, uint32_t param_index, clap_param_info_t* param_info);
  bool (*get_value)(const clap_plugin_t* plugin, clap_id param_id, double* out_value);
  bool (*value_to_text)(const clap_plugin_t* plugin, clap_id param_id, double value, char* out_buffer, uint32_t out_buffer_capacity);
  bool (*text_to_value)(const clap_plugin_t* plugin, clap_id param_id, const char* param_value_text, double* out_value);
  void (*flush)(const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out);
} clap_plugin_params_t;

