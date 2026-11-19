// T50 mapping / T51 isolated adapter. Demo thresholds are NOT_CALIBRATED.
export interface P051GravityInput {
  source: 'sensor' | 'synthetic' | 'stale'; sampleTimeMs: number; valid: boolean;
  targetTiltDeg: number | null; shakeInput: number; shakeMagnitudeMps2: number;
}
export class P051GravityConfig {
  revision: string = 'T50_DEMO_CANDIDATE_NOT_CALIBRATED_V1';
  alpha: number = 0.6; // existing WaterSandField inputSmoothingAlpha
  shakeOnMps2: number = 1; shakeFullMps2: number = 6;
  staleMs: number = 250; sensorIntervalNs: number = 20000000;
}
export class P051GravityInputAdapter {
  private time: number = -1; private x: number = 0; private y: number = 0; private z: number = 0;
  private last: P051GravityInput | null = null;
  constructor(readonlyConfig: P051GravityConfig) {
    if (!Number.isFinite(readonlyConfig.alpha) || readonlyConfig.alpha<=0 || readonlyConfig.alpha>1 ||
      !Number.isFinite(readonlyConfig.shakeOnMps2) || readonlyConfig.shakeOnMps2<0 ||
      !Number.isFinite(readonlyConfig.shakeFullMps2) || readonlyConfig.shakeFullMps2<=readonlyConfig.shakeOnMps2 ||
      !Number.isFinite(readonlyConfig.staleMs) || readonlyConfig.staleMs<=0) throw new Error('GRAVITY_CONFIG');
    this.config=readonlyConfig;
  }
  private readonly config: P051GravityConfig;
  reset(): void { this.time=-1;this.x=0;this.y=0;this.z=0;this.last=null; }
  consumeAccelerometer(x: number,y: number,z: number,time: number): P051GravityInput {
    if (![x,y,z,time].every((v: number): boolean => Number.isFinite(v)) || time<0 || time<=this.time)
      return {source:'sensor',sampleTimeMs:time,valid:false,targetTiltDeg:null,shakeInput:0,shakeMagnitudeMps2:0};
    const a=this.config.alpha;
    this.x=this.time<0?x:a*x+(1-a)*this.x;
    this.y=this.time<0?y:a*y+(1-a)*this.y;
    this.z=this.time<0?z:a*z+(1-a)*this.z;
    const magnitude=Math.hypot(x-this.x,y-this.y,z-this.z);
    const angle=Math.hypot(this.x,this.y)>0.3?Math.max(-89,Math.min(89,Math.atan2(-this.x,this.y)*180/Math.PI)):null;
    this.time=time;
    const input: P051GravityInput={source:'sensor',sampleTimeMs:time,valid:true,targetTiltDeg:angle,
      shakeInput:Math.max(0,Math.min(1,(magnitude-this.config.shakeOnMps2)/(this.config.shakeFullMps2-this.config.shakeOnMps2))),shakeMagnitudeMps2:magnitude};
    this.last=input; return input;
  }
  latest(time: number): P051GravityInput | null {
    if (this.last===null) return null;
    if (!Number.isFinite(time) || time<this.time || time-this.time>this.config.staleMs)
      return {source:'stale',sampleTimeMs:time,valid:true,targetTiltDeg:null,shakeInput:0,shakeMagnitudeMps2:0};
    return this.last;
  }
}
