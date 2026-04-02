import test from 'tst'
import { ok, is } from 'tst'
import { load } from './index.js'
import { fileURLToPath } from 'url'
import { dirname, join } from 'path'

const __dirname = dirname(fileURLToPath(import.meta.url))
const PLUGIN = join(__dirname, 'test-plugin', 'gain.clap')

test('load plugin', () => {
  const p = load(PLUGIN)
  ok(p.name, 'has name')
  ok(p.vendor, 'has vendor')
  console.log(`  name: ${p.name}, vendor: ${p.vendor}`)
  p.close()
})

test('list parameters', () => {
  const p = load(PLUGIN)
  const params = p.params
  ok(params.length > 0, 'has params')
  console.log(`  ${params.length} parameters`)
  for (const param of params) {
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
  /* Gain of 1.0 on silence should produce silence */
  const energy = outL.reduce((s, v) => s + v * v, 0)
  is(energy, 0, 'silence in = silence out')
  p.close()
})

test('process sine wave', () => {
  const p = load(PLUGIN)
  const sr = 44100, freq = 440, dur = 0.1
  const n = Math.ceil(sr * dur)
  const sine = new Float32Array(n)
  for (let i = 0; i < n; i++) sine[i] = Math.sin(2 * Math.PI * freq * i / sr) * 0.5

  const out = p.processAll([sine, new Float32Array(sine)])
  const inEnergy = sine.reduce((s, v) => s + v * v, 0) / n
  const outEnergy = out[0].reduce((s, v) => s + v * v, 0) / n
  console.log(`  in energy: ${inEnergy.toFixed(6)}, out energy: ${outEnergy.toFixed(6)}`)
  /* Gain = 1.0, so energy should be identical */
  ok(Math.abs(inEnergy - outEnergy) < 1e-6, 'unity gain preserves energy')
  p.close()
})
