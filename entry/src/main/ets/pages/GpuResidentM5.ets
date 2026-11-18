import { common } from '@kit.AbilityKit';
import { GpuInputAttempt } from '../physics/GpuInputAttempt';
import native from 'libgrsm1.so';
import { FrameCallback } from '@ohos.arkui.UIContext';

interface M5Words { revision?: number; features?: number; identity?: number[]; sourceControlGeneration?: number[]; words?: number[]; }
// T550 (problem T546 §4): safe native object parse for the M5 route. Bounded
// string, caught parse, object root, revision gate; LEGACY_NON_JSON_REPLY
// surfaces old bare-token replies as controlled failure.
interface M5ParseResult { ok: boolean; object: Record<string, Object> | null; error: string; }
function m5ParseNativeObject(raw: string, allowedRevision: number): M5ParseResult {
  const fail = (error: string): M5ParseResult => ({ ok: false, object: null, error: error });
  if (typeof raw !== 'string' || raw.length === 0 || raw.length > 65536) return fail('NATIVE_REPLY_NOT_STRING');
  let parsed: Object;
  try { parsed = JSON.parse(raw); } catch (e) { return fail('LEGACY_NON_JSON_REPLY'); }
  if (parsed === null || typeof parsed !== 'object' || Array.isArray(parsed)) return fail('NATIVE_REPLY_NOT_OBJECT');
  const record = parsed as Record<string, Object>;
  if (record['revision'] === 'GPU_NAPI_ERROR_V1') {
    const code = typeof record['code'] === 'string' ? record['code'] as string : 'UNKNOWN';
    return fail('NAPI_ERROR:' + code);
  }
  const revision = record['revision'];
  if (typeof revision !== 'number' || revision !== allowedRevision) return fail('REVISION_MISMATCH');
  return { ok: true, object: record, error: '' };
}
class M5Frame extends FrameCallback {
  owner: GpuResidentM5 | null = null;
  onFrame(ns: number): void { if (this.owner !== null) this.owner.frameTick(this, ns); }
}

@Entry
@Component
struct GpuResidentM5 {
  @State message: string = 'V5 研究入口；起沙默认关闭';
  @State signalOn: boolean = false;
  private frame: M5Frame | null = null;
  @State inputDiagnostic: string = '';
  private startupIntent: number = 0;
  private inputCall(api: string, words: Uint32Array, schema: string, invoke: (value: Uint32Array) => string): string {
    const context = getContext(this) as common.UIAbilityContext;
    return GpuInputAttempt.call('M3B', api, words, schema, context.filesDir, invoke, (state: string): void => { this.inputDiagnostic = state; });
  }
  private requested(reply: string, state: string): boolean { return GpuInputAttempt.requested(reply, state); }
  private stopFrame(): void {
    const previous = this.frame;
    if (previous !== null) previous.owner = null;
    this.frame = null;
  }
  private startupFault(message: string): void {
    ++this.startupIntent;
    this.stopFrame(); this.identity = []; this.message = message;
    try { native.setActive(false); } catch (cleanup) { this.message += ';CLEANUP_FAILED:' + String(cleanup); }
  }
  private identity: number[] = [];
  private config: number[] = [];
  private sourceGeneration: number[] = [1, 0];
  private sequence: number = 0;
  private sample: number = 0;
  private intent: number = 0;
  private request: number = 0;
  private mode: number = 0;

  private pair(w: Uint32Array, at: number, n: number): void { w[at] = n % 4294967296; w[at + 1] = Math.floor(n / 4294967296); }
  private attachFrame(): void {
    if (this.frame !== null) this.frame.owner = null;
    const next = new M5Frame(); next.owner = this; this.frame = next;
    this.getUIContext().postFrameCallback(next);
  }
  private start(mode: number): void {
    const intent: number = ++this.startupIntent;
    this.stopFrame(); this.identity = [];
    try {
      const raw = this.inputCall('m5ResearchConfig', new Uint32Array([mode, 0]), 'M5_RESEARCH_REQUEST2', (words: Uint32Array): string => native.m5ResearchConfig(words));
      if (raw.length === 0 || raw.length > 4096) throw new Error('M5_RESEARCH_REPLY_BOUND');
      const parsed: Object = JSON.parse(raw);
      if (parsed === null || typeof parsed !== 'object' || Array.isArray(parsed)) throw new Error('M5_RESEARCH_REPLY_OBJECT');
      const record = parsed as Record<string, Object>;
      if (record['revision'] !== 'T517_EXPLICIT_RESEARCH_CANDIDATE_1') throw new Error(raw);
      const values: Object = record['words'];
      if (!Array.isArray(values) || values.length !== 32) throw new Error('M5_CONFIG32_INVALID');
      const config: number[] = [];
      const list: Object[] = values;
      for (let i: number = 0; i < list.length; i++) { const value: Object = list[i]; if (typeof value !== 'number' || !Number.isSafeInteger(value) || value < 0 || value > 4294967295) throw new Error('M5_CONFIG_WORD_INVALID'); config.push(value); }
      this.config = config; this.mode = mode; native.setActive(true);
      const reply = this.inputCall('m5Configure', new Uint32Array(config), 'M5_CONFIG32_V5', (words: Uint32Array): string => native.m5Configure(words));
      this.message = reply;
      if (!this.requested(reply, 'M5_NEW_EMPTY_RUN_REQUESTED')) { native.setActive(false); return; }
      this.sequence = this.sample = this.intent = this.request = 0;
      GpuInputAttempt.waitSession((): string => native.m5Session(), '5', 31, (): boolean => intent === this.startupIntent,
        (identity: number[], source: number[]): void => { this.identity = identity; this.sourceGeneration = source; this.attachFrame(); },
        (error: string): void => { this.startupFault(error); });
    } catch (error) { this.startupFault('M5_START_FAULT:' + String(error)); }
  }
  private resume(): void {
    const intent: number = ++this.startupIntent; this.stopFrame(); this.identity = [];
    try {
      this.message = native.m5ResumeInput();
      if (this.message !== '0') return;
      this.sequence = this.sample = this.intent = this.request = 0;
      GpuInputAttempt.waitSession((): string => native.m5Session(), '5', 31, (): boolean => intent === this.startupIntent,
        (identity: number[], source: number[]): void => { this.identity = identity; this.sourceGeneration = source; this.attachFrame(); },
        (error: string): void => { this.startupFault(error); });
    } catch (error) { this.startupFault('M5_RESUME_FAULT:' + String(error)); }
  }
  private offerSynthetic(): boolean {
    if (this.mode !== 2) return true;
    if (this.sample >= 4294967295) { this.message = 'M5_SAMPLE_SEQUENCE_EXHAUSTED'; return false; }
    const clock = JSON.parse(native.m5InputClock()) as M5Words; // kept inside try of frameTick; bounded words gate below
    if (clock.words?.length !== 2 || this.config.length !== 32) return false;
    const w = new Uint32Array(64);
    w[0] = 893994572; w[1] = 5; w[2] = 2; w[3] = 2;
    w[4] = this.identity[6]; w[5] = this.identity[7]; w[6] = ++this.sample; w[7] = 1;
    w[8] = clock.words[0]; w[9] = clock.words[1]; w[14] = 2; w[15] = this.config[23];
    w[40] = this.identity[8]; w[41] = this.identity[9]; w[42] = this.identity[2]; w[43] = this.identity[3];
    w[44] = this.identity[0]; w[45] = this.identity[1]; w[46] = this.identity[4]; w[47] = this.identity[5];
    for (let i: number = 0; i < 8; i++) w[48 + i] = this.config[24 + i];
    const bits = new DataView(w.buffer);
    bits.setFloat64(18 * 4, this.signalOn ? 1 : 0, true);
    bits.setFloat64(34 * 4, 0.6, true); bits.setFloat64(36 * 4, 1, true); bits.setFloat64(38 * 4, 6, true);
    const code: string = native.m5OfferInput64(w);
    if (code !== '0') this.message = `合成输入拒绝 ${code}`;
    return code === '0';
  }
  frameTick(frame: M5Frame, ns: number): void {
    if (frame !== this.frame || frame.owner !== this || !Number.isSafeInteger(ns) || ns < 0) return;
    try {
      if (this.identity.length !== 18) {
        const raw: string = native.m5Session();
        let s: M5Words | null = null;
        try { s = JSON.parse(raw) as M5Words; } catch (e) { s = null; }
        if (s === null || typeof s.revision !== 'number' || s.revision !== 5 || s.features !== 31 || s.identity?.length !== 18) { this.getUIContext().postFrameCallback(frame); return; }
        for (const w of s.identity) if (typeof w !== 'number' || !Number.isSafeInteger(w) || w < 0 || w > 0xffffffff) { this.getUIContext().postFrameCallback(frame); return; }
        this.identity = s.identity; this.sourceGeneration = s.sourceControlGeneration ?? [1, 0];
      }
      if (!this.offerSynthetic()) { frame.owner = null; return; }
      if (this.sequence >= Number.MAX_SAFE_INTEGER) { frame.owner = null; this.message = 'M5_PULSE_SEQUENCE_EXHAUSTED'; return; }
      const pulse = new Uint32Array(22); pulse.set(this.identity); this.pair(pulse, 18, ++this.sequence); this.pair(pulse, 20, ns);
      const code: string = native.m5Pulse(pulse);
      this.message = code === '0' ? native.status() : `V5 暂停 ${code}`;
      if (code === '0') this.getUIContext().postFrameCallback(frame); else frame.owner = null;
    } catch (e) { frame.owner = null; native.setActive(false); this.message = 'V5 输入暂停'; }
  }
  private source(kind: number): void {
    if (this.identity.length !== 18 || this.intent >= Number.MAX_SAFE_INTEGER || this.request >= Number.MAX_SAFE_INTEGER) return;
    const w = new Uint32Array(566); w.set(this.identity); w[18] = this.sourceGeneration[0]; w[19] = this.sourceGeneration[1]; this.pair(w, 20, ++this.intent);
    const at: number = 22; w[at] = 1129534023; w[at + 1] = 5; w[at + 2] = kind;
    w[at + 4] = this.identity[0]; w[at + 5] = this.identity[1]; this.pair(w, at + 6, ++this.request);
    w[at + 8] = this.identity[12]; w[at + 9] = this.identity[13]; w[at + 10] = this.identity[2]; w[at + 11] = this.identity[3];
    w[at + 12] = this.identity[16]; w[at + 13] = this.identity[17]; w[at + 14] = 1; w[at + 15] = 1;
    this.pair(w, at + 24, this.request); w[at + 28] = this.identity[14]; w[at + 29] = this.identity[15];
    const bits = new DataView(w.buffer); bits.setFloat64((at + 16) * 4, 64.5, true);
    bits.setFloat64((at + 18) * 4, kind === 1 ? 0.5 : 12, true); bits.setFloat64((at + 20) * 4, kind === 1 ? 8 : 0.25, true);
    this.message = native.m5Offer(w);
  }
  private target(index: number): void {
    if (this.identity.length !== 18) return;
    const w = new Uint32Array(19); w.set(this.identity); w[18] = index; this.message = native.m5Target(w);
  }
  aboutToDisappear(): void {
    this.startupFault(this.message); }
  build() {
    Column({ space: 6 }) {
      Text('V5 独立研究：真实来源层与活动粒子').fontSize(17)
      Text('候选参数未标定；合成信号不代表真实传感器资格').fontSize(12)
      Row({ space: 6 }) {
        Button('新空运行 · 起沙关闭').onClick(() => this.start(0))
        Button('新空运行 · 合成研究').onClick(() => this.start(2))
      }
      Row({ space: 6 }) {
        Button('投沙').onClick(() => this.source(0))
        Button('加水').onClick(() => this.source(1))
        Button(this.signalOn ? '停止合成信号' : '开启合成信号').onClick(() => this.signalOn = !this.signalOn)
        Button('恢复输入').onClick(() => this.resume())
      }
      Row({ space: 6 }) {
        Button('-30°').onClick(() => this.target(-7680))
        Button('0°').onClick(() => this.target(0))
        Button('+30°').onClick(() => this.target(7680))
        Button('冷档状态').onClick(() => this.message = native.m5ColdCapabilities())
      }
      Text(this.inputDiagnostic).fontSize(10)
      Text(this.message).fontSize(11).maxLines(4)
      XComponent({ id: 'gpuResidentM5', type: XComponentType.SURFACE, libraryname: 'grsm1' }).width('100%').layoutWeight(1)
    }.width('100%').height('100%').padding(6).backgroundColor('#080b12')
  }
}
