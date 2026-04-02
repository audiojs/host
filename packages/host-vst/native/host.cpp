/*
 * host.cpp — VST3 plugin host implementation
 */
#include "vst3.h"
#include "host.h"
#include <string>
#include <vector>
#include <cstring>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <dlfcn.h>
  #include <dirent.h>
#endif

#ifdef __APPLE__
  #include <CoreFoundation/CoreFoundation.h>
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;

/* Non-COM TUIDs for factory->createInstance */
static const TUID IID_IComponent = {
  (int8)0xE8,0x31,(int8)0xFF,0x31, (int8)0xF2,(int8)0xD5,0x43,0x01,
  (int8)0x92,(int8)0x8E,(int8)0xBB,(int8)0xEE, 0x25,0x69,0x78,0x02 };
static const TUID IID_FUnknown = {
  0,0,0,0, 0,0,0,0, (int8)0xC0,0,0,0, 0,0,0,0x46 };

/* --- Cross-platform dynamic loading --- */

#ifdef _WIN32
  static inline void* lib_open(const char* path) { return (void*)LoadLibraryA(path); }
  static inline void* lib_sym(void* lib, const char* name) { return (void*)GetProcAddress((HMODULE)lib, name); }
  static inline void  lib_close(void* lib) { FreeLibrary((HMODULE)lib); }
  static inline const char* lib_error() { static char buf[128]; snprintf(buf, sizeof(buf), "error %lu", GetLastError()); return buf; }
#else
  static inline void* lib_open(const char* path) { return dlopen(path, RTLD_NOW); }
  static inline void* lib_sym(void* lib, const char* name) { return dlsym(lib, name); }
  static inline void  lib_close(void* lib) { dlclose(lib); }
  static inline const char* lib_error() { return dlerror(); }
#endif

static thread_local char s_error[512] = {0};
static void set_error(const char* fmt, const char* detail = "") {
  snprintf(s_error, sizeof(s_error), "%s%s", fmt, detail);
}

/* --- Host-side COM implementations --- */

class HostContext : public IHostApplication {
  uint32 ref = 1;
public:
  tresult queryInterface(const TUID _iid, void** obj) override {
    if (tuid_match(_iid, UID_FUnknown) || tuid_match(_iid, UID_IHostApplication)) {
      addRef(); *obj = this; return kResultOk;
    }
    *obj = nullptr; return kResultFalse;
  }
  uint32 addRef() override { return ++ref; }
  uint32 release() override { if (--ref == 0) { delete this; return 0; } return ref; }
  tresult getName(String128 name) override {
    const char16 n[] = u"audio-host";
    memcpy(name, n, sizeof(n));
    return kResultOk;
  }
  tresult createInstance(const TUID, const TUID, void** obj) override {
    *obj = nullptr; return kNotImplemented;
  }
};

class MemStream : public IBStream {
  std::vector<uint8_t> buf;
  int64 pos = 0;
  uint32 ref = 1;
public:
  tresult queryInterface(const TUID _iid, void** obj) override {
    if (tuid_match(_iid, UID_FUnknown) || tuid_match(_iid, UID_IBStream)) {
      addRef(); *obj = this; return kResultOk;
    }
    *obj = nullptr; return kResultFalse;
  }
  uint32 addRef() override { return ++ref; }
  uint32 release() override { if (--ref == 0) { delete this; return 0; } return ref; }
  tresult read(void* buffer, int32 numBytes, int32* numBytesRead) override {
    int32 avail = (int32)buf.size() - (int32)pos;
    int32 n = numBytes < avail ? numBytes : (avail > 0 ? avail : 0);
    if (n > 0) memcpy(buffer, buf.data() + pos, n);
    pos += n;
    if (numBytesRead) *numBytesRead = n;
    return kResultOk;
  }
  tresult write(void* buffer, int32 numBytes, int32* numBytesWritten) override {
    if (pos + numBytes > (int64)buf.size()) buf.resize(pos + numBytes);
    memcpy(buf.data() + pos, buffer, numBytes);
    pos += numBytes;
    if (numBytesWritten) *numBytesWritten = numBytes;
    return kResultOk;
  }
  tresult seek(int64 p, int32 mode, int64* result) override {
    if (mode == 0) pos = p;
    else if (mode == 1) pos += p;
    else pos = (int64)buf.size() + p;
    if (pos < 0) pos = 0;
    if (result) *result = pos;
    return kResultOk;
  }
  tresult tell(int64* p) override { *p = pos; return kResultOk; }
  const uint8_t* data() const { return buf.data(); }
  size_t size() const { return buf.size(); }
  void setData(const uint8_t* d, size_t s) { buf.assign(d, d + s); pos = 0; }
};

/* --- Plugin handle --- */

struct vst3_plugin {
  void* lib;
  IPluginFactory* factory;
  IComponent* component;
  IAudioProcessor* processor;
  IEditController* controller;
  bool ctrlSeparate;
  HostContext* ctx;
  int32 inCh, outCh;
  double sampleRate;
  int32 blockSize;
  char name[64];
  char vendor[64];
#ifdef __APPLE__
  CFBundleRef bundle;
#endif
};

/* --- Helpers --- */

/* Find the shared library inside a .vst3 bundle */
static std::string findBinary(const char* bundlePath) {
  std::string base(bundlePath);
  while (!base.empty() && (base.back() == '/' || base.back() == '\\')) base.pop_back();

#ifdef _WIN32
  /* Windows: Contents/x86_64-win/*.vst3 */
  std::string archDir = base + "\\Contents\\x86_64-win";
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA((archDir + "\\*.vst3").c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) {
    std::string result = archDir + "\\" + fd.cFileName;
    FindClose(h);
    return result;
  }
  /* Flat .vst3 file (not a bundle) */
  return base;
#else
  /* macOS: Contents/MacOS/<binary> */
  std::string macosDir = base + "/Contents/MacOS";
  DIR* dir = opendir(macosDir.c_str());
  if (!dir) return "";
  std::string result;
  struct dirent* entry;
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') continue;
    result = macosDir + "/" + entry->d_name;
    break;
  }
  closedir(dir);
  return result;
#endif
}

static std::string u16to8(const char16* s, int maxLen = 128) {
  std::string r;
  for (int i = 0; i < maxLen && s[i]; i++) {
    char16 c = s[i];
    if (c < 0x80) r += (char)c;
    else if (c < 0x800) { r += (char)(0xC0 | (c >> 6)); r += (char)(0x80 | (c & 0x3F)); }
    else { r += (char)(0xE0 | (c >> 12)); r += (char)(0x80 | ((c >> 6) & 0x3F)); r += (char)(0x80 | (c & 0x3F)); }
  }
  return r;
}

/* --- C API --- */

extern "C" {

const char* vst3_get_error(void) { return s_error; }

vst3_plugin_t* vst3_open(const char* path, double sampleRate, int channels, int blockSize) {
  s_error[0] = 0;

  std::string dylib = findBinary(path);
  if (dylib.empty()) { set_error("No binary in bundle: ", path); return nullptr; }

  void* lib = lib_open(dylib.c_str());
  if (!lib) { set_error("dlopen failed: ", lib_error()); return nullptr; }

#ifdef __APPLE__
  CFBundleRef bundle = nullptr;
  {
    std::string bp(path);
    while (!bp.empty() && bp.back() == '/') bp.pop_back();
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
      kCFAllocatorDefault, (const UInt8*)bp.c_str(), bp.size(), true);
    if (url) { bundle = CFBundleCreate(kCFAllocatorDefault, url); CFRelease(url); }
  }
  typedef bool (*BundleEntryFn)(CFBundleRef);
  auto bundleEntry = (BundleEntryFn)lib_sym(lib,"bundleEntry");
  if (bundleEntry) bundleEntry(bundle);
#endif

  typedef IPluginFactory* (*GetFactoryFn)();
  auto getFactory = (GetFactoryFn)lib_sym(lib,"GetPluginFactory");
  if (!getFactory) { set_error("No GetPluginFactory"); lib_close(lib); return nullptr; }

  IPluginFactory* factory = getFactory();
  if (!factory) { set_error("GetPluginFactory returned null"); lib_close(lib); return nullptr; }

  PClassInfo ci;
  int found = -1;
  for (int32 i = 0; i < factory->countClasses(); i++) {
    factory->getClassInfo(i, &ci);
    if (strcmp(ci.category, "Audio Module Class") == 0) { found = i; break; }
  }
  if (found < 0) { set_error("No Audio Module Class"); factory->release(); lib_close(lib); return nullptr; }
  factory->getClassInfo(found, &ci);

  PFactoryInfo fi;
  factory->getFactoryInfo(&fi);

  IComponent* component = nullptr;
  if (factory->createInstance(ci.cid, IID_IComponent, (void**)&component) != kResultOk || !component) {
    set_error("createInstance IComponent failed");
    factory->release(); lib_close(lib); return nullptr;
  }

  HostContext* ctx = new HostContext();
  if (component->initialize((FUnknown*)ctx) != kResultOk) {
    set_error("IComponent::initialize failed");
    component->release(); factory->release(); lib_close(lib); delete ctx; return nullptr;
  }

  IAudioProcessor* processor = nullptr;
  qi((FUnknown*)component, UID_IAudioProcessor, (void**)&processor);
  if (!processor) {
    set_error("No IAudioProcessor");
    component->terminate(); component->release(); factory->release(); lib_close(lib); delete ctx; return nullptr;
  }

  /* Get IEditController — may be same object or separate class */
  IEditController* controller = nullptr;
  bool ctrlSeparate = false;
  if (qi((FUnknown*)component, UID_IEditController, (void**)&controller) != kResultOk || !controller) {
    TUID ctrlCid;
    if (component->getControllerClassId(ctrlCid) == kResultOk) {
      FUnknown* ctrlObj = nullptr;
      if (factory->createInstance(ctrlCid, IID_FUnknown, (void**)&ctrlObj) == kResultOk && ctrlObj) {
        qi(ctrlObj, UID_IEditController, (void**)&controller);
        if (controller) {
          ctrlSeparate = true;
          controller->initialize((FUnknown*)ctx);
          /* Connect component ↔ controller via IConnectionPoint */
          IConnectionPoint *compCP = nullptr, *ctrlCP = nullptr;
          qi((FUnknown*)component, UID_IConnectionPoint, (void**)&compCP);
          qi((FUnknown*)controller, UID_IConnectionPoint, (void**)&ctrlCP);
          if (compCP && ctrlCP) { compCP->connect(ctrlCP); ctrlCP->connect(compCP); }
          if (compCP) compCP->release();
          if (ctrlCP) ctrlCP->release();
        }
        ctrlObj->release();
      }
    }
  }

  ProcessSetup setup = { kRealtime, kSample32, blockSize, sampleRate };
  processor->setupProcessing(setup);

  int32 inCh = 0, outCh = 0;
  if (component->getBusCount(kAudio, kInput) > 0) {
    BusInfo bi; component->getBusInfo(kAudio, kInput, 0, bi);
    inCh = bi.channelCount;
    component->activateBus(kAudio, kInput, 0, true);
  }
  if (component->getBusCount(kAudio, kOutput) > 0) {
    BusInfo bi; component->getBusInfo(kAudio, kOutput, 0, bi);
    outCh = bi.channelCount;
    component->activateBus(kAudio, kOutput, 0, true);
  }
  if (inCh == 0) inCh = channels;
  if (outCh == 0) outCh = channels;

  component->setActive(true);
  processor->setProcessing(true);

  auto* p = new vst3_plugin();
  p->lib = lib; p->factory = factory; p->component = component;
  p->processor = processor; p->controller = controller; p->ctrlSeparate = ctrlSeparate;
  p->ctx = ctx; p->inCh = inCh; p->outCh = outCh;
  p->sampleRate = sampleRate; p->blockSize = blockSize;
  strncpy(p->name, ci.name, 63); p->name[63] = 0;
  strncpy(p->vendor, fi.vendor, 63); p->vendor[63] = 0;
#ifdef __APPLE__
  p->bundle = bundle;
#endif
  return (vst3_plugin_t*)p;
}

void vst3_close(vst3_plugin_t* handle) {
  if (!handle) return;
  auto* p = (vst3_plugin*)handle;
  if (!p->lib) return; /* already closed */

  if (p->processor) { p->processor->setProcessing(false); p->processor->release(); p->processor = nullptr; }
  if (p->component) { p->component->setActive(false); p->component->terminate(); p->component->release(); p->component = nullptr; }
  if (p->controller && p->ctrlSeparate) { p->controller->terminate(); p->controller->release(); }
  p->controller = nullptr;
  if (p->factory) { p->factory->release(); p->factory = nullptr; }

  typedef bool (*BundleExitFn)();
  auto bundleExit = (BundleExitFn)lib_sym(p->lib,"bundleExit");
  if (bundleExit) bundleExit();
  lib_close(p->lib);
  p->lib = nullptr;

#ifdef __APPLE__
  if (p->bundle) { CFRelease(p->bundle); p->bundle = nullptr; }
#endif
  if (p->ctx) { p->ctx->release(); p->ctx = nullptr; }
}

void vst3_destroy(vst3_plugin_t* handle) {
  if (!handle) return;
  vst3_close(handle);
  delete (vst3_plugin*)handle;
}

void vst3_process(vst3_plugin_t* handle, float** inputs, float** outputs, int numChannels, int numSamples) {
  auto* p = (vst3_plugin*)handle;
  if (!p || !p->processor) return;

  AudioBusBuffers inBus = { numChannels, 0, {} };
  inBus.channelBuffers32 = inputs;
  AudioBusBuffers outBus = { numChannels, 0, {} };
  outBus.channelBuffers32 = outputs;

  ProcessData data = {};
  data.processMode = kRealtime;
  data.symbolicSampleSize = kSample32;
  data.numSamples = numSamples;
  data.numInputs = inputs ? 1 : 0;
  data.numOutputs = 1;
  data.inputs = inputs ? &inBus : nullptr;
  data.outputs = &outBus;
  data.inputParameterChanges = nullptr;
  data.outputParameterChanges = nullptr;
  data.inputEvents = nullptr;
  data.outputEvents = nullptr;

  p->processor->process(data);
}

const char* vst3_get_name(vst3_plugin_t* h) { return h ? ((vst3_plugin*)h)->name : ""; }
const char* vst3_get_vendor(vst3_plugin_t* h) { return h ? ((vst3_plugin*)h)->vendor : ""; }
int vst3_get_channels(vst3_plugin_t* h, int dir) {
  if (!h) return 0;
  return dir == 0 ? ((vst3_plugin*)h)->inCh : ((vst3_plugin*)h)->outCh;
}

int vst3_get_param_count(vst3_plugin_t* h) {
  auto* p = (vst3_plugin*)h;
  return (p && p->controller) ? p->controller->getParameterCount() : 0;
}

int vst3_get_param_info(vst3_plugin_t* h, int index, vst3_param_info_t* out) {
  auto* p = (vst3_plugin*)h;
  if (!p || !p->controller) return -1;
  ParameterInfo info;
  if (p->controller->getParameterInfo(index, info) != kResultOk) return -1;
  out->id = info.id;
  std::string name = u16to8(info.title);
  strncpy(out->name, name.c_str(), 255); out->name[255] = 0;
  out->defaultValue = info.defaultNormalizedValue;
  out->stepCount = info.stepCount;
  out->min = p->controller->normalizedParamToPlain(info.id, 0.0);
  out->max = p->controller->normalizedParamToPlain(info.id, 1.0);
  return 0;
}

double vst3_get_param(vst3_plugin_t* h, uint32_t id) {
  auto* p = (vst3_plugin*)h;
  return (p && p->controller) ? p->controller->getParamNormalized(id) : 0;
}

void vst3_set_param(vst3_plugin_t* h, uint32_t id, double value) {
  auto* p = (vst3_plugin*)h;
  if (p && p->controller) p->controller->setParamNormalized(id, value);
}

int vst3_get_state(vst3_plugin_t* h, void** data, int* size) {
  auto* p = (vst3_plugin*)h;
  if (!p || !p->component) return -1;
  auto* s = new MemStream();
  if (p->component->getState(s) != kResultOk) { s->release(); return -1; }
  *size = (int)s->size();
  *data = malloc(*size);
  memcpy(*data, s->data(), *size);
  s->release();
  return 0;
}

int vst3_set_state(vst3_plugin_t* h, const void* data, int size) {
  auto* p = (vst3_plugin*)h;
  if (!p || !p->component) return -1;
  auto* s = new MemStream();
  s->setData((const uint8_t*)data, size);
  tresult r = p->component->setState(s);
  s->release();
  return r == kResultOk ? 0 : -1;
}

void vst3_free_state(void* data) { free(data); }

} /* extern "C" */
