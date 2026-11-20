import native from 'libgrs_g13.so';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { G13ResearchConfig } from '../physics/G13ResearchConfig';
import { GpuG13Cold } from '../physics/GpuG13Cold';
import { GpuG13Input } from '../physics/GpuG13Input';
import { common } from '@kit.AbilityKit';
interface G13Scope { session: string; }
interface G13Status { scope: G13Scope; native: G13Native; initialization: G13Initialization; }
interface G13Initialization { currentLifecycle: string; firstConfigFailure: string; queuedTicket: string; }
interface G13SessionReply { state: string; }
interface G13Native { commit: string; fieldToken: string; coldToken: string; }
class G13Frame extends FrameCallback {
  owner: GpuResidentG13 | null = null;
  onFrame(ns: number): void { this.owner?.tick(this, ns); }
}
@Entry
@Component
struct GpuResidentG13 {
  @State private message: string = 'G13 流场研究 OFF；设备资格与物理指标尚未验证';
  @State private ready: boolean = false;
  @State private firstConfigFailure: string = '';
  @State private initStatus: string = 'UNCONFIGURED';
  private statusTimer: number = -1;
  private frame: G13Frame | null = null;
  private sequence: number = 0;
  private scope: string = '{}';
  private cold: GpuG13Cold = new GpuG13Cold();
  private input: GpuG13Input = new GpuG13Input();
  private source(opcode: number): void { this.input.offer(opcode).then((reply: string): void => { this.message = reply; }).catch((error: Error): void => { this.message = 'SOURCE_REJECTED:' + error.message; }); }
  private view(mode: number): void { this.input.view(mode).then((reply: string): void => { this.message = reply; }).catch((error: Error): void => { this.message = error.message; }); }
  tick(frame: G13Frame, ns: number): void {
    if (frame !== this.frame) return;
    if (!Number.isSafeInteger(ns) || ns < 0) { this.stop(); this.message = 'UI_CLOCK_NOT_EXACT'; return; }
    try {
      const raw: string = native.g13Status(); const status = JSON.parse(raw) as G13Status;
      this.scope = JSON.stringify(status.scope);
      if (!this.ready) { this.getUIContext().postFrameCallback(frame); return; }
      native.g13PagePulse(status.scope.session, String(++this.sequence), String(Math.floor(ns)));
      if (this.sequence % 30 === 0 && this.firstConfigFailure.length === 0) this.message = raw;
      this.getUIContext().postFrameCallback(frame);
    } catch (e) { this.stop(); this.message = 'FAULT:' + String(e); }
  }
  private start(): void {
    this.ready = false;
    const context = getContext(this) as common.UIAbilityContext;
    this.message = native.g13BeginSession(JSON.stringify({ filesDir: context.filesDir }));
    if ((JSON.parse(this.message) as G13SessionReply).state !== 'SESSION') return;
    this.message = native.g13Configure(G13ResearchConfig.words(), G13ResearchConfig.seed());
    // Configuration is asynchronous. A later explicit activation takes the
    // actual native scope rather than guessing future run/context IDs.
  }
  private activate(): void {
    if (!this.ready) return;
    const status = JSON.parse(native.g13Status()) as G13Status;
    this.scope = JSON.stringify(status.scope); this.message = native.g13SetActive(true, this.scope);
    this.sequence = 0; const frame = new G13Frame(); frame.owner = this; this.frame = frame;
    this.getUIContext().postFrameCallback(frame);
  }
  private stop(): void { if (this.frame !== null) this.frame.owner = null; this.frame = null; this.cold.cancel(); this.message = native.g13Stop(this.scope); }
  private sampleInit(): void {
    try { const status = JSON.parse(native.g13Status()) as G13Status;
      this.initStatus = JSON.stringify(status.initialization);
      this.ready = status.initialization.currentLifecycle === 'COMPUTE_READY';
      if (status.initialization.firstConfigFailure.length > 0) this.firstConfigFailure = status.initialization.firstConfigFailure;
    } catch (error) { this.ready = false; this.initStatus = 'INIT_STATUS_REJECTED ' + String(error); }
  }
  aboutToAppear(): void { this.statusTimer = setInterval(() => { this.sampleInit(); }, 250); }
  aboutToDisappear(): void { if (this.statusTimer >= 0) clearInterval(this.statusTimer); this.stop(); }
  private archive(): void {
    const context = getContext(this) as common.UIAbilityContext;
    const status = JSON.parse(native.g13Status()) as G13Status;
    this.cold.collect(status.native.coldToken, context.filesDir).then((path: string): void => { this.message = path; }).catch((e: Error): void => { this.message = 'INCOMPLETE:' + e.message; });
  }
  build() {
    Column() {
      Text('G13 · 纯流场独立研究').fontSize(18)
      XComponent({ id: 'g13-research', type: XComponentType.SURFACE, libraryname: 'grs_g13' }).height('65%').width('100%')
      Row() {
        Button('配置 REST').onClick(() => { this.start(); })
        Button('激活').enabled(this.ready).onClick(() => { this.activate(); })
        Button('停止').onClick(() => { this.stop(); })
        Button('冷档').onClick(() => { this.archive(); })
      }
      Row() {
        Button('重力改向').onClick(() => { this.source(1); })
        Button('四秒 RAMP').onClick(() => { this.source(2); })
        Button('一秒涡旋力').onClick(() => { this.source(3); })
      }
      Row() {
        Button('速度').onClick(() => { this.view(0); })
        Button('示踪').onClick(() => { this.view(1); })
        Button('涡量').onClick(() => { this.view(2); })
        Button('动压').onClick(() => { this.view(3); })
        Button('总压').onClick(() => { this.view(4); })
      }
      Text(this.initStatus).fontSize(9).maxLines(3)
      Text(this.firstConfigFailure).fontSize(9).maxLines(3)
      Text(this.message).fontSize(10).maxLines(8)
    }.width('100%').height('100%')
  }
}
