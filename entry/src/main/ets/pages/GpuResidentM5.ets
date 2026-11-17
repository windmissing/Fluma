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
    // T550 (problem T546 §4): startup transaction guarded; only the explicit
    // M5_NEW_EMPTY_RUN_REQUESTED success token arms the frame.
    try {
      const parsed = m5ParseNativeObject(native.m5ResearchConfig(new Uint32Array([mode, 0])), 0);
      if (parsed.error === 'REVISION_MISMATCH' || !parsed.ok && parsed.error === 'NATIVE_REPLY_NOT_OBJECT') {
        // T517_EXPLICIT_RESEARCH_CANDIDATE_1 carries a string revision; re-check.
        const raw: string = native.m5ResearchConfig(new Uint32Array([mode, 0]));
        if (typeof raw === 'string' && raw.indexOf('"T517_EXPLICIT_RESEARCH_CANDIDATE_1"') >= 0 && raw.indexOf('"words"') >= 0) {
          let words: Object | undefined;
          try { words = (JSON.parse(raw) as Record<string, Object>)['words']; } catch (e) { words = undefined; }
          if (Array.isArray(words)) {
            const list: Object[] = words;
            for (let i = 0; i < list.length; i++) { const w: Object = list[i]; if (typeof w !== 'number' || !Number.isSafeInteger(w) || w < 0 || w > 0xffffffff) { this.message = '研究参数无效'; return; } }
            const numbers: number[] = [];
            for (let i = 0; i < list.length; i++) numbers.push(list[i] as number);
            this.config = numbers; this.mode = mode;
            native.setActive(true);
            const reply: string = native.m5Configure(new Uint32Array(this.config));
            this.message = reply;
            if (reply !== 'M5_NEW_EMPTY_RUN_REQUESTED') return;
            this.identity = []; this.sequence = this.sample = this.intent = this.request = 0;
            this.attachFrame();
            return;
          }
        }
        this.message = '研究参数不可用（' + parsed.error + '）'; return;
      }
      if (!parsed.ok) { this.message = 'M5_CONFIG_FAULT：' + parsed.error; return; }
      const obj: Record<string, Object> = parsed.object as Record<string, Object>;
      const words: Object | undefined = obj['words'];
      if (!Array.isArray(words) || words.length !== 32) { this.message = '研究参数不可用'; return; }
      const wordsList: Object[] = words;
      for (let i = 0; i < wordsList.length; i++) { const w: Object = wordsList[i]; if (typeof w !== 'number' || !Number.isSafeInteger(w) || w < 0 || w > 0xffffffff) { this.message = '研究参数无效'; return; } }
      const numbers2: number[] = [];
      for (let i = 0; i < wordsList.length; i++) numbers2.push(wordsList[i] as number);
      this.config = numbers2; this.mode = mode;
      native.setActive(true);
      const reply: string = native.m5Configure(new Uint32Array(this.config));
      this.message = reply;
      if (reply !== 'M5_NEW_EMPTY_RUN_REQUESTED') return;
      this.identity = []; this.sequence = this.sample = this.intent = this.request = 0;
      this.attachFrame();
    } catch (e) {
      if (this.frame !== null) this.frame.owner = null;
      this.frame = null;
      this.message = 'M5_START_FAULT：' + String(e);
    }
  }
  private resume(): void {
    this.message = native.m5ResumeInput();
    if (this.message !== '0') return;
    this.identity = []; this.sequence = this.sample = this.intent = this.request = 0;
    this.attachFrame();
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
  aboutToDisappear(): void { if (this.frame !== null) this.frame.owner = null; this.frame = null; this.identity = []; native.setActive(false); }
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
      Text(this.message).fontSize(11).maxLines(4)
      XComponent({ id: 'gpuResidentM5', type: XComponentType.SURFACE, libraryname: 'grsm1' }).width('100%').layoutWeight(1)
    }.width('100%').height('100%').padding(6).backgroundColor('#080b12')
  }
}
