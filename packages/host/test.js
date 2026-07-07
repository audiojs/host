import test from 'tst'
import { ok, is } from 'tst'
import { load, register, formats } from './index.js'

test('formats detected', () => {
  ok(formats.length > 0, 'at least one format')
  console.log(`  formats: ${formats.join(', ')}`)
})

test('load vst3', () => {
  if (!formats.includes('.vst3')) return console.log('  skipped: @audio/host-vst not installed')
  const p = load('/Library/Audio/Plug-Ins/VST3/Bertom_DenoiserClassic.vst3')
  ok(p.name, 'has name')
  console.log(`  ${p.name} (${p.vendor})`)
  p.close()
})

test('load clap', async () => {
  if (!formats.includes('.clap')) return console.log('  skipped: @audio/host-clap not installed')
  const { dirname, join } = await import('path')
  const { fileURLToPath } = await import('url')
  const pluginDir = join(dirname(fileURLToPath(import.meta.url)), '..', 'host-clap', 'test-plugin')
  const p = load(join(pluginDir, 'gain.clap'))
  ok(p.name, 'has name')
  console.log(`  ${p.name} (${p.vendor})`)
  p.close()
})

test('process audio through vst3', () => {
  if (!formats.includes('.vst3')) return console.log('  skipped')
  const p = load('/Library/Audio/Plug-Ins/VST3/Yamaha/EQ-1A.vst3')
  const sr = 44100, n = 4096
  const sine = new Float32Array(n)
  for (let i = 0; i < n; i++) sine[i] = Math.sin(2 * Math.PI * 440 * i / sr) * 0.5
  const out = p.processAll([sine, new Float32Array(sine)])
  const energy = out[0].reduce((s, v) => s + v * v, 0) / n
  ok(energy > 0, 'has output')
  console.log(`  energy: ${energy.toFixed(6)}`)
  p.close()
})

test('unsupported format throws', () => {
  let threw = false
  try { load('plugin.lv2') } catch { threw = true }
  ok(threw, 'throws on unknown format')
})
