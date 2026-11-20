import native from 'libgrs_g9rx.so';
import { GpuG9RxCold } from '../physics/GpuG9RxCold';
import { common } from '@kit.AbilityKit';
import { FrameCallback } from '@ohos.arkui.UIContext';
interface RxConfig { words: number[]; }
interface RxConfigureReply { state: string; ticket?: string; }
interface RxConfigureStatus { ticket: string; phase: string; failure: string; result: number; firstEgl: number; firstGl: number; }
interface RxSession { scope: string[]; }
interface RxStatus { configured: boolean; active: boolean; initialization: string; sceneStale: boolean; capability: number[]; mapping: number[]; commit: string; configure: RxConfigureStatus; }
class RxFrame extends FrameCallback {
  owner: GpuResidentG9Rx | null = null;
  onFrame(ns: number): void { this.owner?.tick(this, ns); }
}
@Entry
@Component
struct GpuResidentG9Rx {
  @State private message: string = '原生回执粒子场研究 OFF';
  private cold: GpuG9RxCold = new GpuG9RxCold();
  private archive(): void { const context = getContext(this) as common.UIAbilityContext; this.cold.collect('ROOT', context.filesDir).then((path: string): void => { this.message = path; }).catch((error: Error): void => { this.message = error.message; }); }
  private frame: RxFrame | null = null;
  private session: string = '';
  private configureTicket: string = '';
  private sequence: number = 0;
  private researchActive: boolean = false;
  private sourceIntent: number = 0;
  private scopeWords: Uint32Array = new Uint32Array(18);
  private cap: number[] = [];
  private mapping: number[] = [];
  private u64(words: Uint32Array, at: number, text: string): void {
    let lo: number = 0; let hi: number = 0;
    for (let i: number = 0; i < text.length; i++) {
      const digit: number = text.charCodeAt(i) - 48;
      if (digit < 0 || digit > 9) throw new Error('BAD_U64');
      const next: number = lo * 10 + digit;
      hi = hi * 10 + Math.floor(next / 4294967296); lo = next % 4294967296;
      if (hi > 4294967295) throw new Error('U64_OVERFLOW');
    }
    words[at] = lo; words[at + 1] = hi;
  }
  private configure(): void {
    if (this.configureTicket.length > 0 && this.session.length === 0) return;
    const config = JSON.parse(native.g9RxResearchConfig(192)) as RxConfig;
    const context = getContext(this) as common.UIAbilityContext;
    const raw: string = native.g9RxConfigure(new Uint32Array(config.words), 'RESEARCH_ONLY', context.filesDir);
    const ack = JSON.parse(raw) as RxConfigureReply;
    this.message = raw;
    if (ack.state !== 'CONFIG_QUEUED' || ack.ticket === undefined) return;
    this.configureTicket = ack.ticket;
    this.researchActive = true; this.session = ''; this.sequence = 0;
    this.cap = []; this.mapping = [];
    if (this.frame === null) { const frame = new RxFrame(); frame.owner = this; this.frame = frame; this.getUIContext().postFrameCallback(frame); }
  }
  tick(frame: RxFrame, ns: number): void {
    if (frame !== this.frame || !this.researchActive) return;
    try {
      if (!Number.isSafeInteger(ns) || ns < 0) throw new Error('UI_CLOCK_UNQUALIFIED');
      const status = JSON.parse(native.g9RxStatus()) as RxStatus;
      const attempt: RxConfigureStatus = status.configure;
      if (attempt.ticket !== this.configureTicket) throw new Error('STALE_CONFIGURE_TICKET');
      if (attempt.phase !== 'APPLIED') {
        const stage: string = 'RX #' + attempt.ticket + ' ' + attempt.phase;
        if (attempt.phase === 'FAILED' || attempt.phase === 'CANCELLED') {
          this.stop();
          this.message = stage + ' ' + attempt.failure + ' result=' + attempt.result +
            ' EGL=' + attempt.firstEgl + ' GL=' + attempt.firstGl;
          return;
        }
        this.message = stage;
      } else if (status.initialization.indexOf('FAILED') >= 0) {
        this.stop();
        this.message = status.initialization;
        return;
      } else if (status.configured) {
        const latestSession: string = native.g9RxSession();
        if (this.session.length === 0 || latestSession !== this.session || !status.active) {
          this.session = latestSession; const scope = JSON.parse(this.session) as RxSession;
          if (scope.scope.length !== 9) throw new Error('BAD_SCOPE');
          for (let i: number = 0; i < 9; i++) this.u64(this.scopeWords, 2 * i, scope.scope[i]);
          this.message = native.g9RxSetActive(true, this.session);
          const activated: string = native.g9RxSession(); if (activated !== this.session) { this.session = activated; const renewed = JSON.parse(activated) as RxSession; for (let i: number = 0; i < 9; i++) this.u64(this.scopeWords, 2 * i, renewed.scope[i]); }
          this.sequence = 0;
        }
        const pulse = new Uint32Array(22); pulse.set(this.scopeWords);
        this.u64(pulse, 18, String(++this.sequence)); this.u64(pulse, 20, String(ns)); native.g9RxPulse(pulse);
        this.cap = status.capability; this.mapping = status.mapping;
        if (status.sceneStale) this.message = 'STALE：' + status.initialization;
        else if (this.sequence % 30 === 0) this.message = 'GNR1 commit=' + status.commit + '；设备效果未验证';
      }
      this.getUIContext().postFrameCallback(frame);
    } catch (error) { this.stop(); this.message = 'FAULT: ' + String(error); }
  }
  private cancelInput(): void {
    if (this.cap.length !== 64 || this.mapping.length !== 64) return;
    const cancel = new Uint32Array(64); cancel[0] = 1347899975; cancel[1] = 1; cancel[2] = 64; cancel[3] = 3; cancel[7] = 1;
    cancel.set(this.mapping.slice(10, 18), 30); cancel.set(this.cap.slice(16, 20), 38); cancel.set(this.cap.slice(6, 10), 42); cancel.set(this.mapping.slice(8, 10), 46); cancel.set(this.mapping.slice(48, 56), 48);
    native.g9RxTouchIngress(cancel);
  }
  private touch(event: TouchEvent): void {
    if (!this.researchActive || this.cap.length !== 64 || this.mapping.length !== 64) return;
    try {
      const raw = new Uint32Array(64); const view = new DataView(raw.buffer);
      raw[0] = 1347899975; raw[1] = 1; raw[2] = 64; raw[7] = 1;
      raw[3] = event.type === TouchType.Down ? 0 : event.type === TouchType.Move ? 1 : event.type === TouchType.Up ? 2 : 3;
      raw.set(this.mapping.slice(10, 18), 30); raw.set(this.cap.slice(16, 20), 38);
      raw.set(this.cap.slice(6, 10), 42); raw.set(this.mapping.slice(8, 10), 46); raw.set(this.mapping.slice(48, 56), 48);
      if (raw[3] !== 3) {
        if (event.changedTouches.length !== 1) throw new Error('CHANGED_POINT_REQUIRED');
        const point: TouchObject = event.changedTouches[0];
        raw[4] = point.id; raw[5] = event.changedTouches.length; raw[6] = event.touches.length;
        raw[10] = 1; view.setFloat64(8 * 4, event.timestamp, true);
        const coords: number[] = [point.x, point.y, point.windowX, point.windowY, point.screenX, point.screenY, this.getUIContext().vp2px(1), 0, 0];
        for (let i: number = 0; i < coords.length; i++) { if (!Number.isFinite(coords[i])) throw new Error('COORDINATE_UNAVAILABLE'); view.setFloat64((12 + 2 * i) * 4, coords[i], true); }
      }
      this.message = native.g9RxTouchIngress(raw);
    } catch (error) { this.cancelInput(); this.message = 'INPUT_REJECTED:' + String(error); }
  }
  private source(): void { this.message = native.g9RxSource('{"intent":"' + String(++this.sourceIntent) + '","sourceId":"2","mass":"0.2","x":"64","y":"200","vx":"0","vy":"0"}'); }
  private stop(): void {
    this.researchActive = false;
    if (this.configureTicket.length > 0 && this.session.length === 0) {
      this.message = native.g9RxCancelConfigure(this.configureTicket);
    }
    this.configureTicket = '';
    if (this.session.length > 0) { try { this.message = native.g9RxStop(this.session); } catch (error) { this.message = String(error); } }
    this.session = '';
    if (this.frame !== null) this.frame.owner = null; this.frame = null;
  }
  aboutToDisappear(): void { this.stop(); }
  onPageHide(): void { this.stop(); }
  build() {
    Column() {
      Row() {
        Button('启用研究').onClick(() => this.configure())
        Button('注入粒子').onClick(() => this.source())
        Button('停止').onClick(() => this.stop())
        Button('保存原件').onClick(() => this.archive())
      }.height(48)
      Text(this.message).fontSize(11).maxLines(3).height(44)
      XComponent({ id: 'g9rx', type: XComponentType.SURFACE, libraryname: 'grs_g9rx' })
        .width('100%').layoutWeight(1).onTouch((event: TouchEvent) => this.touch(event))
    }.width('100%').height('100%')
  }
}
