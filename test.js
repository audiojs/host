/**
 * E2E test: decode audio → process through VST3 plugin → play through speaker
 *
 * Usage: node test.js [plugin.vst3] [audio-file]
 *
 * Defaults to Yamaha REV-X reverb + bundled test wav.
 * Requires: audio-decode, audio-speaker (from sibling projects or npm)
 */
import { readFileSync } from 'fs'
import { load } from './packages/host-vst/index.js'

const AUDIO_FILE = process.argv[3] || '/Users/div/projects/web-audio-api/test/sounds/steps-stereo-16b-44khz.wav'
const PLUGIN_PATH = process.argv[2] || '/Library/Audio/Plug-Ins/VST3/Yamaha/REV-X_HALL.vst3'

/* Resolve audiojs deps from sibling projects or node_modules */
async function importFrom(...paths) {
  for (const p of paths) {
    try { return await import(p) } catch {}
  }
  throw new Error('Could not import: ' + paths[0])
}

const decode = (await importFrom(
  '/Users/div/projects/@audio/decode/audio-decode.js',
  'audio-decode'
)).default

const speaker = (await importFrom(
  '/Users/div/projects/@audio/speaker/index.js',
  'audio-speaker'
)).default

/* 1. Decode audio file */
const raw = readFileSync(AUDIO_FILE)
const audio = await decode(raw)
const channels = audio.channelData
const sr = audio.sampleRate
const numCh = channels.length
const numSamples = channels[0].length

console.log(`input: ${AUDIO_FILE.split('/').pop()}`)
console.log(`  ${numCh}ch, ${sr}Hz, ${numSamples} samples (${(numSamples / sr).toFixed(2)}s)`)

/* 2. Load plugin */
const plugin = load(PLUGIN_PATH, { sampleRate: sr, channels: numCh })
console.log(`plugin: ${plugin.name} (${plugin.vendor})`)
console.log(`  ${plugin.inputChannels} in, ${plugin.outputChannels} out, ${plugin.params.length} params`)

/* 3. Process audio through plugin */
const t0 = performance.now()
const out = plugin.processAll(channels)
const dt = performance.now() - t0
const rtRatio = (numSamples / sr * 1000) / dt
console.log(`process: ${dt.toFixed(1)}ms (${rtRatio.toFixed(0)}x realtime)`)

/* 4. Play result */
const write = speaker({ sampleRate: sr, channels: numCh, bitDepth: 32 })
console.log(`playing through ${write.backend}...`)

/* Interleave Float32 channels → Buffer */
const frameSize = numCh * 4
const buf = Buffer.alloc(numSamples * frameSize)
for (let i = 0; i < numSamples; i++) {
  for (let c = 0; c < numCh; c++) {
    buf.writeFloatLE(out[c][i], (i * numCh + c) * 4)
  }
}

write(buf, (err) => {
  if (err) console.error('write error:', err)
  write.flush(() => {
    write.close()
    plugin.close()
    console.log('done')
  })
})
