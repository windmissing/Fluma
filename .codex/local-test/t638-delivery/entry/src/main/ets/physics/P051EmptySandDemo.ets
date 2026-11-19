import { P051DiagnosticBuffer, P051FailureStoreV2, P051FailureExportV2, P051PhysicsFailureV2 } from './P051DiagnosticBuffer';
import { P051DryAdmissionRequest, P051_DRY_REVISION } from './P051DryParticleAdmission';
import { P051GravityInput } from './P051GravityInputAdapter';
// T51 isolated demo model. Sensor direction mapping is owned by T50.
import { WaterSandField } from './WaterSandField';
import { defaultP011ParticleParams } from './P011ParticleM2Field';
import { P011GestureRateClock } from './P011GestureRateClock';
import { P011PublishedRenderState } from './P011PublishedRenderState';
export interface P051StartReceipt { accepted: boolean; reason: string; stopGeneration: number; }
export class P051EmptySandDemo {
  private stopped: number = 0;
  get sourceHeld(): boolean { return this.touching; }
  get stopGeneration(): number { return this.stopped; }
  diagnostics: P051DiagnosticBuffer | null = null;
  diagnosticFrame: number = 0; private diagnosticTime: number = 0; private diagnosticStaleCount: number = 0;
  private observe(kind: string, reason: string = '', x: number = this.x, y: number = this.y): void {
    this.diagnostics?.record({kind:kind,runId:this.field.getP011RunId(),commit:this.field.getRenderPhysicsCommitId(),
      frame:this.diagnosticFrame,time:this.diagnosticTime,x:x,y:y,reason:reason});
  }
  readonly field: WaterSandField = new WaterSandField(128,256);
  private readonly clock: P011GestureRateClock = new P011GestureRateClock();
  private touching: boolean = false;
  private x: number = 64; private y: number = 220; private sequence: number = 0;
  active: boolean = true;
  gravityMappingStatus: string = 'T50_NOT_CALIBRATED';
  private failed: boolean = false;
  private input: P051GravityInput | null = null;
  private modelGeneration: number = 0;
  private failures: P051FailureStoreV2 = new P051FailureStoreV2();
  private previousFailures: P051FailureExportV2 | null = null;
  private lastGoodRunId: string | null = null; private lastGoodCommit: number | null = null; private lastGoodFrame: number | null = null;
  private lastGoodTilt: number | null = null;
  private sensorCount: number | null = null; private sensorX: number | null = null; private sensorY: number | null = null;
  private sensorZ: number | null = null; private sensorTimeMs: number | null = null;
  exportPhysicsFailureV2(previous: boolean = false): P051FailureExportV2 | null {
    if(previous)return this.previousFailures===null?null:JSON.parse(JSON.stringify(this.previousFailures)) as P051FailureExportV2;
    return this.failures.exportFrozen();
  }
  recordDiagnosticSensor(count: number, x: number, y: number, z: number, timeMs: number): void {
    this.sensorCount=count;this.sensorX=x;this.sensorY=y;this.sensorZ=z;this.sensorTimeMs=timeMs;
  }
  recordDiagnosticPublication(runId: string, commit: number, frame: number, tilt: number): void {
    this.lastGoodRunId=runId;this.lastGoodCommit=commit;this.lastGoodFrame=frame;this.lastGoodTilt=tilt;
  }
  constructor() { this.reset(); }
  reset(): void {
    this.stop('reset');
    if(this.failures.hasFailure())this.previousFailures=this.failures.exportFrozen();
    this.failures=new P051FailureStoreV2();this.modelGeneration++;
    const result=this.field.initializeDryParticleDemo({revision:P051_DRY_REVISION,sourceId:51,particleParams:defaultP011ParticleParams()});
    if(!result.ok)throw new Error('DEMO_DRY_INIT_'+result.code);
    this.sequence = 0; this.failed=false;this.input=null;this.field.setTilt(0);this.field.setShakeInput(0);
    this.lastGoodRunId=null;this.lastGoodCommit=null;this.lastGoodFrame=null;this.lastGoodTilt=null;
    this.sensorCount=null;this.sensorX=null;this.sensorY=null;this.sensorZ=null;this.sensorTimeMs=null;
    this.failures.lifecycle(this.field.getP011RunId(),this.modelGeneration,'reset','initialized',this.field.getRenderPhysicsCommitId(),this.diagnosticFrame,this.diagnosticTime);
  }
  setDiagnosticInputTime(timeMs: number): void {this.diagnosticTime=timeMs;}
  start(x: number, y: number): P051StartReceipt {
    const reason = !this.active ? 'INACTIVE' : this.failed ? 'FAILED' :
      !Number.isFinite(x) || !Number.isFinite(y) || x < 0 || x >= 128 || y < 0 || y > 256 ? 'OUT_OF_DOMAIN' :
      this.touching ? 'ALREADY_HELD' : 'ACCEPTED';
    this.observe('model.start',reason,x,y);
    if (reason !== 'ACCEPTED') return {accepted:false,reason:reason,stopGeneration:this.stopped};
    this.x=x;this.y=y;this.touching=true;this.clock.begin();
    return {accepted:true,reason:reason,stopGeneration:this.stopped};
  }
  move(x: number, y: number): void {
    if (!this.touching) return;
    if (!Number.isFinite(x) || !Number.isFinite(y) || x<0 || x>=128 || y<0 || y>256) { this.stop('OUT_OF_DOMAIN');return; }
    this.observe('model.move','',x,y);this.x=x;this.y=y;
  }
  stop(reason: string = 'unspecified'): void {
    try {
      if(this.failed&&reason!=='physics-failed'&&reason!=='reset')this.failures.repeat();
      this.failures.lifecycle(this.field.getP011RunId(),this.modelGeneration,'stop',reason,this.field.getRenderPhysicsCommitId(),this.diagnosticFrame,this.diagnosticTime);
    }catch(e){} // Sidecar failure cannot replace the original physics outcome.
    this.observe('model.stop',reason);
    if(this.touching)this.stopped++;
    this.touching=false;this.clock.stop();this.field.clearGestureSandQueue();
  }
  setActive(active: boolean): void { this.active = active; if (!active) { this.stop('inactive');this.field.setShakeInput(0);this.input=null; } }
  tick(frameTimeNs: number, dt: number): boolean {
    this.diagnosticTime=frameTimeNs/1000000;this.diagnosticFrame++;
    if(this.input?.source==='stale')this.diagnosticStaleCount++;
    this.diagnostics?.record({kind:'tick',runId:this.field.getP011RunId(),commit:this.field.getRenderPhysicsCommitId(),frame:this.diagnosticFrame,time:this.diagnosticTime,source:this.input?.source ?? 'none',staleCount:this.diagnosticStaleCount,value:this.input?.shakeInput ?? 0,reason:this.active?'active':'inactive'});
    if (!this.active) return true;
    if (this.failed) return false;
    if (this.input !== null) {
      if (this.input.targetTiltDeg !== null) this.field.setTilt(this.input.targetTiltDeg);
      this.field.setShakeInput(this.input.shakeInput);
      if (this.input.source === 'stale') this.stop('stale');
    }
    const sequenceBefore=this.sequence;
    const sourceBefore=this.field.getDryDiagnostics();
    if (this.touching) {
      const seconds = this.clock.sample(frameTimeNs,'ns');
      if (seconds > 0) {
        const request: P051DryAdmissionRequest={revision:P051_DRY_REVISION,sourceId:51,sequence:++this.sequence,
          inputTimeMs:frameTimeNs/1000000,x:this.x,y:this.y,seed:51,requestedH:8*seconds,requestedL:0};
        this.diagnostics?.record({kind:'dry.request',runId:this.field.getP011RunId(),commit:this.field.getRenderPhysicsCommitId(),frame:this.diagnosticFrame,time:this.diagnosticTime,x:request.x,y:request.y,sequence:request.sequence,requested:request.requestedH+request.requestedL});
        const receipt=this.field.injectDryGestureSand(request);
        this.diagnostics?.record({kind:'dry.receipt',runId:receipt.runId,commit:receipt.physicsCommitId,frame:this.diagnosticFrame,time:this.diagnosticTime,sequence:receipt.sequence,admitted:receipt.admittedH+receipt.admittedL,reason:receipt.rejectedReason ?? 'ACCEPTED'});
        if(receipt.rejectedReason!==null && receipt.rejectedReason!=='NO_OP')this.stop('admission:'+receipt.rejectedReason);
      }
    }
    const beforeCommit=this.field.getRenderPhysicsCommitId();
    const traceBefore=this.field.getLastTiltPreviewTrace();
    const stepDt=Math.max(0,Math.min(1/30,dt));
    const ok=this.field.step(stepDt).ok;
    if(!ok)try {
      const sourceAfter=this.field.getDryDiagnostics(),trace=this.field.getLastTiltPreviewTrace();
      const current=trace!==null&&trace!==traceBefore&&trace.runId===this.field.getP011RunId()&&trace.commitBefore>=beforeCommit?trace:null;
      const evidence=current?.firstFailure ?? null,candidate=current?.current ?? null;
      const row: P051PhysicsFailureV2={revision:'P051_FIRST_PHYSICS_FAILURE_V2',
        firstFailureId:this.field.getP011RunId()+':'+this.modelGeneration+':'+this.diagnosticFrame,
        runId:this.field.getP011RunId(),modelGeneration:this.modelGeneration,beforeCommit:beforeCommit,
        afterCommit:this.field.getRenderPhysicsCommitId(),rawCode:this.field.getLastError(),stage:evidence?.origin ?? null,
        hotCode:candidate?.particleStatus ?? null,m2Code:evidence?.m2Code ?? null,
        attemptId:current?.attemptId ?? null,substepId:current?.substepId ?? null,
        candidateRole:candidate?.role ?? null,candidateOrdinal:candidate?.callOrdinal ?? null,
        dt:stepDt,dtUnit:'seconds',requestedDt:dt,frameTime:frameTimeNs,frameTimeUnit:'nanoseconds',
        sourceSequenceBefore:sequenceBefore,sourceSequenceAfter:this.sequence,sourceEpochBefore:sourceBefore.sourceEpoch,sourceEpochAfter:sourceAfter.sourceEpoch,
        externalAdmissionCountBefore:sourceBefore.externalAdmissionCount,externalAdmissionCountAfter:sourceAfter.externalAdmissionCount,
        acceptedTiltDeg:current?.thetaBefore ?? null,inputSource:this.input?.source ?? null,inputSampleTimeMs:this.input?.sampleTimeMs ?? null,
        targetTiltDeg:this.input?.targetTiltDeg ?? null,shakeInput:this.input?.shakeInput ?? null,staleCount:this.diagnosticStaleCount,
        sensorCount:this.sensorCount,sensorX:this.sensorX,sensorY:this.sensorY,sensorZ:this.sensorZ,sensorTimeMs:this.sensorTimeMs,
        lastGoodRunId:this.lastGoodRunId,lastGoodCommit:this.lastGoodCommit,lastGoodFrame:this.lastGoodFrame,
        traceStatus:current?.status ?? 'NOT_CAPTURED',missingStatus:'NULL_FIELDS_NOT_CAPTURED',repeatCount:0};
      this.failures.latch(row);
      this.failures.lifecycle(row.runId,this.modelGeneration,'step-failed','actual-field.step',row.afterCommit,this.diagnosticFrame,this.diagnosticTime);
    }catch(e){}
    if (!ok) { this.failed=true;this.stop('physics-failed');this.field.setShakeInput(0); }
    return ok;
  }
  applyDefinedInput(input: P051GravityInput): void {
    if (this.failed || !this.active || !input.valid) return;
    this.input={source:input.source,sampleTimeMs:input.sampleTimeMs,valid:input.valid,
      targetTiltDeg:input.targetTiltDeg,shakeInput:input.shakeInput,shakeMagnitudeMps2:input.shakeMagnitudeMps2};
  }
  // Exact physical API handoff only: T50 must define/authorize the sensor adapter.
  applyDefinedTilt(degrees: number): void { this.field.setTilt(degrees); }
  createPublication(): P011PublishedRenderState { return this.field.createPublishedRenderStateSlot(); }
}
