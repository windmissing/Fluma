const assert=require('node:assert/strict');
const {loader}=require('../t351/load.cjs');
const load=loader('entry/src/main/ets');
const F=load('physics/derived/P051DerivedFacadeV2').P051DerivedFacadeV2;
const {P011Z08Archive,z08Presentation}=load('physics/derived/P011Z08Archive');
const config={isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:32,sourceId:51,
 backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2',motionRevision:'P011_CPU_DRY_CROSS_VERTICAL_V1'};
const req={revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:1,inputTimeMs:100,
 x:63.5,y:.05,seed:51,requestedH:.3,requestedL:0};
const render=(publication,ns,frameId)=>{
 const frameToken=[publication.run,publication.epoch,publication.commit,frameId].join(':');
 const source={frameToken,runId:publication.run,ledgerCommitKey:publication.ledgerKey,
  stateEpoch:publication.epoch,physicsCommitId:publication.commit,surfaceEpoch:7,contextEpoch:8,
  geometryEpoch:9,continuityEpoch:10,generation:11,configRevision:12};
 const frame={...source,fieldEpoch:publication.epoch,frameTimeNs:ns,frameId,requestedGeneration:11,appearanceConfigRevision:12};
 const receipt={presentationState:'CURRENT',currentFrameSucceeded:true,requestedSwapSucceeded:true,
  requestedFrameToken:frameToken,renderedFrameToken:frameToken,renderedSource:source,
  targetSurfaceEpoch:7,targetContextEpoch:8,requestedGeneration:11,renderedGeneration:11,
  actualPresentationGeneration:11,requestedConfigRevision:12,renderedConfigRevision:12,displayPresentVerified:false};
 return [frame,receipt];
};
const runFrame=(field,archive,i,dt=1/60,publicationOverride=null)=>{
 const slot=field.createPublishedSlotCold(),ns=i*16666667;
 archive.beginFrame();field.bindZ08ArchiveCold(archive);
 const result=field.step(dt);
 field.bindZ08ArchiveCold(null);
 assert.equal(field.copyPublishedRenderStateInto(slot),'OK');
 const pub=slot.readZ08Identity();
 const [frame,receipt]=pub?render(pub,ns,i):[null,null];
 archive.finishFrame(result.ok?(publicationOverride??pub):null,result.ok?frame:null,result.ok?receipt:null,ns,i,
  result.ok?'':field.exportCrossVerticalFailureCold());
 return {result,pub,frame,receipt,archive:archive.exportCold()?JSON.parse(archive.exportCold()):null};
};
const field=new F(config),reference=new F(config);
for(const f of [field,reference]){
 assert(f.inject(req).ok);f.setTilt(0);
}
const archive=new P011Z08Archive([1,2,3],false);
const first=runFrame(field,archive,1,1/30);
assert(first.result.ok);
assert.equal(first.archive.presentation,'PRESENT_CURRENT_SWAP');
assert.equal(first.archive.systemPresentVerified,false);
assert.equal(first.archive.candidates.length,2);
assert.equal(first.archive.candidates[0].presentation,'NOT_SELECTED_FOR_DRAW');
assert.equal(first.archive.candidates[1].presentation,'PRESENT_CURRENT_SWAP');
assert.equal(first.archive.selectedPublication.commit,first.archive.candidates[1].publication.commit);
assert(reference.step(1/30).ok);
const scrub=cp=>{
 cp.runId='fixture';cp.authority.externalSourceState.admissions.lastRecord.runId='fixture';
 return cp;
};
assert.deepEqual(scrub(field.exportCheckpointCold()),scrub(reference.exportCheckpointCold()));
const {pub,frame,receipt}=first;
const invalid=[
 {...receipt,renderedSource:{...receipt.renderedSource,physicsCommitId:pub.commit+1}},
 {...receipt,renderedSource:{...receipt.renderedSource,stateEpoch:pub.epoch+1}},
 {...receipt,renderedSource:{...receipt.renderedSource,ledgerCommitKey:'wrong'}},
 {...receipt,renderedSource:{...receipt.renderedSource,contextEpoch:9}},
 {...receipt,requestedSwapSucceeded:false},
 {...receipt,renderedFrameToken:'other'},
 {...receipt,presentationState:'STALE'},
];
for(const bad of invalid)assert.notEqual(z08Presentation(pub,frame,bad,16666667),'PRESENT_CURRENT_SWAP');
assert.notEqual(z08Presentation(pub,frame,receipt,16666668),'PRESENT_CURRENT_SWAP');
assert.throws(()=>new P011Z08Archive([1,1],false),/Z08_IDS/);
const wrongSeal=runFrame(field,new P011Z08Archive([1],false),2,1/60,{...pub,seal:pub.seal+10});
assert.equal(wrongSeal.archive.presentation,'PUBLICATION_NOT_FROM_CALLBACK');
assert(wrongSeal.archive.candidates.every(c=>c.presentation==='NOT_SELECTED_FOR_DRAW'));

let partial=null;
const retired=new Set();
const binding=field.field.particleBinding,apply=binding.apply;
binding.apply=function(scale,local,used,total,preview){
 if(!partial)local[63]=Math.min(local[63],.05);
 return apply.call(this,scale,local,used,total,preview);
};
for(let i=3;i<=45;i++){
 const current=runFrame(field,new P011Z08Archive([1,2,3],false),i);
 assert(current.result.ok,JSON.stringify(current.result));
 for(const candidate of current.archive.candidates){
  for(const evidence of candidate.lands){
   const e=evidence.event;
   assert(evidence.verified,JSON.stringify(evidence));
   assert.equal(e.kind,2);assert.equal(e.column,63);assert.equal(e.targetColumn,-1);
   assert.equal(e.requested,e.granted+e.remainder);
   assert(Math.abs(e.reserved-e.refunded-e.granted)<1e-14);
   assert(Math.abs(e.bedAfter-e.bedBefore-e.granted)<1e-14);
   assert(Math.abs(e.totalBefore-e.totalAfter-e.granted)<1e-14);
   assert(Math.abs(e.localBefore-e.localAfter-e.granted)<1e-14);
   if(e.granted>0&&e.remainder>0){
    assert.equal(evidence.finalState,2);assert.equal(evidence.finalF64Hex.length,5);partial=e;
   }
   if(evidence.retired){
    assert.equal(e.remainder,0);assert(e.granted>0&&!retired.has(e.id));
    assert.equal(evidence.finalState,0);retired.add(e.id);
   }
  }
 }
}
assert(partial&&retired.size>0,JSON.stringify({partial,retired:[...retired]}));

const failed=new F(config);
assert(failed.inject({...req,y:.004}).ok);failed.setTilt(0);
const target=failed.field.derivedPublicationV2,original=target.prepareInactive;
let steps=0;
const failureArchive=new P011Z08Archive([1,2,3],false);
target.prepareInactive=function(...args){
 const code=original.apply(this,args);
 if(code===0&&++steps===2){
  assert(failureArchive.count>0,'failed attempt must contain staged LAND');
  return failed.field.derivedV2.fail(1004,'T392_SECOND_PREPARE');
 }
 return code;
};
const rollback=runFrame(failed,failureArchive,1,1/30);
assert.equal(rollback.result.ok,false);
assert.equal(rollback.archive.captureStatus,'PHYSICS_FAILURE');
assert.equal(rollback.archive.candidates.length,1);
assert.equal(rollback.archive.candidates[0].publication.commit,rollback.pub.commit);
assert.equal(rollback.archive.candidates[0].presentation,'NOT_SELECTED_FOR_DRAW');
assert.equal(rollback.archive.failure.cut,'T392_SECOND_PREPARE');
assert(rollback.archive.failure.rolledBack);
assert.equal(rollback.archive.failure.activeCommit,rollback.pub.commit);
assert.equal(rollback.archive.renderReceipt,null);
assert.equal(rollback.archive.candidates[0].lands.length,0);
console.log(JSON.stringify({pairRejections:9,twoSubsteps:true,partial:partial.granted,
 retired:[...retired],rollbackPrefix:true,authorityUnchanged:true}));

if(process.argv[2]){
 const app=loader(process.argv[2]);
 const Demo=app('physics/derived/P051DerivedDemoV2').P051DerivedDemoV2;
 const model=new Demo(32);
 assert.equal(model.startZ08NearBed(),'Z08_ARMED');
 for(let i=1;i<=60&&!model.exportZ08Cold();i++){
  const ns=i*16666667,slot=model.createPublicationCold();
  model.beginZ08Frame();
  assert(model.tick(ns,1/60),model.status);
  assert.equal(model.copyPublicationInto(slot),'OK');
  const p=slot.readZ08Identity();
  const [frame,receipt]=render(p,ns,i);
  model.finishZ08Frame(slot,frame,receipt,ns,i,false);
 }
 const evidence=JSON.parse(model.exportZ08Cold());
 assert.equal(evidence.presentation,'PRESENT_CURRENT_SWAP');
 assert(evidence.candidates.some(c=>c.lands.some(l=>l.event.granted>0&&l.retired)));
 console.log(JSON.stringify({generatedModel:true,landCallback:evidence.callbacks}));
}
