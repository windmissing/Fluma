import { P051DiagnosticBuffer } from './P051DiagnosticBuffer';
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
  constructor() { this.reset(); }
  reset(): void {
    this.stop('reset');
    const result=this.field.initializeDryParticleDemo({revision:P051_DRY_REVISION,sourceId:51,particleParams:defaultP011ParticleParams()});
    if(!result.ok)throw new Error('DEMO_DRY_INIT_'+result.code);
    this.sequence = 0; this.failed=false;this.input=null;this.field.setTilt(0);this.field.setShakeInput(0);
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
    const ok=this.field.step(Math.max(0,Math.min(1/30,dt))).ok;
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
