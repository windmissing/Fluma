const assert=require('node:assert/strict');
const {loader}=require('../t351/load.cjs');
const load=loader(process.argv[2]+'/entry/src/main/ets');
const {P051DerivedDemoV2:Demo}=load('physics/derived/P051DerivedDemoV2');
function draw(pub,ns,frameId){
 if(pub===null)return [null,null];
 const frameToken=[pub.run,pub.epoch,pub.commit,frameId].join(':');
 const source={frameToken,runId:pub.run,ledgerCommitKey:pub.ledgerKey,stateEpoch:pub.epoch,
  physicsCommitId:pub.commit,surfaceEpoch:7,contextEpoch:8,geometryEpoch:9,
  continuityEpoch:10,generation:11,configRevision:12};
 const frame={...source,fieldEpoch:pub.epoch,frameTimeNs:ns,frameId,requestedGeneration:11,appearanceConfigRevision:12};
 return [frame,{presentationState:'CURRENT',currentFrameSucceeded:true,requestedSwapSucceeded:true,
  requestedFrameToken:frameToken,renderedFrameToken:frameToken,renderedSource:source,targetSurfaceEpoch:7,
  targetContextEpoch:8,requestedGeneration:11,renderedGeneration:11,actualPresentationGeneration:11,
  requestedConfigRevision:12,renderedConfigRevision:12,displayPresentVerified:false}];
}
function callback(model,i,raw,change=null){
 const ns=1e9+i*20e6,now=ns/1e6;
 model.beginZ08Frame(now);
 const ok=model.tick(ns,raw,now);
 const slot=model.createPublicationCold();
 assert.equal(model.copyPublicationInto(slot),'OK');
 const pub=slot.readZ08Identity();
 const [frame,r]=draw(pub,ns,i);
 model.finishZ08Frame(ok?slot:null,ok?frame:null,ok?(change?change(r):r):null,ns,i,!ok,now);
 return model.exportZ08Cold()?JSON.parse(model.exportZ08Cold()):null;
}
const physics=[];
for(const restricted of [false,true]){
 const model=new Demo();
 assert.match(model.startZ08NearBed(restricted,1000),/^Z08_WAITING_BIND/);
 assert.equal(model.startZ08NearBed(restricted,1001),'Z08_ALREADY_ARMED');
 assert.equal(model.z08Dt(1/30),1/60);
 const inject=model.owner.inject;
 model.owner.inject=function(request){
  assert.equal(this.field.z08Archive,model.z08);
  assert(model.z08.arm.bound);
  return inject.call(this,request);
 };
 // Sensor and manual changes cannot override the fixture's selected angle.
 model.applyDefinedInput({valid:true,targetTiltDeg:-25,shakeInput:1,source:'sensor'});
 model.setTilt(-15);
 assert.equal(callback(model,1,1/30),null);
 assert.match(model.z08Status(),/1 帧 \/ 1 步/);
 const archive=callback(model,2,1/30);
 assert.equal(archive.arm.admission.admittedH,.3);
 assert.equal(archive.arm.admissionState,'ADMITTED');
 assert.equal(archive.arm.births.length,3);
 assert(archive.arm.births.every(b=>b.generation===1&&b.x===(restricted?63.99999:63.5)));
 assert.equal(archive.successfulSubsteps,2);
 assert.equal(archive.callbacks,2);
 assert.equal(archive.candidates.length,1);
 assert.equal(archive.landCurrentSwap,true);
 assert.equal(archive.systemPresentVerified,false);
 assert.equal(archive.candidates[0].dt,1/60);
 assert.equal(archive.candidates[0].effectiveTiltDeg,restricted?24:0);
 const land=archive.firstT3.lands;
 assert.equal(land.length,3);
 assert(land.every(l=>l.event.column===(restricted?64:63)&&l.event.targetColumn===-1&&l.verified));
 assert.deepEqual(land.map(l=>l.event.granted),restricted?[.1,.032121583881089655,0]:[.1,.1,.09999999999999998]);
 assert.deepEqual(land.map(l=>l.retired),restricted?[true,false,false]:[true,true,true]);
 assert(land.every(l=>Math.abs(l.event.reserved-l.event.refunded-l.event.granted)<1e-15));
 assert.equal(land.at(-1).bedFinal,restricted?.13212158388108966:.3);
 physics.push({fixture:archive.arm.fixture,grants:land.map(l=>l.event.granted),
  retirements:land.filter(l=>l.retired).length,steps:archive.successfulSubsteps,landCurrentSwap:archive.landCurrentSwap});
}
const lostDraw=new Demo();
lostDraw.startZ08NearBed(false,1000);
assert.equal(callback(lostDraw,1,1/60),null);
const bad=callback(lostDraw,2,1/60,r=>({...r,renderedSource:{...r.renderedSource,stateEpoch:99}}));
assert.equal(bad.captureStatus,'POSITIVE_LAND_NO_MATCHING_SWAP');
assert.equal(bad.landCurrentSwap,false);
assert.equal(bad.firstPositive.lands.length,3);

const model=new Demo();
assert(model.owner.inject({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:1,inputTimeMs:1000,
 x:20.5,y:100,seed:51,requestedH:.1,requestedL:0}).ok);
model.armZ08('512','FIRST_T3',1000);
let out=null;
for(let i=1;i<=16&&out===null;i++)out=callback(model,i,1/30);
assert.equal(out.arm.limit,'SUCCESSFUL_SUBSTEPS_8');
assert.equal(out.successfulSubsteps,8);
assert.equal(out.captureStatus,'TARGET_ID_NOT_OBSERVED');
model.armZ08('512','FIRST_T3',2000);
const arm=model.z08Id();
for(let i=1;i<=16;i++)out=callback(model,60+i,0);
assert.equal(out.arm.limit,'FOREGROUND_CALLBACKS_16');
assert.equal(out.callbacks,16);
assert.equal(out.successfulSubsteps,0);
model.armZ08('1','FIRST_T3',5000);
model.expireZ08(arm,30000);
assert.equal(model.exportZ08Cold(),'');
model.expireZ08(model.z08Id(),20000);
out=JSON.parse(model.exportZ08Cold());
assert.equal(out.arm.limit,'WALL_15S');
assert.equal(out.captureStatus,'NOT_BOUND');
assert.equal(out.callbacks,0);
model.armZ08('1','FIRST_T3',21000);
model.cancelZ08('PAGE_HIDDEN',21025);
assert.equal(JSON.parse(model.exportZ08Cold()).captureStatus,'PAGE_HIDDEN');
model.armZ08('1','FIRST_T3',22000);
model.reset(22050);
assert.equal(JSON.parse(model.exportZ08Cold()).captureStatus,'RESET');
const zero=new Demo();
assert(zero.owner.inject({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:1,inputTimeMs:1000,
 x:63.5,y:.004,seed:51,requestedH:.1,requestedL:0}).ok);
zero.armZ08('1','FIRST_T3',1000);
const b=zero.owner.field.particleBinding,apply=b.apply;
b.apply=function(scale,local,used,total,preview){
 local[63]=0;
 return apply.call(this,scale,local,used,total,preview);
};
let denied=null;
for(let i=1;i<=8&&denied===null;i++)denied=callback(zero,i,1/60);
assert.equal(denied.captureStatus,'ZERO_GRANT_T3');
assert.equal(denied.firstT3.lands[0].event.granted,0);
assert.equal(denied.firstPositive,null);
console.log(JSON.stringify({physics,zeroGrantCaptured:true,wrongEpochRejected:true,limits:['8','16','15s'],repeatCancelAndReset:true}));
