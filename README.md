# @audio/host

> Load any audio plugin. Process any audio. No DAW required.

Host VST3 and CLAP plugins from JavaScript — as native modules or [Web Audio](https://github.com/audiojs/web-audio-api) nodes. Format is transparent: point at a file, get audio out. Part of [audiojs](https://github.com/audiojs).

```js
import { load } from '@audio/host'

const reverb = load('reverb.vst3')
const gain = load('gain.clap')

const output = reverb.processAll([left, right])
```

## Features

- **Format-transparent** — `@audio/host` detects `.vst3` / `.clap` and routes to the right backend
- **AudioWorklet** — register any plugin as `AudioWorkletProcessor`, chain with other nodes
- **Zero config** — point at a plugin file, get audio out
- **Parameters** — enumerate, read, write plugin parameters
- **State** — save/restore plugin state as binary blob
- **Cross-platform** — macOS, Linux, Windows via prebuilt binaries
- **128-sample blocks** — matches Web Audio `BLOCK_SIZE` exactly, zero reblocking

## Install

```sh
npm install @audio/host         # universal — installs available format hosts
```

Or install format hosts individually:

```sh
npm install @audio/host-vst    # VST3 only
npm install @audio/host-clap   # CLAP only
```

Prebuilt native binaries install automatically for your platform. Falls back to local `node-gyp` compilation if unavailable.

## Usage

### Discover plugins

```js
import { scan } from '@audio/host'

const plugins = scan()
// [
//   { path: '/Library/Audio/.../REV-X_HALL.vst3', name: 'REV-X Hall', vendor: 'Yamaha Corporation', format: 'vst3' },
//   { path: '/Library/Audio/.../gain.clap', name: 'Test Gain', vendor: 'audiojs', format: 'clap' },
//   ...
// ]

// Or scan specific directories
scan(['/my/custom/plugin/dir'])
```

Scan probes each plugin in a **subprocess** — a crashed or misbehaving plugin won't take down your process.

### Load and process

```js
import { load } from '@audio/host'

const plugin = load('/path/to/plugin.vst3', {
  sampleRate: 44100,  // default
  channels: 2,        // default
  blockSize: 128      // default — matches Web Audio BLOCK_SIZE
})

console.log(plugin.name)    // 'REV-X Hall'
console.log(plugin.vendor)  // 'Yamaha Corporation'

// Process full buffers (handles block splitting internally)
const output = plugin.processAll([leftChannel, rightChannel])

// Or process one block at a time
plugin.process([inL, inR], [outL, outR])

plugin.close()
```

### Parameters

```js
const params = plugin.params
// [{ id: 84, name: 'Threshold', min: 0, max: 1, defaultValue: 0 }, ...]

plugin.setParam(84, 0.5)
plugin.getParam(84)  // 0.5
```

### AudioWorklet (Web Audio API)

Any plugin as a standard `AudioWorkletNode`:

```js
import { AudioContext, AudioWorkletNode, AudioWorkletProcessor } from 'web-audio-api'
import { register } from '@audio/host'

const ctx = new AudioContext()

await ctx.audioWorklet.addModule(register('reverb.vst3', AudioWorkletProcessor))
await ctx.audioWorklet.addModule(register('gain.clap', AudioWorkletProcessor))

const src = ctx.createBufferSource()
const reverb = new AudioWorkletNode(ctx, 'REV-X Hall')
const gain = new AudioWorkletNode(ctx, 'Test Gain')

src.connect(reverb).connect(gain).connect(ctx.destination)
src.start()
```

### Process an audio file

```js
import { readFileSync } from 'fs'
import decode from '@audio/decode'
import speaker from '@audio/speaker'
import { load } from '@audio/host'

const audio = await decode(readFileSync('input.wav'))
const plugin = load('plugin.vst3', { sampleRate: audio.sampleRate })
const output = plugin.processAll(audio.channelData)
plugin.close()

// Play result
const write = speaker({ sampleRate: audio.sampleRate, channels: 2, bitDepth: 32 })
const buf = Buffer.alloc(output[0].length * 2 * 4)
for (let i = 0; i < output[0].length; i++)
  for (let c = 0; c < 2; c++)
    buf.writeFloatLE(output[c][i], (i * 2 + c) * 4)
write(buf, () => write.flush(() => write.close()))
```

## Packages

| Package | Description |
|---------|-------------|
| `@audio/host` | Universal entry — auto-detects format, routes to installed host |
| `@audio/host-vst` | VST3 host (C++) |
| `@audio/host-clap` | CLAP host (C) |

Prebuilt binaries per platform (installed automatically via `optionalDependencies`):

`@audio/host-{vst,clap}-{darwin-arm64,darwin-x64,linux-x64,linux-arm64,win32-x64}`

## API

### `scan(dirs?) → plugin[]`

Scan standard system paths (or custom `dirs`) for installed plugins. Each plugin is probed in a subprocess — crashes are isolated. Returns `[{ path, name, vendor, format }]`.

### `load(path, opts?) → plugin`

Load a plugin. Format detected from file extension.

| Option | Default | Description |
|--------|---------|-------------|
| `sampleRate` | `44100` | Sample rate in Hz |
| `channels` | `2` | Number of audio channels |
| `blockSize` | `128` | Samples per process block |

Returns:

| Property / Method | Description |
|-------------------|-------------|
| `name` | Plugin name |
| `vendor` | Plugin vendor |
| `blockSize` | Block size used |
| `params` | `[{ id, name, min, max, defaultValue }]` |
| `getParam(id)` | Get normalized parameter value |
| `setParam(id, val)` | Set normalized parameter value |
| `process(in, out)` | Process one block — arrays of `Float32Array` |
| `processAll(channels)` | Process full buffers — returns `Float32Array[]` |
| `getState()` | Plugin state as `Buffer` |
| `setState(buf)` | Restore plugin state |
| `close()` | Release resources |

### `register(path, AudioWorkletProcessor, opts?) → fn`

Register plugin as `AudioWorkletProcessor` for use with `addModule`:

```js
await ctx.audioWorklet.addModule(register(path, AudioWorkletProcessor))
const node = new AudioWorkletNode(ctx, pluginName)
```

### `formats`

Array of supported extensions based on installed packages, e.g. `['.vst3', '.clap']`.

## FAQ

**Where are my plugins?**

| OS | VST3 | CLAP |
|----|------|------|
| macOS | `/Library/Audio/Plug-Ins/VST3/` | `/Library/Audio/Plug-Ins/CLAP/` |
| Linux | `~/.vst3/` or `/usr/lib/vst3/` | `~/.clap/` or `/usr/lib/clap/` |
| Windows | `C:\Program Files\Common Files\VST3\` | `C:\Program Files\Common Files\CLAP\` |

**Can I use this in the browser?**

Not for native plugins — they're compiled machine code. CLAP-to-WASM is planned (CLAP is pure C, compiles via Emscripten). WAM support planned as `@audio/host-wam`.

**Does a crashed plugin crash my process?**

Currently yes. Plugin isolation is planned.

**What about GUIs?**

Not supported. This is a headless host. Use a DAW for plugin UIs.

**What about MIDI?**

Planned — note on/off, CC, pitch bend routed to plugin event input.

## Pitfalls

- **Plugins are platform-native.** A macOS `.vst3` won't run on Windows. Each platform needs its own plugins installed.
- **Some plugins need activation.** Licensed plugins (Waves, iZotope) may fail without auth. Free plugins (Airwindows, Bertom, TDR) work directly.
- **Null parameter changes.** Some plugins dereference `inputParameterChanges` without null-checking. If a plugin crashes on `process()`, this may be why.
- **State is opaque.** `getState()` returns the plugin's internal binary format, not portable across versions.

## License

MIT

---

> *"I am the ability in man."* — Bhagavad Gita 7.8
