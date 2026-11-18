import { GpuInputAttempt } from '../physics/GpuInputAttempt';
import native from 'libgrsm1.so';
import { FrameCallback } from '@ohos.arkui.UIContext';
import fs from '@ohos.file.fs';
import systemDateTime from '@ohos.systemDateTime';
import { common } from '@kit.AbilityKit';
import { GpuM3Cold } from '../physics/GpuM3Cold';

interface M3Session {
  revision?: number;
  features?: number;
  identity?: number[];
  sourceControlGeneration?: number[];
}
class M3PageFrame extends FrameCallback {
  owner: GpuResidentM3 | null = null;
  onFrame(ns: number): void {
    if (this.owner !== null) this.owner.frameTick(this, ns);
  }
}

@Entry
@Component
struct GpuResidentM3 {
  @State message: string = 'M3 独立 V4：等待新管线';
  @State exportState: string = '';
  @State fullBranch: boolean = false;
  private frame: M3PageFrame | null = null;
  @State inputDiagnostic: string = '';
  private startupIntent: number = 0;
  private inputCall(api: string, words: Uint32Array, schema: string, invoke: (value: Uint32Array) => string): string {
    const context = getContext(this) as common.UIAbilityContext;
    return GpuInputAttempt.call('M3A', api, words, schema, context.filesDir, invoke, (state: string): void => { this.inputDiagnostic = state; });
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
  private sequence: number = 0;
  private intent: number = 0;
  private request: number = 0;
  private sourceGeneration: number[] = [1, 0];
  private cold: GpuM3Cold = new GpuM3Cold();
  private coldFile: fs.File | null = null;
  private coldPath: string = '';

  private endExport(complete: boolean): void {
    if (this.coldFile === null) return;
    fs.closeSync(this.coldFile.fd);
    this.coldFile = null;
    if (complete) {
      const final: string = this.coldPath.replace('.part.jsonl', '.jsonl');
      fs.renameSync(this.coldPath, final);
      this.exportState = final;
    } else this.exportState = 'INCOMPLETE ' + this.coldPath;
  }

  private exportCold(): void {
    if (this.coldFile !== null || this.identity.length !== 18) return;
    try {
      const context = getContext(this) as common.UIAbilityContext;
      this.coldPath = `${context.filesDir}/t522-m3-cold-${Date.now()}.part.jsonl`;
      this.coldFile = fs.openSync(this.coldPath, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY | fs.OpenMode.TRUNC);
      this.exportState = this.cold.begin(this.identity, this.fullBranch);
      if (this.exportState !== 'REQUESTED') this.endExport(false);
    } catch (e) {
      this.cold.cancel();
      this.endExport(false);
      this.exportState = 'INCOMPLETE';
    }
  }

  private pumpCold(): void {
    if (this.coldFile === null) return;
    try {
      const start: number = systemDateTime.getUptime(systemDateTime.TimeType.STARTUP, false);
      for (let i: number = 0; i < 4; i++) {
        const state: string = this.cold.next((part: string) => {
          if (this.coldFile === null) throw new Error('M3_COLD_FILE_CLOSED');
          fs.writeSync(this.coldFile.fd, part);
        });
        if (state !== 'REQUESTED' && state !== 'READING') { this.endExport(state === 'COMPLETE'); return; }
        if (systemDateTime.getUptime(systemDateTime.TimeType.STARTUP, false) - start >= 8) return;
      }
    } catch (e) { this.cold.cancel(); this.endExport(false); }
  }

  private pair(words: Uint32Array, at: number, value: number): void {
    words[at] = value % 4294967296;
    words[at + 1] = Math.floor(value / 4294967296);
  }

  private start(full: boolean = false): void {
    // T550 (problem T546 §4): startup transaction guarded; only the explicit
    // M3_NEW_EMPTY_RUN_REQUESTED success token arms the frame.
    const intent: number = ++this.startupIntent;
    this.stopFrame(); this.identity = [];
    try {
      this.fullBranch = full;
      this.cold.cancel();
      this.endExport(false);
      native.setActive(true);
      // Explicit candidate policy: index/s, water Q/s, and physical cold fields.
      // No default fallback is supplied when the required omega field is absent.
      const config = new Uint32Array([4, 15, 192, 64, 7680, 8388608,
        1048576, 65536, 1048576, 0, 0, 120, 524288, 1048576, 1, full ? 1 : 0]);
      const reply: string = this.inputCall('m3Configure', config, 'M3_CONFIG16_V4', (words: Uint32Array): string => native.m3Configure(words));
      this.message = reply;
      if (!this.requested(reply, 'M3_NEW_EMPTY_RUN_REQUESTED')) { native.setActive(false); return; }
      this.identity = [];
      this.sequence = 0;
      this.intent = 0;
      this.request = 0;
      GpuInputAttempt.waitSession((): string => native.m3Session(), '4', 15, (): boolean => intent === this.startupIntent,
        (identity: number[], source: number[]): void => { this.identity = identity; this.sourceGeneration = source; this.frame = new M3PageFrame(); this.frame.owner = this; this.getUIContext().postFrameCallback(this.frame); },
        (error: string): void => { this.startupFault(error); });
    } catch (e) {
      this.stopFrame();
      this.startupFault('M3_START_FAULT：' + String(e));
    }
  }

  frameTick(frame: M3PageFrame, ns: number): void {
    if (frame !== this.frame || frame.owner !== this || !Number.isSafeInteger(ns) || ns < 0) return;
    if (this.sequence >= Number.MAX_SAFE_INTEGER) { frame.owner = null; native.setActive(false); this.message = 'M3_SEQUENCE_EXHAUSTED'; return; }
    if (this.identity.length !== 18) {
      let session: M3Session | null = null;
      try { session = JSON.parse(native.m3Session()) as M3Session; } catch (e) { session = null; }
      if (session === null || session.revision !== 4 || session.features !== 15 || session.identity?.length !== 18) {
        this.getUIContext().postFrameCallback(frame);
        return;
      }
      this.identity = session.identity;
      this.sourceGeneration = session.sourceControlGeneration ?? [1, 0];
    }
    const words = new Uint32Array(22);
    words.set(this.identity);
    this.pair(words, 18, ++this.sequence);
    this.pair(words, 20, ns);
    const code: string = native.m3Pulse(words);
    this.message = code === '0' ? native.status() : `M3 pulse ${code}；请新建运行`;
    this.pumpCold();
    if (code === '0') this.getUIContext().postFrameCallback(frame);
    else frame.owner = null;
  }

  private target(index: number): void {
    if (this.identity.length !== 18) return;
    const words = new Uint32Array(19);
    words.set(this.identity);
    words[18] = index;
    this.message = native.m3Target(words);
  }

  private source(kind: number): void {
    if (this.identity.length !== 18) return;
    if (this.intent >= Number.MAX_SAFE_INTEGER || this.request >= Number.MAX_SAFE_INTEGER) { this.message = 'M3_SOURCE_SEQUENCE_EXHAUSTED'; return; }
    const words = new Uint32Array(566);
    words.set(this.identity);
    words[18] = this.sourceGeneration[0];
    words[19] = this.sourceGeneration[1];
    this.pair(words, 20, ++this.intent);
    const at = 22;
    words[at] = 1129534023;
    words[at + 1] = 4;
    words[at + 2] = kind;
    words[at + 4] = this.identity[0];
    words[at + 5] = this.identity[1];
    this.pair(words, at + 6, ++this.request);
    words[at + 8] = this.identity[12];
    words[at + 9] = this.identity[13];
    words[at + 10] = this.identity[2];
    words[at + 11] = this.identity[3];
    words[at + 12] = this.identity[16];
    words[at + 13] = this.identity[17];
    words[at + 14] = 1;
    words[at + 15] = 1;
    this.pair(words, at + 24, this.request);
    words[at + 28] = this.identity[14];
    words[at + 29] = this.identity[15];
    const view = new DataView(words.buffer);
    view.setFloat64((at + 16) * 4, 64.5, true);
    view.setFloat64((at + 18) * 4, 0.5, true);
    view.setFloat64((at + 20) * 4, kind === 1 ? 8 : 0, true);
    if (kind === 2) view.setFloat64((at + 32 + 64 * 4) * 4, 0.25, true);
    this.message = native.m3Offer(words);
  }

  aboutToDisappear(): void {
    ++this.startupIntent;
    try { this.cold.cancel(); this.endExport(false); }
    catch (cleanup) { this.inputDiagnostic = 'CLEANUP_FAILED:' + String(cleanup); }
    finally { this.startupFault(this.message); }
  }

  build() {
    Column({ space: 8 }) {
      Text('GPU M3 · V4').fontSize(20)
      Row({ space: 8 }) {
        Button('新建运行').onClick(() => this.start())
        Button('完整分支新运行').onClick(() => this.start(true))
        Button('加水').onClick(() => this.source(1))
        Button('湿沙').onClick(() => this.source(2))
      }
      Button('导出 V4 冷快照').onClick(() => this.exportCold())
      Row({ space: 8 }) {
        Button('−15°').onClick(() => this.target(-3840))
        Button('0°').onClick(() => this.target(0))
        Button('+15°').onClick(() => this.target(3840))
      }
      XComponent({ id: 'grsM3V4', type: XComponentType.SURFACE, libraryname: 'grsm1' })
        .width('100%').layoutWeight(1)
      Text(this.inputDiagnostic).fontSize(10)
      Text(this.message).fontSize(11).maxLines(4)
      Text(this.exportState).fontSize(10).maxLines(2)
    }.width('100%').height('100%').padding(12)
  }
}
