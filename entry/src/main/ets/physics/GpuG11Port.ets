import native from 'libgrs_g11.so';
export interface G11Scope { run: string; owner: string; context: string; inputEpoch: string; pageSession: string; permission: string; window: string; config: string; surface: string; }
export interface G11Status { state: string; configured: boolean; bootstrapRawReady: boolean; bootstrapSealed: boolean; scope: G11Scope; token: Object | null; presentedMapping: Object | null; }
interface G11Preset { parentRequest: string; seedReceipt: string; config: number[]; }
export class GpuG11Port {
 private sourceSequence: number = 0;
 configure(parent: string, k: number, v: number, full: boolean): string {
  const preset = JSON.parse(native.g11Preset(parent, k, v, full, 256)) as G11Preset;
  if (preset.config === undefined || preset.config.length !== 64) throw new Error('PRESET_REJECTED');
  this.sourceSequence = 0;
  return native.g11ResearchConfigure(parent, preset.parentRequest, new Uint32Array(preset.config), preset.seedReceipt);
 }
 status(): G11Status { return JSON.parse(native.g11Status()) as G11Status; }
 source(): string {
  if (this.sourceSequence >= 4294967295) throw new Error('SOURCE_SEQUENCE_EXHAUSTED');
  const body = new Uint32Array(128); body[40] = 1; body[41] = ++this.sourceSequence;
  const bytes = new DataView(body.buffer);
  bytes.setFloat64(64 * 4, 0.1, true); bytes.setFloat64(66 * 4, 64.0, true); bytes.setFloat64(68 * 4, 128.0, true);
  bytes.setFloat64(70 * 4, 0.0, true); bytes.setFloat64(72 * 4, 0.0, true); body[74] = 2;
  return native.g11ModelCommand128(body, String(this.sourceSequence));
 }
}
