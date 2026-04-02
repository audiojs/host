/**
 * NAPI addon loader — tries prebuilt, then local build
 */
import { createRequire } from 'node:module'
import { join, dirname } from 'node:path'
import { fileURLToPath } from 'node:url'
import { arch, platform } from 'node:os'

const require = createRequire(import.meta.url)
const root = join(dirname(fileURLToPath(import.meta.url)), '..')
const plat = `${platform()}-${arch()}`

let addon
const loaders = [
  () => require(`@audio/host-vst-${plat}`),
  () => require(join(root, 'packages', `host-vst-${plat}`, 'host_vst.node')),
  () => require(join(root, 'prebuilds', plat, 'host_vst.node')),
  () => require(join(root, 'build', 'Release', 'host_vst.node')),
  () => require(join(root, 'build', 'Debug', 'host_vst.node')),
]
for (const load of loaders) {
  try { addon = load(); break } catch {}
}
if (!addon) throw new Error('host-vst addon not found — run `npm run build` or install @audio/host-vst-' + plat)

export default addon
