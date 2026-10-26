import { P051DryAdmissionRequest, P051_DRY_REVISION } from './P051DryParticleAdmission';
import { P051GravityInput } from './P051GravityInputAdapter';
// T51 isolated demo model. Sensor direction mapping is owned by T50.
import { WaterSandField } from './WaterSandField';
import { defaultP011ParticleParams } from './P011ParticleM2Field';
import { P011GestureRateClock } from './P011GestureRateClock';
import { P011PublishedRenderState } from './P011PublishedRenderState';
export class P051EmptySandDemo {
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
    this.stop();
    const result=this.field.initializeDryParticleDemo({revision:P051_DRY_REVISION,sourceId:51,particleParams:defaultP011ParticleParams()});
    if(!result.ok)throw new Error('DEMO_DRY_INIT_'+result.code);
    this.sequence = 0; this.failed=false;this.input=null;this.field.setTilt(0);this.field.setShakeInput(0);
  }
  start(x: number, y: number): void {
    if (!this.active || this.failed) return;
    this.move(x,y); this.touching = true; this.clock.begin();
  }
  move(x: number, y: number): void {
    if (!Number.isFinite(x) || !Number.isFinite(y)) return;
    this.x = x; this.y = y;
  }
  stop(): void { this.touching = false; this.clock.stop(); this.field.clearGestureSandQueue(); }
  setActive(active: boolean): void { this.active = active; if (!active) { this.stop();this.field.setShakeInput(0);this.input=null; } }
  tick(frameTimeNs: number, dt: number): boolean {
    if (!this.active) return true;
    if (this.failed) return false;
    if (this.input !== null) {
      if (this.input.targetTiltDeg !== null) this.field.setTilt(this.input.targetTiltDeg);
      this.field.setShakeInput(this.input.shakeInput);
      if (this.input.source === 'stale') this.stop();
    }
    if (this.touching) {
      const seconds = this.clock.sample(frameTimeNs,'ns');
      if (seconds > 0) {
        const request: P051DryAdmissionRequest={revision:P051_DRY_REVISION,sourceId:51,sequence:++this.sequence,
          inputTimeMs:frameTimeNs/1000000,x:this.x,y:this.y,seed:51,requestedH:8*seconds,requestedL:0};
        const receipt=this.field.injectDryGestureSand(request);
        if(receipt.rejectedReason!==null && receipt.rejectedReason!=='NO_OP')this.stop();
      }
    }
    const ok=this.field.step(Math.max(0,Math.min(1/30,dt))).ok;
    if (!ok) { this.failed=true;this.stop();this.field.setShakeInput(0); }
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
