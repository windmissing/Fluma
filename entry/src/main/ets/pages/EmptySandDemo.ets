import sensor from '@ohos.sensor';
import { P051GravityInputAdapter, P051GravityConfig } from '../physics/P051GravityInputAdapter';
// T51 independent page: real field+pool and T49 product GLES, never a probe.
import { FrameCallback } from '@ohos.arkui.UIContext';
import { P051EmptySandDemo } from '../physics/P051EmptySandDemo';
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P013RenderController, p013NowMs } from '../render/P013RenderController';
import { P012FrameContext, P012AppearanceTier, P012Ablation, P012FillBox } from '../render/P012RenderTypes';
class EmptySandFrame extends FrameCallback {
  owner: EmptySandDemo | null = null;
  onFrame(time: number): void { if (this.owner !== null) this.owner.tick(time); }
}
@Entry
@Component
struct EmptySandDemo {
  private gravityConfig: P051GravityConfig = new P051GravityConfig();
  private gravity: P051GravityInputAdapter = new P051GravityInputAdapter(this.gravityConfig);
  private sensorActive: boolean = false;
  private readonly accelerometer = (data: sensor.AccelerometerResponse): void => {
    if (!this.running) return;
    this.gravity.consumeAccelerometer(data.x,data.y,data.z,p013NowMs());
  };
  private updateSensor(): void {
    if (this.running && !this.sensorActive) {
      try { sensor.on(sensor.SensorId.ACCELEROMETER,this.accelerometer,{interval:this.gravityConfig.sensorIntervalNs});this.sensorActive=true;
        console.info(`WS_EVENT demo_gravity_config ${JSON.stringify(this.gravityConfig)}`);
      } catch (e) { this.status='传感器不可用；竖直重力/触摸仍可用'; }
    } else if (!this.running && this.sensorActive) {
      try { sensor.off(sensor.SensorId.ACCELEROMETER,this.accelerometer); } catch (e) {}
      this.sensorActive=false;this.gravity.reset();
    }
  }
  private model: P051EmptySandDemo = new P051EmptySandDemo();
  private renderer: P013RenderController = new P013RenderController();
  private publication: P011PublishedRenderState = this.model.createPublication();
  private callback: EmptySandFrame = new EmptySandFrame();
  @StorageLink('emptySandDemoForeground') @Watch('syncActive') private foreground: boolean = true;
  private pageShown: boolean = false;
  private running: boolean = false; private queued: boolean = false;
  private geometryEpoch: number = 1;
  private previousNs: number = 0; private serial: number = 0;
  @State private surfaceWidth: number = 320;
  @State private surfaceHeight: number = 480;
  @State private status: string = '等待新管线';
  aboutToAppear(): void { this.callback.owner = this; }
  private syncActive(): void {
    this.running = this.pageShown && this.foreground; this.previousNs = 0;
    this.model.setActive(this.running); this.renderer.setActive(this.running); this.updateSensor();
    if (this.running) this.schedule();
  }
  onPageShow(): void { this.pageShown = true; this.syncActive(); }
  onPageHide(): void { this.pageShown = false; this.syncActive(); }
  aboutToDisappear(): void { this.onPageHide(); this.callback.owner = null; this.renderer.release(); }
  private schedule(): void { if (this.running && !this.queued) { this.queued = true; this.getUIContext().postFrameCallback(this.callback); } }
  tick(ns: number): void {
    this.queued = false; if (!this.running) return;
    const dt = this.previousNs === 0 ? 0 : Math.min(1/30,(ns-this.previousNs)/1e9); this.previousNs = ns;
    const input=this.gravity.latest(p013NowMs());
    if (input !== null) this.model.applyDefinedInput(input);
    const ok = this.model.tick(ns,dt);
    if (!ok) { this.model.stop(); this.status = '物理提交失败：'+this.model.field.getLastError(); this.schedule(); return; }
    if (this.model.field.copyPublishedRenderStateInto(this.publication) === 'OK') {
      const context: P012FrameContext = {
        physicsCommitId:this.publication.physicsCommitId,particleLayerEnabled:true,
        fieldEpoch:this.publication.stateEpoch,continuityEpoch:1,geometryEpoch:this.geometryEpoch,frameSerial:++this.serial,
        sampleTimeMs:ns/1e6,frameDeltaMs:dt*1000,inputMode:'EXPERIENCE',sourceKind:'gesture',
        physicalRouteId:'dual-field-particle-v1',runIdentity:this.publication.runId,
        boxX:0,boxY:0,boxW:this.surfaceWidth,boxH:this.surfaceHeight,pixelRatio:1,visualInverted:false,
        appearanceTier:P012AppearanceTier.OFF,renderSeed:51,configRevision:1,ablation:P012Ablation.OFF,
        fillBox:P012FillBox.TIGHT,canonicalEvidence:false,
        gravityDirX:Math.sin(this.publication.effectiveTiltDeg*Math.PI/180),
        gravityDirY:Math.cos(this.publication.effectiveTiltDeg*Math.PI/180),sandTracerEnabled:false
      };
      const result = this.renderer.submitDualFieldSurfacePx(this.publication,context,ns,this.getUIContext().vp2px(1));
      if (this.serial % 60 === 0) console.info(`WS_EVENT empty_sand_demo ${JSON.stringify({
        input:input,config:this.gravityConfig.revision,physicsCommitId:this.publication.physicsCommitId,
        effectiveTiltDeg:this.publication.effectiveTiltDeg,metrics:this.model.field.getMetrics()})}`);
      this.status = result !== null && result.code === 0 && result.swapSucceeded ?
        '新管线：按住发沙，松手停止' : '新管线等待/拒绝：'+this.renderer.dualFailure;
    }
    this.schedule();
  }
  private touch(event: TouchEvent): void {
    if (event.type === TouchType.Up || event.type === TouchType.Cancel) { this.model.stop(); return; }
    if (event.touches.length === 0) return;
    const point = event.touches[0];
    const x = point.x/this.surfaceWidth*128, y = (1-point.y/this.surfaceHeight)*256;
    if (event.type === TouchType.Down) this.model.start(x,y);
    else if (event.type === TouchType.Move) this.model.move(x,y);
  }
  build() {
    Column({space:8}) {
      Text('空容器落沙 DEMO').fontSize(20)
      Text(this.status).fontSize(12)
      Text('倾斜/摇动影响重力方向；强度参数 NOT_CALIBRATED').fontSize(12)
      Button('重置为空场').onClick(() => { this.gravity.reset();this.model.reset(); this.publication=this.model.createPublication(); this.previousNs=0; })
      Stack() {
        XComponent({id:'p013_lmat_surface',type:XComponentType.TEXTURE,libraryname:'p013renderer'})
          .width('100%').height('100%').onLoad(() => this.renderer.onLoad())
          .onAreaChange((_old: Area, area: Area) => { this.surfaceWidth=Number(area.width);this.surfaceHeight=Number(area.height);this.geometryEpoch++; })
          .onTouch((event: TouchEvent) => this.touch(event))
      }.width('92%').layoutWeight(1).backgroundColor('#E7E4DC')

    }.width('100%').height('100%').padding({top:16,bottom:24}).backgroundColor('#F6F3EC')
  }
}
