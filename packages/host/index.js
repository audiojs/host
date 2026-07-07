/**
 * @module audio-host
 * Universal audio plugin host — load VST3, CLAP (and more) through one API.
 */

const hosts = {}

/* Discover installed format hosts */
for (const [ext, pkg] of [
  ['.vst3', '@audio/host-vst'],
  ['.clap', '@audio/host-clap'],
]) {
  try { hosts[ext] = await import(pkg) } catch {}
}

function hostFor(path) {
  for (const [ext, mod] of Object.entries(hosts)) {
    if (path.endsWith(ext)) return mod
  }
  const known = Object.keys(hosts).join(', ') || 'none installed'
  throw new Error(`Unsupported plugin format: ${path}\nInstalled: ${known}`)
}

/**
 * Load any audio plugin by path. Format detected from extension.
 *
 *   const plugin = load('reverb.vst3')
 *   const gain = load('gain.clap')
 */
export function load(path, opts) {
  return hostFor(path).load(path, opts)
}

/**
 * Register any plugin as AudioWorkletProcessor. Format detected from extension.
 *
 *   await ctx.audioWorklet.addModule(register('reverb.vst3', AudioWorkletProcessor))
 */
export function register(path, BaseClass, opts) {
  return hostFor(path).register(path, BaseClass, opts)
}

/**
 * Scan system for all installed plugins across all formats.
 * Returns array of { path, name, vendor, format }.
 */
export function scan(dirs) {
  const results = []
  for (const mod of Object.values(hosts)) {
    if (mod.scan) results.push(...mod.scan(dirs))
  }
  return results
}

/** Supported format extensions (based on installed host packages) */
export const formats = Object.keys(hosts)
