import worker, { MessageEvents } from '@ohos.worker';
import fs from '@ohos.file.fs';
import util from '@ohos.util';
import { WaterSandField } from '../physics/WaterSandField';
import { defaultP011ParticleParams } from '../physics/P011ParticleM2Field';
import { P011ParticleSnapshot } from '../physics/P011ParticleField';
import { T159NativeBinding,T159Config } from '../physics/T159Native';
import { t159Arm,t159Disarm } from '../physics/T159Selector';
import { t159ResetMetadata } from '../physics/T159Metadata';
export interface T159Command { type: string; route: string; steps: number; queuedAt: number; output: string; config: T159Config; }
const port=worker.workerPort;
let active=false;
function hex(buffer: ArrayBuffer,offset: number,length: number): string {
  const bytes=new Uint8Array(buffer,offset,length); let out='';
  for (let i=0;i<bytes.length;i++) out+=bytes[i].toString(16).padStart(2,'0'); return out;
}
function rawPool(p: P011ParticleSnapshot | null | undefined): string[] {
  if (p === null || p === undefined) return [];
  return [hex(p.pMass.buffer as ArrayBuffer,p.pMass.byteOffset,p.pMass.byteLength),hex(p.pX.buffer as ArrayBuffer,p.pX.byteOffset,p.pX.byteLength),
    hex(p.pY.buffer as ArrayBuffer,p.pY.byteOffset,p.pY.byteLength),hex(p.pVx.buffer as ArrayBuffer,p.pVx.byteOffset,p.pVx.byteLength),hex(p.pVy.buffer as ArrayBuffer,p.pVy.byteOffset,p.pVy.byteLength),
    hex(p.pPhase.buffer as ArrayBuffer,p.pPhase.byteOffset,p.pPhase.byteLength),hex(p.pSource.buffer as ArrayBuffer,p.pSource.byteOffset,p.pSource.byteLength),
    hex(p.pState.buffer as ArrayBuffer,p.pState.byteOffset,p.pState.byteLength),hex(p.pId.buffer as ArrayBuffer,p.pId.byteOffset,p.pId.byteLength),
    hex(p.pSourceId.buffer as ArrayBuffer,p.pSourceId.byteOffset,p.pSourceId.byteLength),hex(p.freeSlots.buffer as ArrayBuffer,p.freeSlots.byteOffset,p.freeSlots.byteLength),
    hex(p.colParticleMassH.buffer as ArrayBuffer,p.colParticleMassH.byteOffset,p.colParticleMassH.byteLength),hex(p.colParticleMassL.buffer as ArrayBuffer,p.colParticleMassL.byteOffset,p.colParticleMassL.byteLength)];
}
function store(path: string,value: string): void {
  const file=fs.openSync(path,fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
  try { const bytes=new util.TextEncoder().encodeInto(value); let offset=0;
    while(offset<bytes.byteLength) { const n=fs.writeSync(file.fd,bytes.buffer.slice(offset)); if(n<=0)throw new Error('SHORT_WRITE'); offset+=n; }
    fs.fsyncSync(file.fd);
  } finally { fs.closeSync(file); }
}
port.onmessage=(event: MessageEvents): void => {
  const c=event.data as T159Command, receivedAt=Date.now();
  if(active) { port.postMessage({status:'FAULT',error:'BUSY'}); return; }
  if(c.type!=='run'||(c.config.nmax!==192&&c.config.nmax!==512)||!Number.isInteger(c.steps)||c.steps<1||c.steps>32||
    !Number.isFinite(c.queuedAt)||c.output.length===0) { port.postMessage({status:'FAULT',error:'COMMAND_INVALID'}); return; }
  active=true; t159ResetMetadata(); let native: T159NativeBinding | null=null;
  const rows: string[]=[]; let fault=''; let result=''; const serviceStart=Date.now();
  try {
    if(c.route==='GPU') {
      native=new T159NativeBinding(c.config); rows.push(JSON.stringify({phase:'READY',result:native.ready}));
      if(native.ready.status!=='READY') throw new Error('READY_FAILED:'+native.ready.errorCode);
    }
    t159Arm(c.route,native);
    // The original field/M2/Hot closure performs all mutations, budgets, audit and rollback.
    const field=new WaterSandField(128,512),params=defaultP011ParticleParams();params.nmax=c.config.nmax;
    if(!field.setParticleRoute(params).ok) throw new Error('FIELD_ROUTE_FAILED');
    field.setNaturalObservationRouteId('dual-field-particle-v1');
    if(!field.particleRouteActive())throw new Error('FIELD_NOT_ACTIVE');
    const before=field.snapshot();
    rows.push(JSON.stringify({phase:'BEFORE',snapshot:before,poolBytes:rawPool(before.particleState),metric:field.getP011Metric()}));
    for(let input=0;input<2;input++) {
      const h=new Float64Array(128),l=new Float64Array(128);h[64]=input===0?params.nmax*params.massPackage:0.25;
      const receipt=field.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:17,sequence:input+1,inputTime:10+input,x:64.5,y:100,seed:13});
      rows.push(JSON.stringify({phase:'SEED',input:input,receipt:receipt}));
      if(receipt===null||Math.abs(receipt.admittedH-h[64])>1e-10)throw new Error('SEED_ADMISSION_FAILED');
    }
    for(let step=0;step<c.steps;step++) {
      const outcome=field.step(1/60);
      if(!outcome.ok)throw new Error('STEP_FAILED:'+outcome.errorCode);
      rows.push(JSON.stringify({phase:'STEP',step:step,outcome:outcome,metric:field.getP011Metric()}));
    }
    const after=field.snapshot();
    rows.push(JSON.stringify({phase:'AFTER',snapshot:after,poolBytes:rawPool(after.particleState),metric:field.getP011Metric(),events:field.getParticleEventTail(16384)}));
    result='TRANSACTION_RETURNED'; // Tester determines correctness; never product PASS.
  } catch(error) { fault=(error as Error).message; result='FAULT'; }
  finally {
    t159Disarm();
    if(native!==null) {
      rows.push(JSON.stringify({phase:'REQUESTS',requests:native.requestRows}));
      const cleanup=native.reset(); rows.push(JSON.stringify({phase:'CLEANUP',result:cleanup}));
      if(cleanup.status!=='RESET') { fault+='|CLEANUP_UNCONFIRMED'; result='FAULT'; }
    }
    const completedAt=Date.now();
    rows.push(JSON.stringify({revision:'T159_TRANSACTION_V2',route:c.route,config:c.config,status:result,error:fault,
      queuedAt:c.queuedAt,receivedAt:receivedAt,completedAt:completedAt,queueMs:receivedAt-c.queuedAt,
      serviceAndCleanupMs:completedAt-serviceStart,attemptWallMs:completedAt-c.queuedAt,gpuRuntime:'NOT_MEASURED'}));
    try {store(c.output,rows.join('\n')+'\n');port.postMessage({status:result,error:fault,output:c.output});}
    catch(error) {port.postMessage({status:'FAULT',error:'ARCHIVE_FAILED:'+(error as Error).message});}
    active=false;
  }
};
