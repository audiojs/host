# audio-host — Implementation Plan

> Universal plugin host: any plugin format, anywhere — desktop, Node.js, browser.
> Monorepo: `audio-host` (core) + `@audio/host-clap`, `@audio/host-vst`, `@audio/host-lv2`, `@audio/host-au`, `@audio/host-wam`.
> Each format host is a separate package in `packages/`. Core provides unified interface + `.host` format.
> Architecture: C adapter per format → NAPI (Node.js) → WASM (browser) → AudioWorklet.

---

## Package Structure

```
audio-host/
├── package.json              # workspaces: ["packages/*"]
├── packages/
│   ├── audio-host/           # core: unified API, .host format, chain engine
│   │   ├── src/
│   │   ├── browser.js
│   │   └── package.json      # `audio-host` — aggregates format hosts
│   ├── host-clap/            # @audio/host-clap
│   │   ├── src/              # C: CLAP adapter → process(in, out, params)
│   │   ├── binding.gyp
│   │   ├── browser.js        # WASM build
│   │   └── package.json
│   ├── host-vst/             # @audio/host-vst
│   │   ├── src/              # C++: VST3 adapter
│   │   ├── binding.gyp
│   │   └── package.json
│   ├── host-lv2/             # @audio/host-lv2
│   │   ├── src/              # C: LV2 adapter
│   │   ├── binding.gyp
│   │   └── package.json
│   ├── host-au/              # @audio/host-au
│   │   ├── src/              # ObjC: AU adapter (macOS/iOS only)
│   │   ├── binding.gyp
│   │   └── package.json
│   └── host-wam/             # @audio/host-wam
│       ├── src/              # JS: WAM ↔ unified interface bridge
│       └── package.json
```

---

## Phase 1: Unified Plugin Interface (core `audio-host`)

The contract every format adapter must implement.

- [ ] Define `PluginAdapter` interface in C header:
  - `discover(path) → plugin_descriptor[]`
  - `load(id) → plugin_handle`
  - `activate(handle, sample_rate, block_size=128)`
  - `process(handle, inputs, outputs, params, events)`
  - `get_params(handle) → param_descriptor[]`
  - `get_state(handle) → blob` / `set_state(handle, blob)`
  - `deactivate(handle)` / `destroy(handle)`
- [ ] Define JS `PluginNode` interface (mirrors `AudioWorkletProcessor.process()`)
- [ ] Define plugin descriptor schema (id, name, vendor, format, inputs, outputs, params)
- [ ] Scaffold monorepo: root `package.json` with workspaces, `packages/` dirs

## Phase 2: `@audio/host-clap`

First format — simplest, pure C, compiles to WASM.

- [ ] CLAP plugin discovery (scan platform paths, read descriptors)
- [ ] CLAP plugin loading (dlopen/LoadLibrary, entry point, factory)
- [ ] Plugin lifecycle (create, activate, deactivate, destroy)
- [ ] Audio processing (128-sample blocks, map to unified `process()`)
- [ ] Parameter enumeration → unified param descriptors
- [ ] Parameter get/set with thread safety
- [ ] State save/restore (opaque blob)
- [ ] MIDI event input (note on/off, CC, pitch bend → CLAP events)
- [ ] Plugin isolation (subprocess or signal handler — crash ≠ host crash)
- [ ] NAPI binding (`binding.gyp`, exposes `ClapHost` class)
- [ ] Prebuilt binaries: `@audio/host-clap-darwin-x64`, `-arm64`, `-linux-x64`, `-linux-arm64`, `-win32-x64`
- [ ] WASM build (Emscripten) — for browser path
- [ ] `browser.js` conditional export
- [ ] Test: load CLAP synth → `process()` → verify audio output

## Phase 3: Core `audio-host` — Chain Engine + `.host` Format

Aggregates format hosts, provides chain routing and project files.

- [ ] Auto-detect installed format packages (`@audio/host-clap`, etc.)
- [ ] Unified `scan()` — delegates to each format host's `discover()`
- [ ] Unified `load(pluginId)` — routes to correct format host by plugin URI
- [ ] Chain engine: directed graph of plugin nodes
  - [ ] Serial routing (A → B → C)
  - [ ] Parallel routing (A → [B, C] → D)
  - [ ] Split/merge
- [ ] `.host` JSON format:
  - [ ] Plugin references by URI (not path) — resolved per-platform
  - [ ] Parameter values as plain numbers
  - [ ] Plugin opaque state as base64
  - [ ] Chain topology as adjacency list
  - [ ] MIDI routing map (controller → param bindings)
  - [ ] Scenes/presets (named state snapshots)
- [ ] `host.load(file)` / `host.save(file)` API
- [ ] AudioWorklet integration: chain as `AudioWorkletProcessor` in `web-audio-api`
- [ ] Test: save on macOS → load on Linux → identical output

## Phase 4: `@audio/host-vst`

Professional standard. C++ at boundary, unified C interface exposed.

- [ ] Integrate VST3 SDK (vendor as submodule or bundled headers)
- [ ] VST3 discovery + loading → unified descriptors
- [ ] VST3 parameters → unified param interface
- [ ] VST3 audio processing (128-sample blocks)
- [ ] VST3 state save/restore
- [ ] VST3 MIDI / note expression
- [ ] NAPI binding + prebuilt binaries
- [ ] Test: mixed CLAP + VST3 chain in one `.host` file

## Phase 5: `@audio/host-lv2`

Linux ecosystem, open standard.

- [ ] LV2 discovery (lilv or direct header scan)
- [ ] LV2 loading + port mapping (audio, control, atom → unified interface)
- [ ] LV2 audio processing
- [ ] LV2 state save/restore (State extension)
- [ ] NAPI binding + prebuilt binaries
- [ ] Test: CLAP + VST3 + LV2 mixed chain

## Phase 6: Browser Path (WASM + AudioWorklet)

Desktop plugins in the browser — the differentiator.

- [ ] `@audio/host-clap` WASM: C host compiled via Emscripten
- [ ] CLAP plugins compiled to WASM loadable by URL
- [ ] AudioWorklet wrapper: WASM host inside `AudioWorkletProcessor.process()`
- [ ] Core `audio-host` browser entry: chain engine in JS, delegates to WASM format hosts
- [ ] Plugin loading from URL (fetch `.wasm` → instantiate in host)
- [ ] Parameter control: main thread ↔ AudioWorklet via `MessagePort`
- [ ] SharedArrayBuffer fast path when available
- [ ] Test: CLAP-WASM synth in browser → Web Audio API → speakers
- [ ] Benchmark: WASM vs native (<2x overhead target)

## Phase 7: `@audio/host-wam`

Bidirectional bridge — WAM plugins in desktop, host plugins as WAMs.

- [ ] WAM → host: load WAM descriptor, wrap as unified `PluginNode`
- [ ] Host → WAM: expose any loaded plugin as WAM instance (WAM API)
- [ ] Parameter mapping (WAM params ↔ AudioParam ↔ unified params)
- [ ] WamEnv / WamGroup lifecycle integration
- [ ] Mixed chain: CLAP + WAM in same `.host` file
- [ ] Test: CLAP effect + WAM synth → audio output in browser

## Phase 8: `@audio/host-au`

Apple ecosystem — macOS/iOS only, optional dependency.

- [ ] AUv3 integration (AUAudioUnit, ObjC bridge)
- [ ] AU discovery + loading → unified interface
- [ ] AU parameters + state
- [ ] NAPI binding (macOS prebuilts only)
- [ ] Test: CLAP + VST3 + AU chain on macOS

## Phase 9: Ship

- [ ] CLI: `npx audio-host play file.host`
- [ ] Plugin GUI forwarding (optional — plugin UI in separate window)
- [ ] Error reporting: clear messages on format/plugin failures
- [ ] Docs + examples
- [ ] npm publish all packages
- [ ] Integration examples: `audio-mic → audio-host → audio-speaker`

---

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                      .host file                          │
│            (JSON: plugins, chain, state)                 │
└────────────────────────┬─────────────────────────────────┘
                         │
              ┌──────────▼──────────┐
              │    audio-host       │
              │  (chain engine,     │
              │   format router,    │
              │   .host format)     │
              └──────────┬──────────┘
                         │ discovers installed @audio/host-* packages
       ┌─────────┬───────┼────────┬──────────┐
       │         │       │        │          │
  ┌────▼───┐ ┌──▼──┐ ┌──▼──┐ ┌──▼──┐  ┌────▼───┐
  │host-   │ │host-│ │host-│ │host-│  │host-   │
  │clap    │ │vst  │ │lv2  │ │au   │  │wam     │
  │(C/WASM)│ │(C++)│ │(C)  │ │(ObjC)│ │(JS)    │
  └────┬───┘ └──┬──┘ └──┬──┘ └──┬──┘  └────┬───┘
       └────────┴───────┴───────┘           │
          Unified Plugin Interface          │
        process(in, out, params)            │
                    │                       │
         ┌──────────┴──────────┐            │
         │                     │            │
    ┌────▼─────┐        ┌─────▼────┐  ┌────▼────┐
    │ Node.js  │        │ Browser  │  │ Browser │
    │  (NAPI)  │        │ (WASM)   │  │ (WAM)   │
    └────┬─────┘        └─────┬────┘  └────┬────┘
         │                    └─────┬──────┘
    ┌────▼─────┐              ┌─────▼──────┐
    │web-audio │              │ Web Audio  │
    │-api node │              │ AudioWorklet│
    │(Worklet) │              │            │
    └──────────┘              └────────────┘
```

## Key Design Decisions

1. **Monorepo, separate packages** — Each format is `@audio/host-{format}`. Install only what you need. Core `audio-host` auto-discovers installed format packages.

2. **128-sample blocks** — `process(inputs, outputs, parameters)` at BLOCK_SIZE=128. Matches `AudioWorkletProcessor.process()`. Universal contract.

3. **C adapter per format** — Each package wraps its format's native API into the unified C interface. VST3's C++ stays inside `host-vst`. CLAP's C stays inside `host-clap`. Core only sees the unified interface.

4. **CLAP-first for browser** — Pure C → WASM via Emscripten. First and cleanest browser path. Other formats follow if their SDKs permit WASM compilation.

5. **`.host` = portable project** — JSON, human-readable, git-diffable. Plugin URIs, not paths. The format survives 10 years.

6. **WAM is bidirectional** — `host-wam` both consumes WAM plugins in desktop chains AND exposes desktop plugins as WAMs in browser. Two worlds, one chain.
