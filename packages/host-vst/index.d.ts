// Generated-adjacent hand-written types — see index.js.
/** A loaded native plugin instance. */
export interface Plugin {
  readonly name: string
  readonly vendor: string
  blockSize: number
  /** Parameter descriptors as reported by the plugin. */
  readonly params: ParamInfo[]
  getParam(id: number): number
  /** Normalized 0..1 value, applied at the next processing block boundary. */
  setParam(id: number, value: number): void
  /** Process one block: planar channel arrays, in place into outputs. */
  process(inputs: Float32Array[] | null, outputs: Float32Array[]): void
  /** Convenience: run whole channel data through block-wise processing. */
  processAll(channels: Float32Array[]): Float32Array[]
  close(): void
}
export interface ParamInfo {
  id: number
  name: string
  min: number
  max: number
  defaultValue: number
  [extra: string]: unknown
}
export interface LoadOptions { sampleRate?: number, channels?: number, blockSize?: number }
export interface ScanEntry { path: string, name: string, vendor: string, format: string }
/** Load a VST3 plugin (.vst3). */
export function load(path: string, opts?: LoadOptions): Plugin
/** Scan default (or given) directories for installed VST3 plugins. */
export function scan(dirs?: string[]): ScanEntry[]
/** Wrap a VST3 plugin as an AudioWorkletProcessor class. */
export function register(path: string, BaseClass: unknown, opts?: LoadOptions): unknown
