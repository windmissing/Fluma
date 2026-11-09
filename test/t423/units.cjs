'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {loader}=require('../t351/load.cjs');
const load=loader(path.join(process.argv[2],'entry/src/main/ets'));
const F=load('physics/derived/P051DerivedFacadeV2').P051DerivedFacadeV2;
const opts={isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',sourceId:51,backend:'CPU_ORIGINAL_F64',
 writerRegistry:'T216_DRY_WRITERS_V2',motionRevision:'P011_CPU_DRY_CROSS_VERTICAL_V1'};
const bytes=a=>Buffer.from(a.buffer,a.byteOffset,a.byteLength).toString('hex');
const request=(sequence,x,y,requestedH)=>({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence,inputTimeMs:sequence*100,
 x,y,seed:51,requestedH,requestedL:0});
function snapshot(f){
 const field=f.field, d=field.derivedV2,p=d.canonical.pool;
 const b=field.particleBinding.work;
 const cross=JSON.parse(f.exportCrossVerticalCold());
 // Ticket capabilities are process-global. Compare equality/order structure within each independent run.
 const tickets=new Map();for(let i=14;i<cross.values.length;i+=20){const t=cross.values[i];
  if(t!==0){if(!tickets.has(t))tickets.set(t,tickets.size+1);cross.values[i]=tickets.get(t);}}
 const pub=f.createPublishedSlotCold();assert.equal(f.copyPublishedSceneStateInto(pub),'OK');
 return [field.getRenderPhysicsCommitId(),pub.state.physicsCommitId,p.liveCount,p.freeCount,p.nextId,
  ...[p.pMass,p.pX,p.pY,p.pVx,p.pVy,p.pState,p.pId,p.pPhase,p.pSource,p.pSourceId,p.freeSlots,p.colParticleMassH,
   p.colParticleMassL,d.canonical.acc,d.canonical.count,d.canonical.generations,d.canonical.ids,d.canonical.dirty,
   d.domain.field.bh,d.domain.field.bl,d.domain.field.mh,d.domain.field.ml,d.domain.field.waterTop,
   field.substepMotionBudgetRemaining,field.substepMotionBudgetUsed].map(bytes),
  b.events.count,b.events.lineageCount,cross.integers,cross.values];
}
function make(n){return [new F({...opts,nmax:n,cpuTxRoute:'LEGACY_FULL'}),new F({...opts,nmax:n,cpuTxRoute:'LOCAL_UNDO_V1'})];}
function step(fs,dt){const values=fs.map(f=>f.step(dt));assert.equal(values[0].ok,values[1].ok);assert.equal(values[0].originalError,values[1].originalError);
 assert.equal(values[0].physicalCommit,values[1].physicalCommit);assert.deepEqual(snapshot(fs[0]),snapshot(fs[1]));return values[0];}
function inject(fs,r){const v=fs.map(f=>f.inject(r));assert.notEqual(v[0].original.runId,v[1].original.runId);assert.deepEqual({...v[0].original,runId:null},{...v[1].original,runId:null});assert.deepEqual(snapshot(fs[0]),snapshot(fs[1]));}
const cases=[];
for(const n of [192,512]){
 const matrix=[['q0',0,64,1/240,3],['empty',0,64,1/60,5],['held_then_up',0,64.5,1/60,10],['blocked_sparse',1.2,63.99999,1/60,8],['contact_dense',n*.12,63.99999,1/30,8]];
 if(n===512)matrix.push(['contact_l101',10.1,63.99999,1/30,3],['contact_l157',15.7,63.99999,1/30,3],['contact_l237',23.7,63.99999,1/30,3]);
 for(const [label,mass,x,dt,frames] of matrix){
  const owners=make(n);let seq=0;
  for(let remaining=mass;remaining>1e-9;){const take=Math.min(remaining,16);inject(owners,request(++seq,x,2,take));remaining-=take;}
  for(const o of owners){o.setTilt(label==='held_then_up'?0:25);o.budget.enabled=true;o.field.configureParticleFineReceipt(true);}
  let r=0;const times=[[],[]],copyRows=[];
  for(let i=0;i<frames;i++){
   const copiesBefore=owners.map(f=>JSON.parse(f.exportCpuTxUndoCold()).hotPoolBytes);
   for(const f of owners)f.budget.begin(i+1,Math.round(dt*1e9),dt);
   if(label==='held_then_up'&&i<5)inject(owners,request(++seq,x,.004,.1));
   const values=owners.map((f,j)=>{const start=process.hrtime.bigint();const v=f.step(dt);times[j].push(Number(process.hrtime.bigint()-start)/1e6);return v;});
   assert.equal(values[0].ok,values[1].ok);assert.equal(values[0].originalError,values[1].originalError);
   assert.deepEqual(snapshot(owners[0]),snapshot(owners[1]));
   copyRows.push(owners.map((f,j)=>{
    f.budget.end();const row=JSON.parse(f.budget.exportCold()).rows.at(-1);
    assert.equal(row.slice(24,44).reduce((a,b)=>a+b,0),row[19]);
    return {Q:row[7],A:row[8],R:row[9],L:row[6],wallMs:row[19],hotBytes:JSON.parse(f.exportCpuTxUndoCold()).hotPoolBytes-copiesBefore[j],exclusive:row.slice(24,44)};
   }));r+=copyRows.at(-1)[0].R;
  }
  const [legacy,local]=owners.map(f=>JSON.parse(f.exportCpuTxUndoCold()));
  assert.equal(legacy.hotPoolBytes-local.hotPoolBytes,local.removedBytes-local.materializedBytes);
  if(label==='blocked_sparse')assert(local.rows>0&&local.removedBytes>local.materializedBytes);
  const stats=xs=>{const a=xs.slice().sort((a,b)=>a-b);return {p50:a[Math.floor((a.length-1)*.5)],p95:a[Math.ceil(a.length*.95)-1],max:a.at(-1)};};
  const roles=owners.map(f=>JSON.parse(f.field.exportParticleFineReceiptCold()));
  cases.push({label,n,mass,dt,frames,observedR:r,legacy,local,copyRows,roles,legacyMs:stats(times[0]),localMs:stats(times[1])});
 }
}
// Force a real derived failure immediately after a local publish and compare the frozen raw bytes.
const owners=make(192);inject(owners,request(1,63.99999,2,1.2));
for(const f of owners){
 f.setTilt(25);const w=f.field.particleBinding.work,d=f.field.derivedV2;
 const flush=d.flush.bind(d);d.flush=function(){
  if(w.patchState===2&&w.patchRequest.kind[0]===3&&w.patchRequest.crossVertical&&w.patchScratch.pState[w.crossEditedSlot]>0&&
     Math.floor(w.patchScratch.pX[w.crossEditedSlot])===w.patchRequest.column[0]&&!f.field.planningPreviewActive)
   return d.fail(4,'T423_INJECTED_AFTER_PUBLISH');
  return flush();
 };
}
let fail=null;
for(let i=0;i<8;i++){fail=step(owners,1/60);if(!fail.ok)break;}
assert(fail&&!fail.ok,'injected cut must execute');
const raw=owners.map(f=>f.exportFailureCold());assert(raw.every(Boolean));
function comparableRaw(record){
 const header=Array.from(record.headers),idx=16+record.scalarNames.indexOf('patchToken');
 assert(idx>=16&&Number.isSafeInteger(header[idx])&&header[idx]>0);header[idx]=1;
 let budgetLane=0;
 const buffers=record.buffers.map((raw,i)=>{
  const name=record.names[i],isTicketLane=name==='budget'&&budgetLane++===4;
  if(name!=='crossValues'&&!isTicketLane)return bytes(raw);
  const copy=raw.slice(),view=new DataView(copy.buffer,copy.byteOffset,copy.byteLength),ids=new Map();
  const first=isTicketLane?0:14,stride=isTicketLane?1:20;
  for(let j=first;j<view.byteLength/8;j+=stride){const id=view.getFloat64(j*8,true);if(id===0)continue;
   assert(Number.isSafeInteger(id)&&id>0);if(!ids.has(id))ids.set(id,ids.size+1);view.setFloat64(j*8,ids.get(id),true);}
  return bytes(copy);
 });
 return {header,buffers};
}
assert.deepEqual(raw[0].names,raw[1].names);assert.deepEqual(comparableRaw(raw[0]),comparableRaw(raw[1]));
assert.equal(raw[0].fault.cut,raw[1].fault.cut);
const prefixOwners=make(192);inject(prefixOwners,request(1,63.99999,2,1.2));
for(const f of prefixOwners){
 f.setTilt(25);const w=f.field,p=w.derivedPublicationV2;
 const prepare=p.prepareInactive.bind(p);let attempts=0;
 p.prepareInactive=function(...args){const rc=prepare(...args);if(++attempts===2&&rc===0){
   const slot=f.createPublishedSlotCold();assert.equal(f.copyPublishedSceneStateInto(slot),'OK');f.testPrefix=slot.state.physicsCommitId;
   return w.derivedV2.fail(1004,'T423_SECOND_PREPARE');}return rc;};
}
const prefixFailure=step(prefixOwners,1/30);assert.equal(prefixFailure.ok,false);
for(const f of prefixOwners){const slot=f.createPublishedSlotCold();assert.equal(f.copyPublishedSceneStateInto(slot),'OK');
 assert(f.testPrefix>0);assert.equal(slot.state.physicsCommitId,f.testPrefix);}
const quiet=make(192);inject(quiet,request(1,63.99999,2,1.2));
quiet[0].budget.enabled=false;quiet[1].budget.enabled=false;
for(const f of quiet)f.setTilt(25);
for(let i=0;i<3;i++)assert(step(quiet,1/60).ok);
const result={revision:'T423_CPU_TX_OFFLINE_V1',device:'NOT_RUN',cases,failedCut:fail.originalError,rawBuffers:raw[0].buffers.length,secondStepPrefix:prefixOwners[0].testPrefix,diagnosticsOffParity:true};
console.log(JSON.stringify(result));
