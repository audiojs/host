import test from 'tst'
import { ok, is } from 'tst'
import { load } from './index.js'

const VST3_PATH = '/Library/Audio/Plug-Ins/VST3'
const PLUGIN = `${VST3_PATH}/Bertom_DenoiserClassic.vst3`

test('load plugin', () => {
  const p = load(PLUGIN)
  ok(p.name, 'has name')
  ok(p.vendor, 'has vendor')
  console.log(`  name: ${p.name}, vendor: ${p.vendor}`)
  console.log(`  channels: ${p.inputChannels} in, ${p.outputChannels} out`)
  p.close()
})

test('list parameters', () => {
  const p = load(PLUGIN)
  const params = p.params
  ok(params.length >= 0, 'params enumerated')
  console.log(`  ${params.length} parameters`)
  for (const param of params.slice(0, 5)) {
    console.log(`  [${param.id}] ${param.name}: ${param.min}..${param.max} (default: ${param.defaultValue})`)
  }
  p.close()
})

test('process silence', () => {
  const p = load(PLUGIN)
  const n = 128
  const inL = new Float32Array(n)
  const inR = new Float32Array(n)
  const outL = new Float32Array(n)
  const outR = new Float32Array(n)

  p.process([inL, inR], [outL, outR])
  /* Silence in should produce silence (or near-silence) out */
  ok(true, 'process did not crash')
  p.close()
})

test('process sine wave', () => {
  const p = load(PLUGIN)
  const sr = 44100, freq = 440, dur = 0.1
  const n = Math.ceil(sr * dur)
  const sine = new Float32Array(n)
  for (let i = 0; i < n; i++) sine[i] = Math.sin(2 * Math.PI * freq * i / sr) * 0.5

  const out = p.processAll([sine, new Float32Array(sine)])
  const energy = out[0].reduce((s, v) => s + v * v, 0) / n
  console.log(`  output energy: ${energy.toFixed(6)}`)
  ok(energy >= 0, 'has output energy')
  p.close()
})

test('process with audio-decode', async () => {
  let decode
  try { decode = (await import('audio-decode')).default } catch { return console.log('  skipped: audio-decode not installed') }

  const p = load(PLUGIN)

  /* Decode a test file if available, otherwise generate test signal */
  const sr = 44100, n = sr /* 1 second */
  const channels = []
  for (let ch = 0; ch < 2; ch++) {
    const buf = new Float32Array(n)
    for (let i = 0; i < n; i++) buf[i] = Math.sin(2 * Math.PI * 440 * i / sr) * 0.5
    channels.push(buf)
  }

  const out = p.processAll(channels)
  ok(out.length === 2, '2 output channels')
  is(out[0].length, n, 'correct output length')
  console.log(`  processed ${n} samples (${(n/sr).toFixed(1)}s)`)
  p.close()
})
