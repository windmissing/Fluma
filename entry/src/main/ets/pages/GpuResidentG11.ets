import native from 'libgrs_g11.so';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { common } from '@kit.AbilityKit';
import { GpuG11Port, G11Status } from '../physics/GpuG11Port';
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
 @State private rawReady: boolean = false;
 @State private sealed: boolean = false;
 private port: GpuG11Port = new GpuG11Port();
 private cold: GpuG11Cold = new GpuG11Cold();
 private frame: G11Frame | null = null;
 private sequence: number = 0;
 private scope: string = '{}';
 private timer: number = -1;
 private configure(): void {
  this.stop(); this.rawReady = this.sealed = false;
  try { this.message = this.port.configure(this.parent, this.k, this.v, this.full); } catch (error) { this.message = String(error); }
 }
 private refresh(): void {
  try { const status = this.port.status(); this.scope = JSON.stringify(status.scope); this.rawReady = status.bootstrapRawReady; this.sealed = status.bootstrapSealed;
   this.message = status.state + '\n' + (status.presentedMapping === null ? '等待有效新管线帧' : JSON.stringify(status.presentedMapping));
  } catch (error) { this.message = String(error); }
 }
 private activate(): void {
  if (!this.sealed) { this.message = native.g11BootstrapSeal(); return; }
  const status = this.port.status(); this.scope = JSON.stringify(status.scope);
  native.g11BeginSession(this.scope); native.g11SetActive(true, this.scope); this.sequence = 0;
  const frame = new G11Frame(); frame.owner = this; this.frame = frame; this.getUIContext().postFrameCallback(frame);
 }
 tick(frame: G11Frame, ns: number): void {
  if (frame !== this.frame) return;
  if (!Number.isSafeInteger(ns) || ns < 0 || !Number.isSafeInteger(++this.sequence)) { this.stop(); this.message = 'UI_CLOCK_NOT_EXACT'; return; }
  try { const s = this.port.status(); native.g11PagePulse(s.scope.pageSession, String(this.sequence), String(ns)); this.getUIContext().postFrameCallback(frame); }
  catch (error) { this.stop(); this.message = String(error); }
 }
 private stop(): void { if (this.frame !== null) this.frame.owner = null; this.frame = null; this.cold.cancel(); if (this.scope !== '{}') this.message = native.g11Stop(this.scope); }
 private archive(kind: string): void {
  const context = getContext(this) as common.UIAbilityContext;
  const status: G11Status = this.port.status();
  const token: string = status.token === null ? '' : JSON.stringify(status.token);
  this.cold.collect(kind, token, this.full ? 'FULL' : 'ROOT', context.filesDir).then((path: string): void => { this.message = path; }).catch((error: Error): void => { this.message = 'INCOMPLETE:' + error.message; });
 }
 aboutToAppear(): void { this.timer = setInterval(() => { this.refresh(); }, 250); }
 aboutToDisappear(): void { if (this.timer >= 0) clearInterval(this.timer); this.stop(); }
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
    Button('停止').onClick(() => { this.stop(); })
   }
   Row() {
    Button('测试源').onClick(() => { try { this.message = this.port.source(); } catch (error) { this.message = String(error); } })
    Button('倾斜').onClick(() => { this.message = native.g11Target(2560, this.scope); })
    Button('seed 原件').enabled(this.rawReady && !this.sealed).onClick(() => { this.archive('BOOTSTRAP'); })
    Button('冷档').onClick(() => { this.archive('COLD'); })
   }
   Text(this.message).fontSize(10).maxLines(6)
  }.width('100%').height('100%')
 }
}
