const assert=require('node:assert/strict'),{loader}=require('../t351/load.cjs');
const load=loader('entry/src/main/ets');
const F=load('physics/derived/P051DerivedFacadeV2').P051DerivedFacadeV2;
const revision='P011_CPU_DRY_CROSS_VERTICAL_V1';
const cfg={isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:32,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'};
const newField=()=>new F({...cfg,motionRevision:revision});
const request=(i,x,y,mass)=>({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:i,inputTimeMs:100*i,x,y,seed:51,requestedH:mass,requestedL:0});
const next=newField(),old=new F(cfg);
for(const f of [next,old]){
 assert(f.inject(request(1,63.99999,2,1.2)).ok);
 f.setTilt(25);
}
const before=next.exportCheckpointCold();
assert.equal(before.motionRevision,revision);
assert.throws(()=>old.restoreCold(before),/VERSION_CONFIG/);
assert.throws(()=>next.restoreCold(old.exportCheckpointCold()),/VERSION_CONFIG/);
assert.throws(()=>new F({...cfg,motionRevision:'bogus'}),/MOTION_UNSUPPORTED/);
const restored=newField();restored.restoreCold(before);
assert.equal(restored.exportCheckpointCold().motionRevision,revision);
assert.throws(()=>next.field.setNaturalObservationProbe({}),/OBSERVATION_UNSUPPORTED/);
const seen=[];
for(let i=0;i<6;i++){
 for(const f of [next,old]){
  const r=f.step(1/60);assert(r.ok,JSON.stringify(r));
 }
 const e=JSON.parse(next.exportCrossVerticalCold());
 for(let j=0;j<e.integers.length/8;j++){
  const k=e.integers.slice(j*8,j*8+8),v=e.values.slice(j*20,j*20+20);
  if((k[6]&15)!==2)continue;
  assert.equal(v[17],0);assert.equal(v[15],v[16]);
  assert.equal(Math.floor(v[8]),k[3]);
  assert(Object.is(v[10],0));if(Math.min(v[2],v[5])>v[12]){assert.equal((k[6]>>4)&3,1);assert(Object.is(v[5],v[9]));assert(v[9]<v[2]);}else assert.equal(v[9],Math.min(256,v[12]+1e-6));
  seen.push({id:k[1],beforeY:v[2],afterY:v[9],reserved:v[15]});
 }
}
assert(seen.length>0);
const slot=next.createPublishedSlotCold();
assert.equal(next.copyPublishedSceneStateInto(slot),'OK');
assert.equal(slot.state.motionRevision,revision);
assert.equal(next.exportCrossVerticalCold().startsWith('{'),true);
console.log(JSON.stringify({checked:seen.length,example:seen[0]}));

const bytes=a=>Buffer.from(a.buffer,a.byteOffset,a.byteLength).toString('hex');
function fixture(){
 const f=newField();
 assert(f.inject(request(1,63.99999,2,1.2)).ok);
 f.setTilt(25);
 return f;
}
function poolImage(f){
 const d=f.field.derivedV2,p=d.canonical.pool,field=d.domain.field;
 return [p.liveCount,p.freeCount,p.nextId,...[p.pMass,p.pX,p.pY,p.pVx,p.pVy,p.pState,p.pId,p.pSourceId,p.pPhase,p.pSource,
  p.freeSlots,p.colParticleMassH,p.colParticleMassL,d.canonical.acc,d.canonical.count,d.canonical.generations,d.canonical.ids,
  field.bh,field.bl,field.mh,field.ml,field.suspH,field.suspL,field.surface,field.waterTop].map(bytes)];
}
function publication(f){
 const p=f.createPublishedSlotCold();
 assert.equal(f.copyPublishedSceneStateInto(p),'OK');
 const s=p.state;
 return [s.runId,s.physicsCommitId,s.stateEpoch,s.motionRevision,s.effectiveTiltDeg,bytes(s.field.particleState.pY),bytes(s.field.mh)];
}
for(const cut of ['reserve','refund','published-row','events-full','m2-finish','tail-identity','tail-slot-reused','tail-write','prepare']){
 const f=fixture(),w=f.field,b=w.particleBinding,sink=b.sink,d=w.derivedV2;
 const pub=publication(f),companion=f.exportCrossVerticalCold();
 let before=null,reached=false;
 const budgets=()=>[bytes(w.substepMotionBudgetRemaining),bytes(w.substepMotionBudgetUsed),w.totalMotionBudgetRemaining,w.totalMotionBudgetUsed,
  w.bottomCreepBudgetUsed,...sink.budget.coldBuffers().map(a=>bytes(new Uint8Array(a))),b.work.events.count,b.work.events.lineageCount];
 const save=w.saveCrossSubstep;
 w.saveCrossSubstep=function(){save.call(this);before={pool:poolImage(f),budgets:budgets()};};
 if(cut==='reserve'||cut==='refund'){
  const key=cut+'Into',original=sink.budget[key];
  sink.budget[key]=function(...args){
   const code=original.apply(this,args);
   if(code===0&&!w.planningPreviewActive){reached=true;return 13;}return code;
  };
 }
 if(cut==='published-row'||cut==='events-full'){
  const apply=sink.applyInto;
  sink.applyInto=function(...args){
   if(!w.planningPreviewActive){
    reached=true;
    if(cut==='published-row')this.failAfterPublish=1;
    else b.work.events.count=b.work.events.capacity;
   }
   return apply.apply(this,args);
  };
 }
 if(cut==='m2-finish'){
  const apply=b.apply;
  b.apply=function(...args){
   const status=apply.apply(this,args);
   if(status===0&&!args[4]){reached=true;return 13;}return status;
  };
 }
 if(cut==='tail-identity'||cut==='tail-slot-reused'||cut==='tail-write'){
  const resolve=w.crossVertical.resolve;
  w.crossVertical.resolve=function(...args){
   if(cut==='tail-identity'){
    assert(this.staged.count>0);
    this.staged.keys[1]++;
   }
   if(cut==='tail-slot-reused'){
    assert(this.staged.count>0);
    d.canonical.generations[this.staged.keys[0]]++;
   }
   const code=resolve.apply(this,args);reached=true;
   return cut==='tail-write'&&code===0?d.fail(1004,'TEST_AFTER_TAIL'):code;
  };
 }
 if(cut==='prepare'){
  const p=w.derivedPublicationV2,prepare=p.prepareInactive;
  p.prepareInactive=function(...args){
   const code=prepare.apply(this,args);
   if(code===0){reached=true;return d.fail(1004,'TEST_AFTER_PREPARE');}return code;
  };
 }
 const r=f.step(1/60);
 assert.equal(r.ok,false,cut);assert(reached,cut);
 assert.deepEqual(poolImage(f),before.pool,cut);
 assert.deepEqual(budgets(),before.budgets,cut);
 assert.deepEqual(publication(f),pub,cut);
 assert.equal(f.exportCrossVerticalCold(),companion,cut);
 const failure=JSON.parse(f.exportCrossVerticalFailureCold());
 assert(failure.failed&&failure.rolledBack&&failure.attempt&&failure.cut,JSON.stringify(failure));
 if(cut==='reserve'||cut==='refund'||cut==='published-row'||cut==='events-full')assert(failure.originalHotCode>0,cut);
}
const prefix=fixture(),prefixW=prefix.field,prefixP=prefixW.derivedPublicationV2;
const prefixPrepare=prefixP.prepareInactive;
let prepared=0,firstPrefix=null;
prefixP.prepareInactive=function(...args){
 const code=prefixPrepare.apply(this,args);
 if(code===0&&++prepared===2){
  firstPrefix=[publication(prefix),prefix.exportCrossVerticalCold()];
  return prefixW.derivedV2.fail(1004,'TEST_SECOND_PREPARE');
 }
 return code;
};
const prefixResult=prefix.step(1/30);
assert.equal(prefixResult.ok,false);assert.equal(prepared,2);assert(firstPrefix);
assert.deepEqual([publication(prefix),prefix.exportCrossVerticalCold()],firstPrefix);
assert.equal(JSON.parse(prefix.exportCrossVerticalFailureCold()).cut,'TEST_SECOND_PREPARE');
console.log(JSON.stringify({rollbackCuts:9,successfulPrefixRetained:true}));
const near=newField();
assert(near.inject(request(1,63.99999,.004,1.2)).ok);
near.setTilt(25);
assert(near.step(1/60).ok);
const first=JSON.parse(near.exportCrossVerticalCold()),ids=new Set();
for(let i=0;i<first.integers.length;i+=8)if((first.integers[i+6]&15)===2)ids.add(first.integers[i+1]);
assert(ids.size>0);
let deposit=0,landed=0;
for(let i=0;i<12;i++){
 const r=near.step(1/60);assert(r.ok,JSON.stringify(r));
 const e=near.field.particleBinding.work.events;
 for(let j=0;j<e.count;j++)if(e.kind[j]===2&&ids.has(e.particleId[j])){
  assert.equal(e.column[j],63);assert.equal(e.targetColumn[j],-1);
  if(e.granted[j]>0)deposit++;
  if(e.remainder[j]===0){assert(ids.delete(e.particleId[j]));landed++;}
 }
}
assert(deposit>0&&landed>0);
const vy=-44.27410539280644-9.8*Math.cos(Math.PI/6)/60;
const y=31.399036625257057+vy/60;
assert(Math.abs(y-30.65877735511109)<1e-13);
assert(Math.abs(vy-(-44.4155562087579))<1e-13);
const memory=new F({...cfg,nmax:512,motionRevision:revision}).field;
const work=memory.particleBinding.work;
const projection=memory.particleBinding.projection;
const rows=memory.crossVertical;
const bytesOf=(a,b)=>a.byteLength+b.byteLength;
const memoryBytes={
 tails:3*bytesOf(rows.staged.keys,rows.staged.values)+rows.seen.byteLength+bytesOf(rows.expected,rows.expectedState),
 plan:[work.planA,work.planB,work.seenRequests,work.patchRequest,work.patchOriginal,work.sealedPlan,memory.particleBinding.sink.budget.requests].reduce((a,p)=>a+p.blockedYFree.byteLength,0),
 projection:9*8*(13+work.crossBudgetProof.length+work.requestCapacity+work.crossRows.capacity*27+work.nmax*11+work.cols*10),
 tailExtra:rows.stagedSlotGeneration.byteLength+rows.publishedFinal.byteLength+rows.publishedFinalState.byteLength,
 projectionInput:work.crossBudgetProof.byteLength,
 parent:memory.crossBefore.byteLength+memory.particleBinding.sink.outerBackupBytes()
};
assert.equal(memoryBytes.tails,313856);
assert.equal(memoryBytes.plan,81936);
console.log(JSON.stringify({deposit,landed,anchor:{y,vy},memoryBytes}));
const {P011DryCrossVerticalV1}=load('physics/derived/P011DryCrossVerticalV1');
for(const arm of [
 {rowState:1,y0:2,yFree:1,rowY:1,bed:1.5,wantY:1.500001,wantState:2},
 {rowState:2,y0:2,yFree:1,rowY:1.500001,bed:.5,wantY:.500001,wantState:2},
 {rowState:1,y0:1,yFree:2,rowY:2,bed:1,wantY:1.000001,wantState:2},
 {rowState:1,y0:256,yFree:256,rowY:256,bed:256,wantY:256,wantState:2},
 {rowState:1,y0:2,yFree:1,rowY:1,bed:0,wantY:1,wantState:1},
]){
 const t=new P011DryCrossVerticalV1(1);
 t.staged.count=1;
 const k=t.staged.keys,v=t.staged.values;
 k.set([0,1,51,0,2,1,2|(arm.rowState<<4),0]);
 v[0]=.1;v[2]=arm.y0;v[5]=arm.yFree;v[8]=.999999999;v[9]=arm.rowY;v[11]=arm.rowState===2?0:-1;
 const p={nmax:1,cols:2,pId:[1],pSourceId:[51],pPhase:[0],pSource:[0],pState:[arm.rowState],pMass:[v[0]],pX:[v[8]],pY:[arm.rowY],pVx:[0],pVy:[v[11]]};
 const f={bh:[arm.bed,0],bl:[0,0],mh:[0,0],ml:[0,0],surface:[arm.bed,0],capacity:256};
 const owner={canonical:{pool:p},domain:{field:f},begin(){return {};},finish(){return 0;},fail(code){return code;}};
 assert.equal(t.resolve(owner,1e-6,null),0);
 assert.equal(p.pState[0],arm.wantState);assert.equal(p.pY[0],arm.wantY);
 if(arm.wantState===2)assert(Object.is(p.pVy[0],0));
 assert.equal(t.staged.values[13],arm.bed);
}
console.log(JSON.stringify({tailArms:5}));

for(const mode of ['SOURCE_LOCAL_ZERO','TARGET_LOCAL_ZERO','TOTAL_ZERO','FULL','CAPACITY']){
 const f=fixture(),w=f.field,b=w.particleBinding,apply=b.apply;
 b.apply=function(scale,local,used,total,preview){
  if(mode==='SOURCE_LOCAL_ZERO')local[63]=0;
  if(mode==='TARGET_LOCAL_ZERO')local[64]=0;
  if(mode==='TOTAL_ZERO')total=w.totalMotionBudgetRemaining=0;
  if(mode==='FULL'){local[63]=local[64]=8;total=w.totalMotionBudgetRemaining=16;scale=w.totalMotionBudgetScale=1;}
  const mass=b.pool.particleMassHAt;
  if(mode==='CAPACITY')b.pool.particleMassHAt=function(c){return c===64?256:mass.call(this,c);};
  try{return apply.call(this,scale,local,used,total,preview);}
  finally{b.pool.particleMassHAt=mass;}
 };
 const r=f.step(1/60);assert(r.ok,JSON.stringify({mode,r}));
 const e=JSON.parse(f.exportCrossVerticalCold());
 const crosses=e.integers.flatMap((_,i)=>i%8===0&&[1,2].includes(e.integers[i+6]&15)?[i/8]:[]);
 if(mode==='FULL'){
  assert.equal(crosses.length,0);assert(b.work.events.granted.some((q,i)=>i<b.work.events.count&&b.work.events.kind[i]===3&&q>0));continue;
 }
 assert(crosses.length>0,mode);
 for(const i of crosses){
  assert.equal(e.integers[8*i+6]&15,mode==='CAPACITY'?1:2);
  assert.equal(e.values[20*i+15],0);assert.equal(e.values[20*i+16],0);
  assert.equal(e.values[20*i+14]===0,mode==='CAPACITY');
 }
}
const left=newField();assert(left.inject(request(1,64.00001,2,1.2)).ok);
left.setTilt(-25);assert(left.step(1/60).ok);
const leftReceipt=JSON.parse(left.exportCrossVerticalCold());
assert(leftReceipt.integers.some((x,i)=>i%8===3&&x===64&&leftReceipt.integers[i+1]===64));
const lift=new F({...cfg,nmax:64,motionRevision:revision});
assert(lift.inject(request(1,20.5,.004,.3)).ok);assert(lift.inject(request(2,63.99999999,3,1.2)).ok);
for(let i=0;i<20;i++)assert(lift.step(1/60).ok);
lift.setShakeInput(1);
for(let i=0;i<3;i++)assert(lift.step(1/60).ok);
lift.setTilt(1);
let simultaneous=0,lifts=0;
const binding=lift.field.particleBinding,beginLift=binding.beginDryBatch;
binding.beginDryBatch=function(...args){
 if(args[6]!==1)return beginLift.apply(this,args);
 const p=this.work.derivedV2.canonical.pool,before=Array.from(p.pId),states=Array.from(p.pState);
 const result=beginLift.apply(this,args);
 if(result){
  lifts++;
  for(let i=0;i<p.nmax;i++)if(before[i]!==p.pId[i])assert.equal(states[i],0,'dry lift replaced a live slot');
  if(lift.field.crossVertical.staged.count>0)simultaneous++;
 }
 return result;
};
for(let i=0;i<6;i++){const r=lift.step(1/60);assert(r.ok,JSON.stringify({i,r,lifts,simultaneous,failure:JSON.parse(lift.exportCrossVerticalFailureCold())}));}
assert(lifts>0&&simultaneous>0,JSON.stringify({lifts,simultaneous}));
console.log(JSON.stringify({forcedBudgetCapacityArms:5,leftCrossing:true,lifts,simultaneous}));




const high=newField();
assert(high.inject(request(1,64.5,.004,.24)).ok);
for(let i=0;i<15;i++)assert(high.step(1/60).ok);
assert(high.field.sandTopScratch[64]>.15);
assert(high.inject(request(2,63.99999,.15,.6)).ok);
high.setTilt(1);
const highB=high.field.particleBinding,highApply=highB.apply;
highB.apply=function(scale,local,used,total,preview){local[63]=0;return highApply.call(this,scale,local,used,total,preview);};
assert(high.step(1/60).ok);
const highE=JSON.parse(high.exportCrossVerticalCold());
let targetRaised=0;
for(let i=0;i<highE.integers.length/8;i++){
 const k=highE.integers.slice(i*8,i*8+8),v=highE.values.slice(i*20,i*20+20);
 if((k[6]&15)===2&&k[3]===63&&k[4]===65){
  assert(v[7]>v[5]);assert.equal(v[9],v[5]);assert.equal(k[6]>>4&3,1);targetRaised++;
 }
}
assert(targetRaised>0);
const sameAngle=fixture(),sb=sameAngle.field.particleBinding,sc=sb.projection.consistent;
let wrongCandidate=false,wrongBank=false;
sb.projection.consistent=function(theta,work,plans){
 wrongCandidate=!sc.call(this,theta,work,plans.slice());
 const y=work.frozenPool.pY[0];work.frozenPool.pY[0]=y+1;
 try{wrongBank=!sc.call(this,theta,work,plans);}finally{work.frozenPool.pY[0]=y;}
 return sc.call(this,theta,work,plans);
};
assert(sameAngle.step(1/60).ok);assert(wrongCandidate&&wrongBank);
const {compareParticlePlansHot,P011ParticleHotPlanBuffer}=load('physics/P011ParticleHot');
const aliasA=new P011ParticleHotPlanBuffer(1,1,1,true),aliasB=new P011ParticleHotPlanBuffer(1,1,1,true);
aliasB.blockedYFree=aliasA.blockedYFree;
assert.equal(aliasB.storageValidHot(),false);
structuredClone(aliasA.blockedYFree.buffer,{transfer:[aliasA.blockedYFree.buffer]});
assert.equal(aliasA.storageValidHot(),false);
const signedZero=newField();
assert(signedZero.inject(request(1,30.5,2,.1)).ok);
const sB=signedZero.field.particleBinding,sApply=sB.apply;
let signChecked=false;
sB.apply=function(...args){
 const w=this.work,a=w.planA,b=w.planB;
 assert.equal(compareParticlePlansHot(a,b),0);
 const first=0;
 if(a.requestCount>0&&a.kind[first]!==3){
  b.blockedYFree[first]=-0;
  assert.notEqual(compareParticlePlansHot(a,b),0);
  b.blockedYFree[first]=0;signChecked=true;
 }
 return sApply.apply(this,args);
};
// Collect has only same-column motion at zero tilt; +0 gating is also
// exercised on a T3 request after the package reaches its own source bed.
for(let i=0;i<48&&!signChecked;i++)assert(signedZero.step(1/60).ok);
assert(signChecked);
console.log(JSON.stringify({targetRaised,sameAngleWrongPlanRejected:wrongCandidate,wrongBeforeBankRejected:wrongBank,signedZeroRejected:signChecked,aliasAndDetachRejected:true}));
