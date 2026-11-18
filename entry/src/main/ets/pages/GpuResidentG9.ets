import native from 'libgrsm1.so';
import { FrameCallback } from '@ohos.arkui.UIContext';
import fs from '@ohos.file.fs';
import { common } from '@kit.AbilityKit';
import { GpuG9Cold } from '../physics/GpuG9Cold';

interface G9Config { words?: number[]; }
interface G9Session { revision?: string; identity?: number[]; }
// T550 (problem T546 §4): shared safe native object parse. Checked string,
// bounded length, caught JSON.parse, non-null non-array object root, exact
// revision match. LEGACY_NON_JSON_REPLY surfaces old bare-token replies as a
// controlled failure instead of crashing on JSON.parse.
interface NativeObjectResult { ok: boolean; words: number[] | null; revision: string | null; error: string; }
function parseNativeObject(raw: string, api: string, allowedRevisions: string[]): NativeObjectResult {
  const fail = (error: string): NativeObjectResult => ({ ok: false, words: null, revision: null, error: error });
  if (typeof raw !== 'string' || raw.length === 0 || raw.length > 65536) return fail('NATIVE_REPLY_NOT_STRING');
  let parsed: Object;
  try { parsed = JSON.parse(raw); } catch (e) { return fail('LEGACY_NON_JSON_REPLY'); }
  if (parsed === null || typeof parsed !== 'object' || Array.isArray(parsed)) return fail('NATIVE_REPLY_NOT_OBJECT');
  const record = parsed as Record<string, Object>;
  // GPU_NAPI_ERROR_V1 (T546 §3): safe error object — stop and surface it.
  if (record['revision'] === 'GPU_NAPI_ERROR_V1') {
    const code = typeof record['code'] === 'string' ? record['code'] as string : 'UNKNOWN';
    return fail('NAPI_ERROR:' + code);
  }
  const revision = record['revision'];
  if (typeof revision !== 'string' || allowedRevisions.indexOf(revision) < 0) return fail('REVISION_MISMATCH');
  if (api === 'g9ResearchConfig' || api === 'g9ResearchConfig64') {
    const words: Object | undefined = record['words'];
    if (!Array.isArray(words) || words.length !== (api === 'g9ResearchConfig64' ? 64 : 24)) return fail('CONFIG_WORDS_INVALID');
    const checked: number[] = [];
    for (let i = 0; i < words.length; i++) {
      const w: Object = words[i];
      if (typeof w !== 'number' || !Number.isSafeInteger(w) || w < 0 || w > 0xffffffff) return fail('CONFIG_WORDS_INVALID');
      checked.push(w);
    }
    return { ok: true, words: checked, revision: revision, error: '' };
  }
  return { ok: true, words: null, revision: revision, error: '' };
}
class G9Frame extends FrameCallback {
  owner: GpuResidentG9 | null = null;
  onFrame(ns: number): void { if (this.owner !== null) this.owner.tick(this, ns); }
}

@Entry
@Component
struct GpuResidentG9 {
  @State message: string = 'G9 独立研究：未启动';
  @State archive: string = '';
  @State angle: number = 0;
  @State realTouch: boolean = false;
  @State touchMessage: string = '未启用真实拨动';
  private publication: string = '0';
  private token: string = '';
  private identity: number[] = [];
  private sequence: number = 0;
  private command: number = 0;
  private frame: G9Frame | null = null;
  private cold: GpuG9Cold = new GpuG9Cold();
  private file: fs.File | null = null;
  private path: string = '';

  private pair(words: Uint32Array, at: number, value: number): void {
    words[at] = value % 4294967296;
    words[at + 1] = Math.floor(value / 4294967296);
  }
  private closeArchive(complete: boolean): void {
    if (this.file === null) return;
    try { fs.fsyncSync(this.file.fd); } finally { fs.closeSync(this.file.fd); this.file = null; }
    if (complete) { const final: string = this.path.replace('.part.jsonl', '.jsonl'); fs.renameSync(this.path, final); this.archive = final; }
    else this.archive = 'INCOMPLETE ' + this.path;
  }
  private start(k: number, full: boolean): void {
    this.realTouch = false; this.publication = '0'; this.token = ''; native.g9TouchCancel();
    // T550 (problem T546 §4): the whole startup transaction (archive close,
    // setActive, config read, configure, frame scheduling) is guarded; any
    // failure cancels the new frame and never schedules a pulse. Only the
    // explicit G9_NEW_EMPTY_RESEARCH_RUN_REQUESTED success token arms the run.
    let firstError: string = '';
    try {
      try { this.cold.cancel(); } catch (cleanup) { firstError = 'COLD_CANCEL_FAILED ' + String(cleanup); }
      try { this.closeArchive(false); } catch (cleanup) { if (firstError === '') firstError = 'ARCHIVE_CLOSE_FAILED ' + String(cleanup); }
      if (this.frame !== null) this.frame.owner = null;
      this.frame = null;
      try { native.setActive(true); } catch (cleanup) { if (firstError === '') firstError = 'SET_ACTIVE_FAILED ' + String(cleanup); }
      const configResult = parseNativeObject(native.g9ResearchConfig(new Uint32Array([k, full ? 1 : 0])), 'g9ResearchConfig', ['T529_EXPLICIT_RESEARCH_CANDIDATE_1']);
      if (!configResult.ok) { this.message = 'G9_CONFIG_FAULT：' + configResult.error + (firstError === '' ? '' : '；清理：' + firstError); return; }
      const configWords: number[] | null = configResult.words;
      if (configWords === null || configWords.length !== 24) { this.message = 'CALIBRATION_CONFIG_REQUIRED'; return; }
      const reply: string = native.g9Configure(new Uint32Array(configWords));
      if (reply !== 'G9_NEW_EMPTY_RESEARCH_RUN_REQUESTED') {
        this.message = 'G9_CONFIG_FAULT：' + reply + (reply.indexOf('G9') === 0 ? '（可读诊断：native.gpuInputDiagnosticsV1(\'G9\')）' : '');
        return;
      }
      this.message = reply;
      this.identity = [];
      this.sequence = this.command = this.angle = 0;
      this.frame = new G9Frame();
      this.frame.owner = this;
      this.getUIContext().postFrameCallback(this.frame);
    } catch (e) {
      // Startup fault: no new pulse, no auto re-configure; first cause kept.
      if (this.frame !== null) this.frame.owner = null;
      this.frame = null;
      this.message = 'G9_START_FAULT：' + String(e) + (firstError === '' ? '' : '；清理：' + firstError);
    }
  }
  private startReal(): void {
    try {
      this.cold.cancel(); this.closeArchive(false); native.g9TouchCancel();
      if (this.frame !== null) this.frame.owner = null;
      this.frame = null; this.publication = '0'; this.token = ''; native.setActive(true);
      const config = parseNativeObject(native.g9ResearchConfig64(new Uint32Array([2])), 'g9ResearchConfig64', ['G9_CONFIG64_R2']);
      if (!config.ok || config.words === null || config.words.length !== 64) throw new Error('G9_CONFIG64_' + config.error);
      const result: string = native.g9Configure64(new Uint32Array(config.words));
      if (result !== 'G9_NEW_S40_REAL_TOUCH_R2_REQUESTED') throw new Error(result);
      this.realTouch = true; this.identity = []; this.sequence = this.command = this.angle = 0;
      this.touchMessage = 'REAL_TOUCH_R2：等待独立时钟资格及成功呈现映射';
      this.message = result; this.frame = new G9Frame(); this.frame.owner = this; this.getUIContext().postFrameCallback(this.frame);
    } catch (e) { this.realTouch = false; this.message = 'G9_REAL_TOUCH_START_FAULT：' + String(e); }
  }
  private publicationWords(words: Uint32Array): boolean {
    if (!/^[1-9][0-9]{0,19}$/.test(this.publication)) return false;
    let low: number = 0; let high: number = 0;
    for (let i: number = 0; i < this.publication.length; i++) {
      const expanded: number = low * 10 + Number(this.publication.charAt(i));
      high = high * 10 + Math.floor(expanded / 4294967296); low = expanded % 4294967296;
      if (high > 4294967295) return false;
    }
    words[24] = low; words[25] = high; return true;
  }
  private touch(event: TouchEvent): void {
    if (!this.realTouch) return;
    try {
      if (event.type === TouchType.Cancel) { this.touchMessage = native.g9TouchCancel(); return; }
      const action: number = event.type === TouchType.Down ? 0 : event.type === TouchType.Move ? 1 : event.type === TouchType.Up ? 2 : -1;
      if (action < 0 || event.changedTouches.length !== 1 || event.touches.length > 1) { native.g9TouchCancel(); this.touchMessage = 'MULTITOUCH_REJECTED'; return; }
      if (!Number.isSafeInteger(event.timestamp) || event.timestamp <= 0) { native.g9TouchCancel(); this.touchMessage = 'INPUT_TIMESTAMP_PRECISION_UNSUPPORTED'; return; }
      const point: TouchObject = event.changedTouches[0];
      const words = new Uint32Array(64); const view = new DataView(words.buffer);
      words[0] = action; words[1] = point.id; words[2] = event.changedTouches.length; words[3] = event.touches.length;
      view.setFloat64(4 * 4, event.timestamp, true);
      const scale: number = this.getUIContext().vp2px(1);
      const raw: number[] = [point.x, point.y, point.windowX, point.windowY, point.screenX, point.screenY, scale, (point.windowX - point.x) * scale, (point.windowY - point.y) * scale];
      for (let i: number = 0; i < raw.length; i++) { if (!Number.isFinite(raw[i])) throw new Error('TOUCH_POINT_INVALID'); view.setFloat64((6 + 2 * i) * 4, raw[i], true); }
      if (!this.publicationWords(words)) { native.g9TouchCancel(); this.touchMessage = 'STALE_PRESENTED_MAPPING'; return; }
      words[26] = words[27] = 1;
      this.touchMessage = native.g9TouchEventV2(words);
    } catch (e) { native.g9TouchCancel(); this.touchMessage = 'TOUCH_FAULT：' + String(e); }
  }
  tick(frame: G9Frame, ns: number): void {
    if (frame !== this.frame || frame.owner !== this || !Number.isSafeInteger(ns)) return;
    try {
      if (this.identity.length !== 18) {
        const sessionResult = parseNativeObject(native.g9Session(), 'g9Session', ['G9_SPF_Q20_V1']);
        if (!sessionResult.ok) throw new Error('G9_SESSION_' + sessionResult.error);
        const identityWords: Object | undefined = (JSON.parse(native.g9Session()) as Record<string, Object>)['identity'];
        if (Array.isArray(identityWords) && identityWords.length === 18) {
          const checked: number[] = [];
          for (let i = 0; i < identityWords.length; i++) {
            const w: Object = identityWords[i];
            if (typeof w !== 'number' || !Number.isSafeInteger(w) || w < 0 || w > 0xffffffff) throw new Error('G9_SESSION_IDENTITY_WORD_INVALID');
            checked.push(w);
          }
          this.identity = checked;
        }
      }
      if (this.identity.length === 18) {
        if (this.sequence >= Number.MAX_SAFE_INTEGER) throw new Error('G9_SEQUENCE_EXHAUSTED');
        const pose = new Uint32Array(19);
        pose.set(this.identity);
        pose[18] = this.angle;
        if (!this.realTouch) native.g9Target(pose); // Legacy explicit manual pose.
        const pulse = new Uint32Array(22);
        pulse.set(this.identity);
        this.pair(pulse, 18, ++this.sequence);
        this.pair(pulse, 20, ns);
        const code: string = native.g9Pulse(pulse);
        if (code !== '0') throw new Error('G9_PULSE_' + code);
        if (this.realTouch && native.g9Target(pose) !== '0') throw new Error('G9_MANUAL_POSE_REJECTED');
        this.message = native.status();
        if (this.realTouch && this.message.length <= 65536) {
          const status = JSON.parse(this.message) as Record<string, Object>;
          if (status['state'] === 'G9_CURRENT' && typeof status['token'] === 'string' && this.token !== (status['token'] as string)) {
            const token: string = status['token'] as string;
            const mappingRaw: string = native.g9PresentedMappingV2(token);
            const mapping = parseNativeObject(mappingRaw, 'g9PresentedMappingV2', ['G9_PRESENTED_MAPPING_V2']);
            if (mapping.ok) { const value = JSON.parse(mappingRaw) as Record<string, Object>; if (typeof value['generation'] === 'string') { this.publication = value['generation'] as string; this.token = token; } }
          }
        }
      }
      if (this.file !== null) {
        const state: string = this.cold.next((part: string) => {
          if (this.file === null) throw new Error('G9_FILE_CLOSED');
          if (fs.writeSync(this.file.fd, part) !== part.length) throw new Error('G9_SHORT_WRITE');
        });
        if (state !== 'REQUESTED' && state !== 'READING') this.closeArchive(state === 'COMPLETE');
      }
      this.getUIContext().postFrameCallback(frame);
    } catch (e) { frame.owner = null; this.cold.cancel(); this.closeArchive(false); this.message = 'G9_FAULT，需新建运行'; }
  }
  private offer(kind: number): void {
    if (this.realTouch) { this.touchMessage = 'REAL_TOUCH_R2：请在仿真区域拨动'; return; }
    if (this.identity.length !== 18 || this.command >= Number.MAX_SAFE_INTEGER) return;
    const words = new Uint32Array(148);
    words.set(this.identity);
    this.pair(words, 18, ++this.command);
    const at: number = 20;
    words[at + 40] = kind;
    this.pair(words, at + 41, this.command);
    const view = new DataView(words.buffer);
    if (kind === 1) {
      view.setFloat64((at + 64) * 4, 0.125, true);
      view.setFloat64((at + 66) * 4, 64.5, true);
      view.setFloat64((at + 68) * 4, 64, true);
      view.setFloat64((at + 70) * 4, 4, true);
      view.setFloat64((at + 72) * 4, 0, true);
      words[at + 74] = 1;
    } else {
      // Explicit model-domain impulse; no uncalibrated pixel-to-force gain.
      view.setFloat64((at + 46) * 4, 64.5, true);
      view.setFloat64((at + 48) * 4, 8, true);
      this.pair(words, at + 56, 1048576 * 1048576);
      this.pair(words, at + 58, 1048576 * 1048576);
      words[at + 63] = 1 << 20;
      words[at + 77] = 1;
    }
    this.message = native.g9OfferModel(words);
  }
  private exportCold(): void {
    if (this.file !== null || this.identity.length !== 18) return;
    try {
      const context = getContext(this) as common.UIAbilityContext;
      this.path = `${context.filesDir}/t529-g9-${Date.now()}.part.jsonl`;
      this.file = fs.openSync(this.path, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY | fs.OpenMode.TRUNC);
      this.archive = this.cold.begin(this.identity);
      if (this.archive !== 'REQUESTED') this.closeArchive(false);
    } catch (e) { this.cold.cancel(); this.closeArchive(false); }
  }
  aboutToDisappear(): void {
    if (this.frame !== null) this.frame.owner = null;
    try { this.cold.cancel(); this.closeArchive(false); }
    finally { native.g9TouchCancel(); this.publication = '0'; this.token = ''; native.setActive(false); }
  }
  build() {
    Column({ space: 6 }) {
      Text('G9 · S/F/P GPU 独立研究').fontSize(20)
      Text(this.realTouch ? 'REAL_TOUCH_R2 · 手动姿态 · 时钟资格独立核验 · 候选待定标' : '候选参数待定标；SYNTHETIC 模型输入；默认关闭').fontSize(12)
      Button('真实拨动 R2 · S40 新运行').onClick(() => this.startReal())
      Text(this.touchMessage).fontSize(10).maxLines(2)
      Row({ space: 6 }) {
        Button('K192 新运行').onClick(() => this.start(192, false))
        Button('K512 新运行').onClick(() => this.start(512, false))
        Button('FULL 新运行').onClick(() => this.start(192, true))
      }
      Row({ space: 6 }) {
        Button('模型掉沙').onClick(() => this.offer(1))
        Button('模型脉冲').onClick(() => this.offer(2))
        Button('冷导出').onClick(() => this.exportCold())
      }
      Slider({ value: this.angle, min: -7680, max: 7680, step: 1 }).onChange((value: number) => { this.angle = Math.round(value); })
      XComponent({ id: 'grs-surface', type: XComponentType.SURFACE, libraryname: 'grsm1' }).layoutWeight(1).width('100%').onTouch((event: TouchEvent) => this.touch(event))
      Text(this.message).fontSize(10).maxLines(3)
      Text(this.archive).fontSize(10).maxLines(2)
    }.width('100%').height('100%')
  }
}
