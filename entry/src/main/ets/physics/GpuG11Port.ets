import native from 'libgrs_g11.so';
export interface G11Scope { run: string; owner: string; context: string; inputEpoch: string; pageSession: string; permission: string; window: string; config: string; surface: string; }
export interface G11ConfigureAttempt { attempt: string; stage: string; scope: G11Scope | null; ownerResult: Object | null; firstFailure: Object | null; cleanup: Object | null; termination: Object | null; }
export interface G11Status { state: string; configured: boolean; bootstrapRawReady: boolean; bootstrapSealed: boolean; scope: G11Scope; token: Object | null; presentedMapping: Object | null; configureAttempts?: G11ConfigureAttempt[]; lifecycle?: Object | null; }
export interface G11ConfigureReply { profile: string; elements: number; bytes: number; offset: number; presetRaw: string; raw: string; error: string; attempt: string; }
interface G11Preset { parentRequest: string; seedReceipt: string; config: number[]; }
interface G11Admission { state: string; attempt?: string; }
export class GpuG11Port {
 private sourceSequence: number = 0;
 configure(parent: string, k: number, v: number, full: boolean): G11ConfigureReply {
  const reply: G11ConfigureReply = { profile: parent + '/' + k + '/' + v + '/' + (full ? 'FULL' : 'ROOT'),
   elements: 0, bytes: 0, offset: 0, presetRaw: '', raw: '', error: '', attempt: '' };
  try {
   reply.presetRaw = native.g11Preset(parent, k, v, full, 256);
   const preset = JSON.parse(reply.presetRaw) as G11Preset;
   if (preset.config === undefined || preset.config.length !== 64) throw new Error('PRESET_REJECTED');
   const view = new Uint32Array(preset.config);
   reply.elements = view.length; reply.bytes = view.byteLength; reply.offset = view.byteOffset;
   this.sourceSequence = 0;
   reply.raw = native.g11ResearchConfigure(parent, preset.parentRequest, view, preset.seedReceipt);
   const state = JSON.parse(reply.raw) as G11Admission;
   reply.attempt = state.attempt ?? '';
  } catch (error) {
   reply.error = String(error);
  }
  return reply;
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
