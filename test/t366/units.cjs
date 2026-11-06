'use strict';
const assert=require('node:assert/strict'),{loader}=require('../t351/load.cjs');
const base=process.argv[2],live='entry/src/main/ets';
const create=root=>loader(root)('physics/derived/P051DerivedFacadeV2').P051DerivedFacadeV2;
const cfg={isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:512,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'};
const stable=x=>{
 if(ArrayBuffer.isView(x))return Buffer.from(x.buffer,x.byteOffset,x.byteLength).toString('hex');
 if(x===null||typeof x!=='object')return x;
 return Object.fromEntries(Object.entries(x).filter(([k])=>!['runId','run','owner','ownerId','ledgerCommitKey','physicsTimeMs'].includes(k)).map(([k,v])=>[k,stable(v)]));
};
const request=(sequence,x,y,mass)=>({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence,inputTimeMs:sequence*100,x,y,seed:51,requestedH:mass,requestedL:0});
const rows=[];
for(const [name,admissions,tilt] of [
 ['empty',[],0],['air',[[64.5,40,.5]],0],['full-land',[[64.5,.001,.1]],0],
 ['contact-101',[[63.5,.001,4],[64.5,.001,4],[65.5,.001,2.1]],0],
 ['contact-134',[[63.5,.001,4],[64.5,.001,4],[65.5,.001,5.4]],0],
 ['crossing',[[63.99999,3,1.2]],25],['rollback',[[64.5,.001,1]],0],
]){
 const peers=[base,live].map(root=>new (create(root))(cfg));
 peers[1].configureParticleFineReceipt(true);
 let frame=1;
 const op=(fn,expect=true)=>{
  const outcomes=peers.map(fn);
  assert.equal(outcomes[0].ok,expect,name);
  assert.deepEqual(stable(outcomes[0]),stable(outcomes[1]),name);
  assert.deepEqual(...peers.map(f=>stable(f.exportCheckpointCold())));
  assert.deepEqual(...peers.map(f=>stable(f.field.particleBinding.work.events)));
  assert.deepEqual(...peers.map(f=>Array.from(f.countersCold())));
  for(const f of peers)assert.equal(f.copyPublishedSceneStateInto(f.createPublishedSlotCold()),'OK');
 };
 for(const f of peers){f.budget.enabled=true;f.setTilt(tilt);}
 admissions.forEach(([x,y,m],i)=>op(f=>f.inject(request(i+1,x,y,m))));
 if(name==='rollback')for(const f of peers)f.field.particleBinding.sink.failAfterPublish=1;
 const initialLive=peers[1].field.derivedV2.canonical.pool.liveCount;
 for(const f of peers)f.budget.begin(frame,33333333,1/30);
 op(f=>f.step(1/30),name!=='rollback');
 for(const f of peers)f.budget.end();
 const f=peers[1],budget=JSON.parse(f.budget.exportCold()),fine=JSON.parse(f.exportParticleFineReceiptCold());
 const b=budget.rows.at(-1),samples=fine.rows.filter(row=>row[0]===frame);
 for(const row of samples)assert.equal(row.slice(8,14).reduce((sum,x)=>sum+x,0),row[7]);
 if(name==='crossing')assert(samples.some(row=>row[18]>0));
 if(name.startsWith('contact')){
  assert(samples.some(row=>row[34]>0)&&samples.some(row=>row[35]>0));
  assert.equal(b[7],2);assert.equal(b[8],0);assert(samples.some(row=>row[2]===1&&row[23]>0&&row[24]>0));
  assert(samples.some(row=>row[2]===2&&row[23]>0));
 }
 rows.push({name,N:512,initialLive,Q:b[7],A:b[8],R:b[9],samples});
}
const peer=new (create(live))(cfg),w=peer.field.particleBinding.work,p=w.pool,s=w.patchBefore;
const original=s.pY,originalMass=s.pMass;
assert.equal(p.validateHotTransfer(s),0);
const checks=p.hotTransferFullComparisons;
s.pY=new Float64Array(original.buffer);assert.equal(p.validateHotTransfer(s),0);
assert(p.hotTransferFullComparisons>checks);
s.pY=original.slice();assert.equal(p.validateHotTransfer(s),0);assert(p.hotTransferFullComparisons>checks);
s.pY=s.pX;assert.equal(p.validateHotTransfer(s),3);
s.pY=p.pY;assert.equal(p.validateHotTransfer(s),3);
s.pY=original;
const detached=originalMass.slice();structuredClone(detached.buffer,{transfer:[detached.buffer]});
s.pMass=detached;assert.equal(p.validateHotTransfer(s),3);s.pMass=originalMass;
assert.equal(p.validateHotTransfer(s),0);
console.log(JSON.stringify({base:'798d5d4',passed:8,failed:0,device:'NOT_RUN',rows},null,2));
