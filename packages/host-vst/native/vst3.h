/*
 * vst3.h — Minimal VST3 COM interface definitions for plugin hosting
 *
 * ABI-compatible reimplementation of Steinberg VST3 interfaces.
 * https://github.com/steinbergmedia/vst3sdk
 */

#pragma once
#include <cstdint>
#include <cstring>

namespace Steinberg {

typedef int32_t tresult;
typedef int8_t int8;
typedef int32_t int32;
typedef int64_t int64;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef char16_t char16;
typedef int8 TUID[16];
typedef char16 String128[128];

enum { kResultOk = 0, kResultFalse = 1, kInvalidArgument = 2,
       kNotImplemented = 3, kInternalError = 4, kNotInitialized = 5 };

/* Build TUID at runtime — non-COM (Steinberg SDK) or COM (JUCE) byte order */
static inline void make_tuid(TUID out, uint32_t l1, uint32_t l2, uint32_t l3, uint32_t l4, bool com) {
  if (!com) {
    out[0]=(int8)(l1>>24); out[1]=(int8)(l1>>16); out[2]=(int8)(l1>>8); out[3]=(int8)l1;
    out[4]=(int8)(l2>>24); out[5]=(int8)(l2>>16); out[6]=(int8)(l2>>8); out[7]=(int8)l2;
  } else {
    out[0]=(int8)l1; out[1]=(int8)(l1>>8); out[2]=(int8)(l1>>16); out[3]=(int8)(l1>>24);
    out[4]=(int8)(l2>>16); out[5]=(int8)(l2>>24); out[6]=(int8)l2; out[7]=(int8)(l2>>8);
  }
  out[8]=(int8)(l3>>24); out[9]=(int8)(l3>>16); out[10]=(int8)(l3>>8); out[11]=(int8)l3;
  out[12]=(int8)(l4>>24); out[13]=(int8)(l4>>16); out[14]=(int8)(l4>>8); out[15]=(int8)l4;
}

/* Match TUID against 4x uint32 in either byte order */
static inline bool tuid_match(const TUID a, uint32_t l1, uint32_t l2, uint32_t l3, uint32_t l4) {
  TUID b;
  make_tuid(b, l1, l2, l3, l4, false);
  if (memcmp(a, b, 16) == 0) return true;
  make_tuid(b, l1, l2, l3, l4, true);
  return memcmp(a, b, 16) == 0;
}

/* Well-known interface IDs (byte-order-independent form) */
#define UID_FUnknown          0x00000000, 0x00000000, 0xC0000000, 0x00000046
#define UID_IComponent        0xE831FF31, 0xF2D54301, 0x928EBBEE, 0x25697802
#define UID_IAudioProcessor   0x42043F99, 0xB7DA453C, 0xA569E79D, 0x9AAEC33D
#define UID_IEditController   0xDCD7BBE3, 0x7742448D, 0xA874AACC, 0x979C759E
#define UID_IConnectionPoint  0x70A4156F, 0x6E6E4026, 0x989148BF, 0xAA60D8D1
#define UID_IHostApplication  0x58E595CC, 0xDB2D4369, 0xA2BF7201, 0x1B2B26B2
#define UID_IBStream          0xC3BF6EA2, 0x30994523, 0x9124FE83, 0x0C727026

/* --- Base interfaces --- */

class FUnknown {
public:
  virtual tresult queryInterface(const TUID _iid, void** obj) = 0;
  virtual uint32 addRef() = 0;
  virtual uint32 release() = 0;
};

/* queryInterface trying both byte orders (Steinberg SDK vs JUCE) */
static inline tresult qi(FUnknown* obj, uint32_t l1, uint32_t l2, uint32_t l3, uint32_t l4, void** out) {
  TUID id;
  make_tuid(id, l1, l2, l3, l4, false);
  tresult r = obj->queryInterface(id, out);
  if (r == kResultOk && *out) return r;
  make_tuid(id, l1, l2, l3, l4, true);
  return obj->queryInterface(id, out);
}

class IBStream : public FUnknown {
public:
  virtual tresult read(void* buffer, int32 numBytes, int32* numBytesRead) = 0;
  virtual tresult write(void* buffer, int32 numBytes, int32* numBytesWritten) = 0;
  virtual tresult seek(int64 pos, int32 mode, int64* result) = 0;
  virtual tresult tell(int64* pos) = 0;
};

class IPluginBase : public FUnknown {
public:
  virtual tresult initialize(FUnknown* context) = 0;
  virtual tresult terminate() = 0;
};

/* --- Factory --- */

struct PClassInfo {
  TUID cid;
  int32 cardinality;
  char category[32];
  char name[64];
};

struct PFactoryInfo {
  char vendor[64];
  char url[256];
  char email[128];
  int32 flags;
};

class IPluginFactory : public FUnknown {
public:
  virtual tresult getFactoryInfo(PFactoryInfo* info) = 0;
  virtual int32 countClasses() = 0;
  virtual tresult getClassInfo(int32 index, PClassInfo* info) = 0;
  virtual tresult createInstance(const TUID cid, const TUID iid, void** obj) = 0;
};

/* --- VST-specific --- */

namespace Vst {

enum MediaTypes { kAudio = 0, kEvent = 1 };
enum BusDirections { kInput = 0, kOutput = 1 };
enum { kSample32 = 0, kSample64 = 1, kRealtime = 0 };
typedef uint32 ParamID;
typedef double ParamValue;

struct BusInfo {
  int32 mediaType;
  int32 direction;
  int32 channelCount;
  String128 name;
  int32 busType;
  uint32 flags;
};

struct ProcessSetup {
  int32 processMode;
  int32 symbolicSampleSize;
  int32 maxSamplesPerBlock;
  double sampleRate;
};

struct AudioBusBuffers {
  int32 numChannels;
  uint64 silenceFlags;
  union {
    float** channelBuffers32;
    double** channelBuffers64;
  };
};

struct ProcessData {
  int32 processMode;
  int32 symbolicSampleSize;
  int32 numSamples;
  int32 numInputs;
  int32 numOutputs;
  AudioBusBuffers* inputs;
  AudioBusBuffers* outputs;
  void* inputParameterChanges;
  void* outputParameterChanges;
  void* inputEvents;
  void* outputEvents;
  void* processContext;
};

struct ParameterInfo {
  ParamID id;
  String128 title;
  String128 shortTitle;
  String128 units;
  int32 stepCount;
  ParamValue defaultNormalizedValue;
  int32 unitId;
  int32 flags;
};

class IComponent : public IPluginBase {
public:
  virtual tresult getControllerClassId(TUID classId) = 0;
  virtual tresult setIoMode(int32 mode) = 0;
  virtual int32 getBusCount(int32 type, int32 dir) = 0;
  virtual tresult getBusInfo(int32 type, int32 dir, int32 index, BusInfo& bus) = 0;
  virtual tresult getRoutingInfo(void* inInfo, void* outInfo) = 0;
  virtual tresult activateBus(int32 type, int32 dir, int32 index, bool state) = 0;
  virtual tresult setActive(bool state) = 0;
  virtual tresult setState(IBStream* state) = 0;
  virtual tresult getState(IBStream* state) = 0;
};

class IAudioProcessor : public FUnknown {
public:
  virtual tresult setBusArrangements(uint64* inputs, int32 numIns, uint64* outputs, int32 numOuts) = 0;
  virtual tresult getBusArrangement(int32 dir, int32 index, uint64& arr) = 0;
  virtual tresult canProcessSampleSize(int32 symbolicSampleSize) = 0;
  virtual uint32 getLatencySamples() = 0;
  virtual tresult setupProcessing(ProcessSetup& setup) = 0;
  virtual tresult setProcessing(bool state) = 0;
  virtual tresult process(ProcessData& data) = 0;
  virtual uint32 getTailSamples() = 0;
};

class IEditController : public IPluginBase {
public:
  virtual tresult setComponentState(IBStream* state) = 0;
  virtual tresult setState(IBStream* state) = 0;
  virtual tresult getState(IBStream* state) = 0;
  virtual int32 getParameterCount() = 0;
  virtual tresult getParameterInfo(int32 paramIndex, ParameterInfo& info) = 0;
  virtual tresult getParamStringByValue(ParamID id, ParamValue valueNormalized, String128 string) = 0;
  virtual tresult getParamValueByString(ParamID id, char16* string, ParamValue& valueNormalized) = 0;
  virtual ParamValue normalizedParamToPlain(ParamID id, ParamValue valueNormalized) = 0;
  virtual ParamValue plainParamToNormalized(ParamID id, ParamValue plainValue) = 0;
  virtual ParamValue getParamNormalized(ParamID id) = 0;
  virtual tresult setParamNormalized(ParamID id, ParamValue value) = 0;
  virtual tresult setComponentHandler(FUnknown* handler) = 0;
  virtual void* createView(const char* name) = 0;
};

class IConnectionPoint : public FUnknown {
public:
  virtual tresult connect(IConnectionPoint* other) = 0;
  virtual tresult disconnect(IConnectionPoint* other) = 0;
  virtual tresult notify(void* message) = 0;
};

class IHostApplication : public FUnknown {
public:
  virtual tresult getName(String128 name) = 0;
  virtual tresult createInstance(const TUID cid, const TUID iid, void** obj) = 0;
};

} // namespace Vst
} // namespace Steinberg
