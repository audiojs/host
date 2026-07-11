/** A loaded native plugin instance. */
export interface Plugin {
  readonly name: string
  readonly vendor: string
  blockSize: number
  /** Parameter descriptors as reported by the plugin. */
  readonly params: ParamInfo[]
  getParam(id: number): number
  /** Process one block: planar bus arrays, in place into outputs. */
  process(inputs: Float32Array[][], outputs: Float32Array[][]): void
  /** Convenience: run whole channel data through block-wise processing. */
  processAll(channels: Float32Array[]): Float32Array[]
  close(): void
}
export interface ParamInfo {
  id: number
  name: string
  min: number
  max: number
  default: number
  [extra: string]: unknown
}
export interface LoadOptions { sampleRate?: number, channels?: number, blockSize?: number }
export interface ScanEntry { path: string, name: string, vendor: string, format: string }
/** Load any plugin by path — format detected from extension (.vst3, .clap). */
export function load(path: string, opts?: LoadOptions): Plugin
/** Register any plugin as an AudioWorkletProcessor — format from extension. */
export function register(path: string, BaseClass: unknown, opts?: LoadOptions): unknown
/** Scan for installed plugins across all installed format hosts. */
export function scan(dirs?: string[]): ScanEntry[]
/** Formats with an installed host, e.g. ['.vst3', '.clap']. */
export const formats: string[]
