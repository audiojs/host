// Host-owned VST3 parameter queues. setParam writes one point at the next block's
// start; repeated writes to one parameter coalesce to the latest value.
#pragma once
#include "vst3.h"
#include <vector>

class ParameterChanges : public Steinberg::Vst::IParameterChanges {
  using ParamID = Steinberg::Vst::ParamID;
  using ParamValue = Steinberg::Vst::ParamValue;
  using int32 = Steinberg::int32;
  using uint32 = Steinberg::uint32;
  using tresult = Steinberg::tresult;
  using TUID = Steinberg::TUID;
  class Queue final : public Steinberg::Vst::IParamValueQueue {
  public:
    ParamID id;
    ParamValue value = 0;
    explicit Queue(ParamID id) : id(id) {}
    tresult queryInterface(const TUID iid, void** obj) override {
      *obj = nullptr;
      if (!Steinberg::tuid_match(iid, UID_FUnknown) && !Steinberg::tuid_match(iid, UID_IParamValueQueue)) return Steinberg::kResultFalse;
      *obj = this; return Steinberg::kResultOk;
    }
    uint32 addRef() override { return 1; }
    uint32 release() override { return 1; }
    ParamID getParameterId() override { return id; }
    int32 getPointCount() override { return 1; }
    tresult getPoint(int32 index, int32& offset, ParamValue& out) override {
      if (index) return Steinberg::kInvalidArgument;
      offset = 0; out = value; return Steinberg::kResultOk;
    }
    tresult addPoint(int32 offset, ParamValue v, int32& index) override {
      if (offset) return Steinberg::kInvalidArgument;
      value = v; index = 0; return Steinberg::kResultOk;
    }
  };
  std::vector<Queue> queues;
public:
  tresult queryInterface(const TUID iid, void** obj) override {
    *obj = nullptr;
    if (!Steinberg::tuid_match(iid, UID_FUnknown) && !Steinberg::tuid_match(iid, UID_IParameterChanges)) return Steinberg::kResultFalse;
    *obj = this; return Steinberg::kResultOk;
  }
  uint32 addRef() override { return 1; }
  uint32 release() override { return 1; }
  int32 getParameterCount() override { return (int32)queues.size(); }
  Steinberg::Vst::IParamValueQueue* getParameterData(int32 index) override {
    return index >= 0 && index < (int32)queues.size() ? &queues[index] : nullptr;
  }
  Steinberg::Vst::IParamValueQueue* addParameterData(const ParamID& id, int32& index) override {
    for (index = 0; index < (int32)queues.size(); index++)
      if (queues[index].id == id) return &queues[index];
    queues.emplace_back(id);
    return &queues.back();
  }
  void set(ParamID id, ParamValue value) {
    int32 index;
    addParameterData(id, index)->addPoint(0, value, index);
  }
  // Keeps capacity: processing consumes updates without allocating or freeing.
  void clear() { queues.clear(); }
};
