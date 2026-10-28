import { P051GravityInput } from '../P051GravityInputAdapter';
export class ClearWaterGravityToken {
 revision:string='CW_GRAVITY_ACCEPT_V1';adapterRev:number=1;ownerId:number=0;runGeneration:number=0;inputGeneration:number=0;configRev:number=0;tick:number=0;
 sampleTimeMs:number=0;sampleSequence:number=0;rawTarget:number=0;boundTarget:number=0;saturated:boolean=false;shakeDiagnostic:number=0;
 h:number=0;attempt:number=0;substepIndex:number=0;aBefore:number=0;trial:number=0;revisionCandidate:number=0;committedPhysics:number=0;failure:number=0;
}
export class ClearWaterGravityPort {
 readonly token:ClearWaterGravityToken=new ClearWaterGravityToken();acceptedAngle:number=0;angleValid:boolean=false;angleRevision:number=0;
 readonly calibrationStatus:string='PENDING_CALIBRATION';private sampleTime:number=-1;private sequence:number=0;private frozen:boolean=false;
 reset():void {this.acceptedAngle=0;this.angleValid=false;this.angleRevision=0;this.sampleTime=-1;this.sequence=0;this.frozen=false;}
 freeze(input:P051GravityInput|null,nowMs:number,offline:boolean=false):boolean {
  this.frozen=false;
  if(input===null||!input.valid||(input.source!=='sensor'&&!(offline&&input.source==='synthetic'))||input.targetTiltDeg===null||!Number.isFinite(input.targetTiltDeg)||Math.abs(input.targetTiltDeg)>89||!Number.isFinite(input.shakeInput)||!Number.isFinite(input.sampleTimeMs)||input.sampleTimeMs<this.sampleTime||!Number.isFinite(nowMs)||nowMs<input.sampleTimeMs||nowMs-input.sampleTimeMs>250)return false;
  if(input.sampleTimeMs>this.sampleTime){this.sequence++;this.sampleTime=input.sampleTimeMs;}
  const t=this.token;t.sampleTimeMs=input.sampleTimeMs;t.sampleSequence=this.sequence;t.rawTarget=input.targetTiltDeg;t.boundTarget=Math.max(-30,Math.min(30,input.targetTiltDeg));t.saturated=t.rawTarget!==t.boundTarget;t.shakeDiagnostic=input.shakeInput;this.frozen=true;return true;
 }
 trial(h:number,index:number):number {
  if(!this.frozen||!Number.isFinite(h)||h<=0||h>1/60)throw new Error('CW_GRAVITY_STALE');
  const t=this.token;t.h=h;t.substepIndex=index;t.attempt++;t.aBefore=this.acceptedAngle;
  const df=(t.boundTarget-this.acceptedAngle)*(1-Math.exp(-h/0.12));t.trial=this.acceptedAngle+Math.max(-90*h,Math.min(90*h,df));t.revisionCandidate=this.angleRevision+1;t.committedPhysics=0;t.failure=0;
  if(!Number.isFinite(t.trial)||Math.abs(t.trial)>30)throw new Error('CW_GRAVITY_TRIAL');return t.trial;
 }
 complete(success:boolean,physics:number,failure:number):void {const t=this.token;if(success){this.acceptedAngle=t.trial;this.angleValid=true;this.angleRevision=t.revisionCandidate;t.committedPhysics=physics;}else t.failure=failure;}
}
