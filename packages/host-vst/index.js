/**
 * @module @audio/host-vst
 * VST3 plugin host for audiojs
 */
import addon from './src/addon.js'
import { readdirSync, statSync } from 'fs'
import { join } from 'path'
import { execFileSync } from 'child_process'

export function load(path, opts = {}) {
  const { sampleRate = 44100, channels = 2, blockSize = 128 } = opts
  const handle = addon.open(path, sampleRate, channels, blockSize)
  let closed = false

  return {
    get name() { return addon.getName(handle) },
    get vendor() { return addon.getVendor(handle) },
    get inputChannels() { return addon.getChannels(handle, 0) },
    get outputChannels() { return addon.getChannels(handle, 1) },
    blockSize,

    get params() {
      const n = addon.getParamCount(handle)
      const out = []
      for (let i = 0; i < n; i++) {
        const info = addon.getParamInfo(handle, i)
        if (info) out.push(info)
      }
      return out
    },

    getParam(id) { return addon.getParam(handle, id) },
    setParam(id, value) { addon.setParam(handle, id, value) },

    process(inputs, outputs) { addon.process(handle, inputs, outputs) },
    processAll(ch) { return processBlocks(addon, handle, ch, blockSize) },

    getState() { return addon.getState(handle) },
    setState(buf) { addon.setState(handle, buf) },

    close() {
      if (closed) return
      closed = true
      addon.close(handle)
    }
  }
}

/**
 * Scan system for installed VST3 plugins.
 * Returns array of { path, name, vendor, format } for each loadable plugin.
 */
export function scan(dirs) {
  return scanDir(dirs || defaultPaths({
    darwin: ['/Library/Audio/Plug-Ins/VST3', '$HOME/Library/Audio/Plug-Ins/VST3'],
    linux: ['$HOME/.vst3', '/usr/lib/vst3', '/usr/local/lib/vst3'],
    win32: ['$PROGRAMFILES/Common Files/VST3'],
  }), '.vst3', 'vst3')
}

/**
 * Register a VST3 plugin as an AudioWorkletProcessor.
 *
 *   import { AudioWorkletProcessor } from 'web-audio-api'
 *   await ctx.audioWorklet.addModule(register(path, AudioWorkletProcessor))
 *   const node = new AudioWorkletNode(ctx, 'Plugin Name')
 */
export function register(path, BaseClass, opts = {}) {
  const probe = load(path, { ...opts, sampleRate: opts.sampleRate || 44100 })
  const name = probe.name
  const descriptors = probe.params.map(p => ({
    name: p.name,
    defaultValue: p.defaultValue,
    minValue: p.min,
    maxValue: p.max,
    automationRate: 'k-rate'
  }))
  probe.close()

  return function(scope) {
    class PluginProcessor extends BaseClass {
      static get parameterDescriptors() { return descriptors }

      constructor(options) {
        super(options)
        this._handle = addon.open(path, scope.sampleRate,
          opts.channels || 2, opts.blockSize || 128)
      }

      process(inputs, outputs) {
        const input = inputs[0], output = outputs[0]
        if (!output || !output.length) return true
        addon.process(this._handle, input.length ? input : null, output)
        return true
      }
    }

    scope.registerProcessor(name, PluginProcessor)
  }
}

function defaultPaths(map) {
  const plat = process.platform
  return (map[plat] || []).map(p =>
    p.replace('$HOME', process.env.HOME || process.env.USERPROFILE || '')
     .replace('$PROGRAMFILES', process.env.PROGRAMFILES || 'C:\\Program Files'))
}

function scanDir(dirs, ext, format) {
  /* Collect all plugin paths */
  const paths = []
  for (const dir of dirs) {
    let entries
    try { entries = readdirSync(dir) } catch { continue }
    for (const name of entries) {
      const full = join(dir, name)
      if (name.endsWith(ext)) { paths.push(full); continue }
      try {
        if (statSync(full).isDirectory())
          for (const sub of readdirSync(full))
            if (sub.endsWith(ext)) paths.push(join(full, sub))
      } catch {}
    }
  }

  /* Probe each in a subprocess — isolates crashes and ObjC class conflicts */
  const results = []
  for (const path of paths) {
    try {
      const code = `import{load}from'${import.meta.url}';try{const p=load(${JSON.stringify(path)});console.log(JSON.stringify({path:${JSON.stringify(path)},name:p.name,vendor:p.vendor,format:'${format}'}));p.close()}catch{}`
      const out = execFileSync(process.execPath, ['--input-type=module', '-e', code],
        { timeout: 5000, stdio: ['pipe', 'pipe', 'pipe'] })
      const line = out.toString().trim()
      if (line) results.push(JSON.parse(line))
    } catch {}
  }
  return results
}

function processBlocks(addon, handle, channels, blockSize) {
  if (!Array.isArray(channels) || !channels.length ||
    channels.some(c => !(c instanceof Float32Array)))
    throw new TypeError('channels must be a nonempty array of Float32Array')
  const len = channels[0].length
  if (channels.some(c => c.length !== len)) throw new RangeError('channel lengths must match')
  const out = Array.from({ length: addon.getChannels(handle, 1) }, () => new Float32Array(len))
  for (let off = 0; off < len; off += blockSize) {
    const end = Math.min(len, off + blockSize)
    addon.process(handle, channels.map(c => c.subarray(off, end)), out.map(c => c.subarray(off, end)))
  }
  return out
}
