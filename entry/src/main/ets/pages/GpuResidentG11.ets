import native from 'libgrs_g11.so';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { common } from '@kit.AbilityKit';
import { G11ConfigureAttempt, G11ConfigureReply, GpuG11Port, G11Status } from '../physics/GpuG11Port';
import { GpuG11Cold } from '../physics/GpuG11Cold';
class G11Frame extends FrameCallback {
 owner: GpuResidentG11 | null = null;
 onFrame(ns: number): void { this.owner?.tick(this, ns); }
}
@Entry
@Component
struct GpuResidentG11 {
 @State private parent: string = 'R1';
 @State private k: number = 192;
 @State private v: number = 2048;
 @State private full: boolean = false;
 @State private message: string = 'G11 OFF';
 @State private controllerState: string = '';
 @State private configureState: string = '';
 @State private archiveState: string = '';
 @State private archiveFirstError: string = '';
 @State private lifecycle: string = '';
 @State private rawReady: boolean = false;
 @State private sealed: boolean = false;
 private port: GpuG11Port = new GpuG11Port();
 private cold: GpuG11Cold = new GpuG11Cold();
 private frame: G11Frame | null = null;
 private sequence: number = 0;
 private scope: string = '{}';
 private timer: number = -1;
 private configureReply: G11ConfigureReply | null = null;
 private archiveRequest: number = 0;
 private configure(): void {
  if (!this.stop('RECONFIGURE')) return;
  this.rawReady = this.sealed = false;
  this.configureReply = this.port.configure(this.parent, this.k, this.v, this.full);
  console.info('G11_CONFIGURE_RAW ' + JSON.stringify(this.configureReply));
  this.showConfigure(this.configureReply, undefined);
 }
 private showConfigure(sent: G11ConfigureReply, row: G11ConfigureAttempt | undefined): void {
  const initial = sent.raw !== '' ? sent.raw : sent.presetRaw;
  let text = sent.profile + ' · ' + sent.elements + ' 元素 / ' + sent.bytes + ' B @' + sent.offset + '\n' + initial;
  if (sent.error !== '') text += '\n' + sent.error;
  if (row !== undefined) {
   text += '\n' + row.stage;
   if (row.firstFailure !== null) text += '\n首错 ' + JSON.stringify(row.firstFailure);
   if (row.ownerResult !== null) text += '\n' + JSON.stringify(row.ownerResult);
   if (row.termination !== null) text += '\n' + JSON.stringify(row.termination);
  }
  this.configureState = text;
 }
 private refresh(): void {
  try {
   const status = this.port.status();
   this.scope = JSON.stringify(status.scope);
   const sent = this.configureReply;
   const row = sent === null || sent.attempt === '' ? undefined :
    status.configureAttempts?.find((attempt: G11ConfigureAttempt): boolean => attempt.attempt === sent.attempt);
   const sameScope = sent === null || (row !== undefined && row.scope !== null && JSON.stringify(row.scope) === this.scope);
   this.rawReady = sameScope && status.bootstrapRawReady;
   this.sealed = sameScope && status.bootstrapSealed;
   if (sent !== null) this.showConfigure(sent, row);
   this.controllerState = status.state + '\n' + (status.presentedMapping === null ? '等待有效新管线帧' : JSON.stringify(status.presentedMapping));
   if (status.lifecycle !== undefined && status.lifecycle !== null) this.lifecycle = JSON.stringify(status.lifecycle);
  } catch (error) { this.controllerState = String(error); }
 }
 private activate(): void {
  if (!this.sealed) { this.message = native.g11BootstrapSeal(); return; }
  const status = this.port.status(); this.scope = JSON.stringify(status.scope);
  native.g11BeginSession(this.scope); native.g11SetActive(true, this.scope); this.sequence = 0;
  const frame = new G11Frame(); frame.owner = this; this.frame = frame; this.getUIContext().postFrameCallback(frame);
 }
 tick(frame: G11Frame, ns: number): void {
  if (frame !== this.frame) return;
  if (!Number.isSafeInteger(ns) || ns < 0 || !Number.isSafeInteger(++this.sequence)) { this.stop('PAGE_FAULT'); this.message = 'UI_CLOCK_NOT_EXACT'; return; }
  try { const s = this.port.status(); native.g11PagePulse(s.scope.pageSession, String(this.sequence), String(ns)); this.getUIContext().postFrameCallback(frame); }
  catch (error) { this.stop('PAGE_FAULT'); this.message = String(error); }
 }
 private stop(source: string): boolean {
  if (this.frame !== null) this.frame.owner = null;
  this.frame = null; this.cold.cancel();
  if (this.scope !== '{}') {
   try { this.lifecycle = source + ' ' + native.g11Stop(this.scope, source); }
   catch (error) { this.lifecycle = source + ' ' + String(error); return false; }
  }
  return true;
 }
 private archiveProgress(view: number, stage: string, raw: string): void {
  console.info('G11_WORKER_RECEIPT ' + stage + ' ' + raw);
  if (view !== this.archiveRequest) return;
  this.archiveState = stage + (raw === '' ? '' : ' ' + raw);
  if (stage.endsWith(':NATIVE_REJECTED') && this.archiveFirstError === '') this.archiveFirstError = raw;
 }
 private archive(kind: string): void {
  const view = ++this.archiveRequest;
  this.archiveFirstError = '';
  try {
   const context = getContext(this) as common.UIAbilityContext;
   const status: G11Status = this.port.status();
   const token: string = status.token === null ? '' : JSON.stringify(status.token);
   this.cold.collect(kind, token, this.full ? 'FULL' : 'ROOT', context.filesDir,
    (stage: string, raw: string): void => { this.archiveProgress(view, stage, raw); })
    .catch((error: Error): void => { if (view === this.archiveRequest) this.archiveState = error.message; });
  } catch (error) { this.archiveState = String(error); }
 }
 private probeWorker(): void {
  const view = ++this.archiveRequest;
  this.archiveFirstError = '';
  this.cold.probe((stage: string, raw: string): void => { this.archiveProgress(view, stage, raw); })
   .catch((error: Error): void => { if (view === this.archiveRequest) this.archiveState = error.message; });
 }
 aboutToAppear(): void { this.timer = setInterval(() => { this.refresh(); }, 250); }
 aboutToDisappear(): void { if (this.timer >= 0) clearInterval(this.timer); this.stop('PAGE_HIDE'); }
 build() {
  Column() {
   Text('G11 · 接受通量组合研究').fontSize(18)
   XComponent({ id: 'g11-research', type: XComponentType.SURFACE, libraryname: 'grs_g11' }).width('100%').height('58%')
   Row() {
    Button(this.parent).onClick(() => { this.parent = this.parent === 'R1' ? 'DM' : 'R1'; })
    Button('K ' + this.k).onClick(() => { this.k = this.k === 192 ? 512 : 192; })
    Button('V ' + this.v).onClick(() => { this.v = this.v === 2048 ? 8192 : 2048; })
    Button(this.full ? 'FULL' : 'ROOT').onClick(() => { this.full = !this.full; })
   }
   Row() {
    Button('配置').onClick(() => { this.configure(); })
    Button(this.sealed ? '激活' : '封存 seed').enabled(this.rawReady).onClick(() => { this.activate(); })
    Button('停止').onClick(() => { this.stop('USER_STOP'); })
   }
   Row() {
    Button('测试源').onClick(() => { try { this.message = this.port.source(); } catch (error) { this.message = String(error); } })
    Button('倾斜').onClick(() => { this.message = native.g11Target(2560, this.scope); })
    Button('seed 原件').enabled(this.rawReady && !this.sealed).onClick(() => { this.archive('BOOTSTRAP'); })
    Button('冷档').onClick(() => { this.archive('COLD'); })
    Button('Worker 入口').onClick(() => { this.probeWorker(); })
   }
   Scroll() {
    Column() {
     Text(this.configureState).fontSize(10)
     Text(this.controllerState).fontSize(10)
     Text(this.lifecycle).fontSize(10)
     Text(this.archiveFirstError).fontSize(10)
     Text(this.archiveState).fontSize(10)
     Text(this.message).fontSize(10)
    }.width('100%')
   }.layoutWeight(1)
  }.width('100%').height('100%')
 }
}
