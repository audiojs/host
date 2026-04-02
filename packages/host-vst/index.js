/**
 * @module @audio/host-vst
 * VST3 plugin host for audiojs
 */
import addon from './src/addon.js'

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

function processBlocks(addon, handle, channels, blockSize) {
  const len = channels[0].length, numCh = channels.length
  const out = Array.from({ length: numCh }, () => new Float32Array(len))

  /* Pre-allocate block views — reused across iterations */
  const inBlock = Array.from({ length: numCh }, () => new Float32Array(blockSize))
  const outBlock = Array.from({ length: numCh }, () => new Float32Array(blockSize))

  for (let off = 0; off < len; off += blockSize) {
    const n = Math.min(blockSize, len - off)
    if (n === blockSize) {
      /* Hot path: subarray is zero-alloc (shares underlying buffer) */
      const inSub = [], outSub = []
      for (let c = 0; c < numCh; c++) {
        inSub[c] = channels[c].subarray(off, off + blockSize)
        outSub[c] = out[c].subarray(off, off + blockSize)
      }
      addon.process(handle, inSub, outSub)
    } else {
      /* Last partial block: copy into pre-allocated padded buffers */
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
