/**
 * @module @audio/host-clap
 * CLAP plugin host for audiojs
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

    process(inputs, outputs) { addon.process(handle, inputs, outputs) },
    processAll(ch) { return processBlocks(addon, handle, ch, blockSize) },

    close() {
      if (closed) return
      closed = true
      addon.close(handle)
    }
  }
}

/**
 * Scan system for installed CLAP plugins.
 * Returns array of { path, name, vendor, format } for each loadable plugin.
 */
export function scan(dirs) {
  return scanDir(dirs || defaultPaths({
    darwin: ['/Library/Audio/Plug-Ins/CLAP', '$HOME/Library/Audio/Plug-Ins/CLAP'],
    linux: ['$HOME/.clap', '/usr/lib/clap', '/usr/local/lib/clap'],
    win32: ['$PROGRAMFILES/Common Files/CLAP'],
  }), '.clap', 'clap')
}

/**
 * Register a CLAP plugin as an AudioWorkletProcessor.
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
  const len = channels[0].length, numCh = channels.length
  const out = Array.from({ length: numCh }, () => new Float32Array(len))

  const inBlock = Array.from({ length: numCh }, () => new Float32Array(blockSize))
  const outBlock = Array.from({ length: numCh }, () => new Float32Array(blockSize))

  for (let off = 0; off < len; off += blockSize) {
    const n = Math.min(blockSize, len - off)
    if (n === blockSize) {
      const inSub = [], outSub = []
      for (let c = 0; c < numCh; c++) {
        inSub[c] = channels[c].subarray(off, off + blockSize)
        outSub[c] = out[c].subarray(off, off + blockSize)
      }
      addon.process(handle, inSub, outSub)
    } else {
      for (let c = 0; c < numCh; c++) {
        inBlock[c].fill(0)
        inBlock[c].set(channels[c].subarray(off, off + n))
        outBlock[c].fill(0)
      }
      addon.process(handle, inBlock, outBlock)
      for (let c = 0; c < numCh; c++) out[c].set(outBlock[c].subarray(0, n), off)
    }
  }
  return out
}
