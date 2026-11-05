import { P011ParticleSnapshot } from '../P011ParticleField';
import { P011ParticleHotPlanBuffer } from '../P011ParticleHot';
export class StopCuts {
 enabled:boolean=false;readonly ids=new Uint32Array(3);readonly rows=new Float64Array(256*48);
 readonly bits=new DataView(new ArrayBuffer(8));count=0;head=0;overflow=0;serial=0;frame=0;
 run:string='';token:string='';actual:string='';epoch=0;commit=0;source=0;held=false;active=false;
 admissionCount=0;frozen=0;lastError=0;derivedStatus=0;previousNs=0;currentNs=0;dt=0;elapsed=0;rollbacks=0;attempt=0;substep=0;tilt=0;motions=0;requests=0;applyCount=0;writes=0;reason=0;stepOk=0;stepped=0;dormant=0;q=0;subDt=0;
 private readonly runs:string[]=new Array<string>(256).fill('');private readonly actuals:string[]=new Array<string>(256).fill('');
 private readonly tokens:string[]=new Array<string>(256).fill('');
 private record(stage:number,id:number,slot:number,y:number,vy:number,present:boolean):void {
  const a=this.head*48,v=this.rows;v.fill(0,a,a+48);v[a]=++this.serial;v[a+1]=this.frame;v[a+2]=stage;v[a+3]=id;v[a+4]=slot;v[a+5]=present?1:0;
  this.bits.setFloat64(0,y,true);v[a+6]=this.bits.getUint32(0,true);v[a+7]=this.bits.getUint32(4,true);
  this.bits.setFloat64(0,vy,true);v[a+8]=this.bits.getUint32(0,true);v[a+9]=this.bits.getUint32(4,true);
  v[a+10]=this.epoch;v[a+11]=this.commit;v[a+12]=this.source;v[a+13]=this.held?1:0;v[a+14]=this.active?1:0;v[a+15]=this.previousNs;v[a+16]=this.currentNs;v[a+17]=this.dt;v[a+18]=this.elapsed;v[a+19]=this.rollbacks;v[a+20]=this.attempt;v[a+21]=this.substep;v[a+22]=this.tilt;v[a+23]=this.motions;v[a+24]=this.requests;v[a+25]=this.applyCount;v[a+26]=this.writes;v[a+27]=this.reason;v[a+28]=this.stepOk;v[a+29]=this.stepped;v[a+30]=this.q;v[a+31]=this.subDt;v[a+32]=this.dormant;v[a+33]=this.admissionCount;v[a+34]=this.frozen;v[a+35]=this.lastError;v[a+36]=this.derivedStatus;
  this.actuals[this.head]='';this.runs[this.head]=this.run;this.tokens[this.head]=this.token;this.head=(this.head+1)%256;if(this.count<256)this.count++;else this.overflow++;
 }
 capture(stage:number,p:P011ParticleSnapshot):void {
  if(!this.enabled)return;
  for(let k=0;k<3;k++){
   let slot=-1;for(let i=0;i<p.nmax;i++)if(p.pState[i]!==0&&p.pId[i]===this.ids[k]&&this.ids[k]!==0){slot=i;break;}
   if(slot<0&&this.ids[k]!==0&&stage===0){this.record(6,this.ids[k],-1,0,0,false);this.ids[k]=0;}
   if(this.ids[k]===0&&stage===0)for(let i=0;i<p.nmax;i++)if(p.pState[i]===1&&p.pId[i]!==this.ids[0]&&p.pId[i]!==this.ids[1]&&p.pId[i]!==this.ids[2]){this.ids[k]=p.pId[i];slot=i;break;}
   if(this.ids[k]!==0)this.record(stage,this.ids[k],slot,slot<0?0:p.pY[slot],slot<0?0:p.pVy[slot],slot>=0);
  }
 }
 predicted(p:P011ParticleHotPlanBuffer):void{if(!this.enabled)return;this.attempt=p.attemptToken;this.substep=p.substepId;this.tilt=p.effectiveTiltRad;this.motions=p.motionCount;this.requests=p.requestCount;for(let k=0;k<3;k++)if(this.ids[k]){let at=-1;for(let i=0;i<p.motionCount;i++)if(p.motionParticleId[i]===this.ids[k]){at=i;break;}this.record(1,this.ids[k],at,at<0?0:p.motionY[at],at<0?0:p.motionVy[at],at>=0);}}
 attach(requested:string,actual:string):void{if(!this.enabled)return;this.token=requested;this.actual=actual;for(let i=0;i<this.count;i++){const at=(this.head-this.count+256+i)%256;if(this.rows[at*48+1]===this.frame){this.tokens[at]=requested;this.actuals[at]=actual;}}}
 finishStep(ok:boolean,stepped:boolean,dormant:boolean,q:number,subDt:number):void{if(!this.enabled)return;this.stepOk=ok?1:0;this.stepped=stepped?1:0;this.dormant=dormant?1:0;this.q=q;this.subDt=subDt;this.record(8,0,-1,0,0,false);}
 exportCold():string {const rows:number[][]=[],runs:string[]=[],tokens:string[]=[],actuals:string[]=[];for(let i=0;i<this.count;i++){const at=(this.head-this.count+256+i)%256;rows.push(Array.from(this.rows.subarray(at*48,at*48+48)));runs.push(this.runs[at]);tokens.push(this.tokens[at]);actuals.push(this.actuals[at]);}return JSON.stringify({revision:'T359_STOP_CUTS_V1',capacity:256,overflow:this.overflow,rows:rows,runs:runs,tokens:tokens,actualTokens:actuals,actualToken:this.actual,stages:['canonicalBefore','predicted','afterApply','prepublication','bank','slot','retired','rollback','stepReceipt'],fields:['serial','frame','stage','id','physicalSlot','present','yLo','yHi','vyLo','vyHi','epoch','commit','sourceEpoch','held','active','previousNs','currentNs','clippedDt','acceptedElapsed','rollbacks','attempt','substep','acceptedTiltRad','motionCount','requestCount','applyCount','writeCount','rollbackReason','stepOk','stepped','Q','substepDt','dormant','admissionCount','frozen','lastError','derivedStatus']});}
}
