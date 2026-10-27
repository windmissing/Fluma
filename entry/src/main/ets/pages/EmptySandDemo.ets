import systemDateTime from '@ohos.systemDateTime';
import { P051TouchSourceAdapter, P051InputGeometry, P051TouchPoint } from '../physics/P051TouchSourceAdapter';
import { P013SceneController } from '../render/P013SceneController';
import { p012FactoryAppearance } from '../render/P012RenderTypes';
import { P051DiagnosticBuffer } from '../physics/P051DiagnosticBuffer';
import { P011TiltPreviewTrace } from '../physics/P011TiltPreviewTrace';
import sensor from '@ohos.sensor';
import { P051GravityInputAdapter, P051GravityConfig, P051GravityInput } from '../physics/P051GravityInputAdapter';
// T51 independent page: real field+pool and T49 product GLES, never a probe.
import { FrameCallback } from '@ohos.arkui.UIContext';
import { P051EmptySandDemo } from '../physics/P051EmptySandDemo';
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P013RenderController, p013NowMs } from '../render/P013RenderController';
import { P012FrameContext, P012AppearanceTier, P012Ablation, P012FillBox } from '../render/P012RenderTypes';
interface P051FailurePoolSummary { copyStatus: string; live: number; airborne: number; contact: number; }
interface P051FailureSensor { x: number; y: number; z: number; timeMs: number; }
interface P051FailurePayload {
  revision: string; status: string; dropped: number; runId: string; beforeCommit: number; afterCommit: number;
  dt: number; medium: string; input: P051GravityInput | null; actualSensorCount: number; actualSensor: P051FailureSensor | null;
  configRevision: string; effectiveTiltDeg: number; beforePool: P051FailurePoolSummary;
  sourceEpoch: number; externalAdmissionCount: number | null; pool: P051FailurePoolSummary; traces: P011TiltPreviewTrace[];
  poolProgress: string | null; beforePoolProgress: string | null; particleEventTail: string[];
}
interface P051FailureChunk { revision: string; runId: string; part: number; parts: number; complete: boolean; dropped: number; text: string; }
class EmptySandFrame extends FrameCallback {
  owner: EmptySandDemo | null = null;
  onFrame(time: number): void { if (this.owner !== null) this.owner.tick(time); }
}
@Entry
@Component
struct EmptySandDemo {
  private failureTraceEnabled: boolean = true;
  private failureTraceExported: boolean = false;
  private failureTraces: P011TiltPreviewTrace[] = [];
  private failureTraceDropped: number = 0;
  private failureV2ExportedId: string | null = null;
  private materialFailureExportedToken: string | null = null;
  private emitMaterialRejection(): void {
    try{
      const bundle=this.renderer.getLastMaterialRejection(),token=bundle?.firstFailure?.frameToken ?? null;
      if(bundle===null||token===null||token===this.materialFailureExportedToken)return;
      const text=JSON.stringify(bundle),parts=Math.min(64,Math.ceil(text.length/2048));
      for(let i=0;i<parts;i++)console.info('WS_EVENT scene_material_rejection '+JSON.stringify({revision:bundle.revision,frameToken:token,
        part:i+1,parts:parts,complete:text.length<=131072,totalCharacters:text.length,text:text.slice(i*2048,(i+1)*2048)}));
      this.materialFailureExportedToken=token;
    }catch(e){console.info('WS_EVENT scene_material_rejection EXPORT_FAILED');}
  }
  private emitPhysicsFailureV2(): void {
    try {
      const bundle=this.model.exportPhysicsFailureV2();
      const id=bundle?.firstFailure?.firstFailureId ?? null;
      if(bundle===null||id===null||id===this.failureV2ExportedId)return;
      console.info(`WS_EVENT first_physics_failure_v2 ${JSON.stringify(bundle)}`);
      this.failureV2ExportedId=id;
    }catch(e){console.info('WS_EVENT first_physics_failure_v2 EXPORT_FAILED');}
  }
  private armFailureTrace(): void {
    if(!this.failureTraceEnabled)return;
    this.model.field.setTiltPreviewTraceSink((trace: P011TiltPreviewTrace): void => {
      if(this.failureTraces.length>=2){this.failureTraces.shift();this.failureTraceDropped++;}
      this.failureTraces.push(trace);
    });
  }
  private resetDemo(): void {
    this.emitPhysicsFailureV2();
    this.touchSource.invalidate('RESET');this.gravity.reset();this.model.reset();
    this.model.field.setPoolProgressDiagnosticEnabled(true,(): number => systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,false));
    this.failureTraceExported=false;this.failureTraces.length=0;this.failureTraceDropped=0;this.armFailureTrace();
    this.failureV2ExportedId=null;
    this.publication=this.model.createPublication();this.previousNs=0;
  }
  private emitFirstCode5(input: P051GravityInput | null, dt: number, beforeCommit: number, beforePoolProgress: string | null): void {
    if (!this.failureTraceEnabled || this.failureTraceExported || this.model.field.getLastError() !== 5) return;
    this.failureTraceExported = true;
    this.model.field.setTiltPreviewTraceSink(null);
    const after = this.model.createPublication();
    const copy = this.model.field.copyPublishedRenderStateInto(after);
    const p = copy === 'OK' ? after.field.particleState : null;
    const prior=this.publication.field.particleState;
    let beforeContact=0,beforeAirborne=0;
    if(prior!==null&&prior!==undefined)for(let i=0;i<prior.nmax;i++){
      if(prior.pState[i]===2)beforeContact++;else if(prior.pState[i]===1)beforeAirborne++;
    }
    let contact = 0, airborne = 0;
    if (p !== null && p !== undefined) for (let i = 0; i < p.nmax; i++) {
      if (p.pState[i] === 2) contact++; else if (p.pState[i] === 1) airborne++;
    }
    const source=this.model.field.getDryDiagnostics();
    let allComplete=this.failureTraceDropped===0;
    for(const trace of this.failureTraces)if(trace.status!=='COMPLETE')allComplete=false;
    const bundle: P051FailurePayload = {revision:'P051_FIRST_CODE5_TRACE_V1',status:allComplete?'COMPLETE':'TRACE_INCOMPLETE',
      dropped:this.failureTraceDropped,runId:this.model.field.getP011RunId(),beforeCommit:beforeCommit,
      afterCommit:this.model.field.getRenderPhysicsCommitId(),dt:dt,medium:this.model.field.getMediumMode(),
      input:input,actualSensorCount:this.diagnosticSensorCount,actualSensor:this.lastRawSensor,
      configRevision:this.gravityConfig.revision,effectiveTiltDeg:after.effectiveTiltDeg,
      beforePool:{copyStatus:'LAST_PRESENTED',live:this.publication.liveCount,airborne:beforeAirborne,contact:beforeContact},
      sourceEpoch:source.sourceEpoch,
      externalAdmissionCount:source.externalAdmissionCount,
      pool:{copyStatus:copy,live:after.liveCount,airborne:airborne,contact:contact},traces:this.failureTraces,
      poolProgress:this.model.field.getPoolProgressDiagnostic(),beforePoolProgress:beforePoolProgress,
      particleEventTail:this.model.field.getParticleEventTail(8)};
    const payload = JSON.stringify(bundle);
    const size=2048, limit=64, emitted=Math.min(limit,Math.ceil(payload.length/size));
    if(payload.length>size*limit)bundle.status='TRACE_INCOMPLETE';
    const bounded=payload.length>size*limit?JSON.stringify(bundle):payload;
    for(let i=0;i<emitted;i++) { const chunk: P051FailureChunk = {revision:'P051_FIRST_CODE5_TRACE_V1',
      runId:this.model.field.getP011RunId(),part:i+1,parts:emitted,complete:payload.length<=size*limit,
      dropped:this.failureTraceDropped+(payload.length>size*limit?1:0),text:bounded.slice(i*size,(i+1)*size)};
      console.info(`WS_EVENT first_code5_trace ${JSON.stringify(chunk)}`); }
  }
  private diagnosticSensorCount: number = 0;
  private lastRawSensor: P051FailureSensor | null = null;
  private diagnostics: P051DiagnosticBuffer = new P051DiagnosticBuffer();
  private diagnostic(kind: string, time: number, source: string = '', x: number = 0, y: number = 0): void {
    this.diagnostics.record({kind:kind,time:time,runId:this.model.field.getP011RunId(),commit:this.model.field.getRenderPhysicsCommitId(),frame:this.serial,source:source,x:x,y:y,sensorCount:this.diagnosticSensorCount});
  }
  private flushDiagnostics(): void {
    for(let i=0;i<8;i++){const chunk=this.diagnostics.drain();if(chunk.rows.length===0)break;console.info(`WS_EVENT demo_diag ${JSON.stringify(chunk)}`);}
  }
  private gravityConfig: P051GravityConfig = new P051GravityConfig();
  private gravity: P051GravityInputAdapter = new P051GravityInputAdapter(this.gravityConfig);
  private sensorActive: boolean = false;
  private readonly accelerometer = (data: sensor.AccelerometerResponse): void => {
    if (!this.running) return;
    this.diagnosticSensorCount++;const now=p013NowMs();this.lastRawSensor={x:data.x,y:data.y,z:data.z,timeMs:now};this.diagnostic('sensor',now,'sensor',data.x,data.y);
    this.model.recordDiagnosticSensor(this.diagnosticSensorCount,data.x,data.y,data.z,now);
    this.gravity.consumeAccelerometer(data.x,data.y,data.z,now);
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
  private touchSource: P051TouchSourceAdapter = new P051TouchSourceAdapter(this.model,this.diagnostics);
  private inputGeometry: P051InputGeometry = {revision:'P051_TOUCH_INPUT_ENGINEERING_V1',geometryEpoch:1,widthVp:0,heightVp:0,ready:false};
  private renderer: P013SceneController = new P013SceneController();
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
  aboutToAppear(): void {
    this.callback.owner = this;this.model.diagnostics=this.diagnostics;
    this.model.field.setPoolProgressDiagnosticEnabled(true,(): number => systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,false));
    this.armFailureTrace();
    this.model.field.setGestureSandEventSink((receipt) => {
      this.diagnostics.record({kind:'wet.receipt',time:p013NowMs(),runId:this.model.field.getP011RunId(),commit:this.model.field.getRenderPhysicsCommitId(),frame:this.serial,admitted:receipt.admittedH+receipt.admittedL,reason:receipt.rejectedReason ?? 'ACCEPTED'});
    });
  }
  private syncActive(): void {
    this.running = this.pageShown && this.foreground; this.previousNs = 0;
    if(!this.running)this.touchSource.invalidate('PAGE_INACTIVE');
    this.model.setActive(this.running); this.renderer.setActive(this.running); this.updateSensor();
    if (this.running) this.schedule();
  }
  onPageShow(): void { this.pageShown = true; this.syncActive(); }
  onPageHide(): void { this.pageShown = false; this.syncActive(); }
  aboutToDisappear(): void { this.onPageHide(); this.callback.owner = null; this.renderer.release();this.flushDiagnostics();this.model.field.setGestureSandEventSink(null);this.model.field.setTiltPreviewTraceSink(null); }
  private schedule(): void { if (this.running && !this.queued) { this.queued = true; this.getUIContext().postFrameCallback(this.callback); } }
  tick(ns: number): void {
    this.queued = false; if (!this.running) return;
    const dt = this.previousNs === 0 ? 0 : Math.min(1/30,(ns-this.previousNs)/1e9); this.previousNs = ns;
    const input=this.gravity.latest(p013NowMs());
    if (input !== null) this.model.applyDefinedInput(input);
    this.model.diagnosticFrame=this.serial;
    this.failureTraces.length=0;this.failureTraceDropped=0;
    const beforeCommit=this.model.field.getRenderPhysicsCommitId();
    const beforePoolProgress=this.model.field.getPoolProgressDiagnostic();
    const ok = this.model.tick(ns,dt);this.touchSource.sync();
    if (!ok) { this.emitPhysicsFailureV2();this.emitFirstCode5(input,dt,beforeCommit,beforePoolProgress);this.model.stop('page-physics-failed');this.flushDiagnostics(); this.status = '物理提交失败：'+this.model.field.getLastError(); this.schedule(); return; }
    if (this.model.field.copyPublishedRenderStateInto(this.publication) === 'OK') {
      this.model.recordDiagnosticPublication(this.publication.runId,this.publication.physicsCommitId,this.serial,this.publication.effectiveTiltDeg);
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
      const result = this.renderer.drawNewPipelineFrame(this.publication,context,p012FactoryAppearance(),0,ns,this.getUIContext().vp2px(1));
      if(result!==null&&!result.currentFrameSucceeded)this.emitMaterialRejection();
      this.diagnostics.record({kind:'present',time:ns/1e6,runId:this.publication.runId,commit:this.publication.physicsCommitId,frame:this.serial,code:result?.code ?? -1,swap:result?.requestedSwapSucceeded ?? false,reason:this.renderer.failure});
      if (this.serial % 60 === 0) console.info(`WS_EVENT empty_sand_demo ${JSON.stringify({
        input:input,config:this.gravityConfig.revision,physicsCommitId:this.publication.physicsCommitId,
        effectiveTiltDeg:this.publication.effectiveTiltDeg,dry:this.model.field.getDryDiagnostics(),mediumMode:this.publication.mediumMode,metrics:this.model.field.getMetrics()})}`);
      this.status = result !== null && result.currentFrameSucceeded ?
        '新管线：按住发沙，松手停止' : result?.presentationState==='STALE' ? 'STALE '+(result.lastGoodFrameToken ?? '')+'；'+result.firstFailureStage : '新管线故障：'+this.renderer.failure;
    }
    this.flushDiagnostics();this.schedule();
  }
  private touch(event: TouchEvent): void {
    const touches: P051TouchPoint[]=[],changed: P051TouchPoint[]=[];
    for(const p of event.touches)touches.push({id:p.id,x:p.x,y:p.y});
    for(const p of event.changedTouches)changed.push({id:p.id,x:p.x,y:p.y});
    this.touchSource.handle({type:event.type===TouchType.Down?'DOWN':event.type===TouchType.Move?'MOVE':event.type===TouchType.Up?'UP':'CANCEL',
      touches:touches,changedTouches:changed,timestamp:event.timestamp},this.inputGeometry,this.running,this.serial);
    this.flushDiagnostics();
  }
  private measureInput(area: Area): void {
    const w=Number(area.width),h=Number(area.height);
    if(this.inputGeometry.ready && w===this.surfaceWidth && h===this.surfaceHeight)return;
    this.touchSource.invalidate('GEOMETRY_CHANGED');
    this.surfaceWidth=w;this.surfaceHeight=h;this.geometryEpoch++;
    this.inputGeometry={revision:'P051_TOUCH_INPUT_ENGINEERING_V1',geometryEpoch:this.geometryEpoch,widthVp:w,heightVp:h,
      ready:Number.isFinite(w)&&Number.isFinite(h)&&w>0&&h>0};
  }
  build() {
    Column({space:8}) {
      Text('空容器落沙 DEMO').fontSize(20)
      Text(this.status).fontSize(12)
      Text('倾斜/摇动影响重力方向；强度参数 NOT_CALIBRATED').fontSize(12)
      Button('重置为空场').onClick(() => this.resetDemo())
      Stack() {
        XComponent({id:'p013_lmat_surface',type:XComponentType.TEXTURE,libraryname:'p013renderer'})
          .width('100%').height('100%').onLoad(() => this.renderer.onLoad())
          .hitTestBehavior(HitTestMode.None)
        // Input-only node: no background, opacity, texture, or drawing.
        Stack() {}.width('100%').height('100%').hitTestBehavior(HitTestMode.Block)
          .onTouch((event: TouchEvent) => this.touch(event))
      }.onAreaChange((_old: Area, area: Area) => this.measureInput(area))
        .width('92%').layoutWeight(1)

    }.width('100%').height('100%').padding({top:16,bottom:24})
  }
}
