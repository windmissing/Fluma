'use strict';
const assert=require('node:assert/strict'),{loader}=require('../t351/load.cjs');
const load=loader(process.argv[2]),Demo=load('physics/derived/P051DerivedDemoV2').P051DerivedDemoV2;
const on=new Demo(16),off=new Demo(16);
const cuts=on.budget.cuts;cuts.enabled=true;
const stable=x=>{
 if(ArrayBuffer.isView(x))return Buffer.from(x.buffer,x.byteOffset,x.byteLength).toString('hex');
 if(x===null||typeof x!=='object')return x;
 return Object.fromEntries(Object.entries(x).filter(([k])=>!['runId','run','owner','ownerId','ledgerCommitKey'].includes(k)).map(([k,v])=>[k,stable(v)]));
};
const parity=()=>assert.deepEqual(stable(on.owner.exportCheckpointCold()),stable(off.owner.exportCheckpointCold()));
const run=fn=>{assert.deepEqual(stable(fn(on)),stable(fn(off)));parity();};
for(let i=1;i<=3;i++){
 const req={revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:i,inputTimeMs:i*100,x:58+i,y:2,seed:51,requestedH:.2,requestedL:0};
 const first=on.owner.field.t367NextId();
 run(d=>d.owner.inject(req));
 assert.equal(cuts.admissionCount,i);assert(cuts.ids[0]>=first&&cuts.ids[1]>=first);
 assert.equal(cuts.ids[2],0);
 const tx=cuts.transaction;
 run(d=>d.owner.inject(req));
 assert.equal(cuts.transaction,tx);
}
run(d=>d.start(64,2));
run(d=>d.stop('UP',321));
let r=JSON.parse(cuts.exportCold());const stop=r.stopAnchor;
assert.equal(r.stopReason,'UP');assert.equal(r.stopTimeMs,321);
assert.equal(stop[2],9);assert(stop[47]>0);
for(let i=1;i<=24;i++){
 for(const d of [on,off]){d.budget.cuts.frame=i;d.budget.enabled=true;d.budget.begin(i,33333333,1/30);}
 run(d=>d.tick(i*33333333,1/30));
 for(const d of [on,off])d.budget.end();
}
r=JSON.parse(cuts.exportCold());
assert(r.overflow>0);
assert.deepEqual(r.stopAnchor,stop);
assert.equal(r.firstAnchor[2],11);assert.equal(r.firstAnchor[11],r.firstCommit);
assert(r.lastAnchor[18]>r.firstAnchor[18]);
const rows=r.rows, terminal=r.terminalRequest;
assert(rows.some(row=>row[43]===1));
assert(rows.some(row=>row[43]===2));
assert(terminal[48]===2&&terminal[50]>0&&terminal[54]===1);
const slot=on.createPublicationCold();assert.equal(on.copyPublicationInto(slot),'OK');
assert(JSON.parse(cuts.exportCold()).rows.some(row=>row[2]===5));
run(d=>d.reset());assert.equal(cuts.transaction,0);assert.equal(cuts.stopReason,'');assert.equal(cuts.count,0);
for(const [reason,stopCase] of [
 ['CANCEL',d=>d.move(-1,2)],
 ['LIFECYCLE',d=>d.setActive(false)],
 ['STALE_SENSOR',d=>d.applyDefinedInput({source:'stale',sampleTimeMs:10,valid:true,targetTiltDeg:null,shakeInput:0,shakeMagnitudeMps2:0})],
 ['ADMISSION_REJECT',d=>{
  const request={revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:10,inputTimeMs:1,x:64,y:2,seed:51,requestedH:.1,requestedL:0};
  assert(d.owner.inject(request).ok);
  d.tick(10000000,0);d.tick(30000000,1/60);
 }],
]){
 const d=new Demo(16);d.budget.cuts.enabled=true;assert(d.start(64,2));stopCase(d);
 assert.equal(d.budget.cuts.stopReason,reason);
}
console.log(JSON.stringify({passed:9,failed:0,device:'NOT_RUN',cohort:r.ids,transaction:r.admissionTransactionId,stopCommit:stop[11],firstCommit:r.firstCommit,lastCommit:r.lastAnchor[11],overflow:r.overflow,terminalCommit:terminal[11]}));
