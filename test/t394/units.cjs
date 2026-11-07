const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),Module=require('node:module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const root=path.resolve(process.argv[2],'entry/src/main/ets'),cache=new Map();
function load(name){
 const file=path.resolve(root,name+'.ets');
 if(cache.has(file))return cache.get(file).exports;
 const m=new Module(file);cache.set(file,m);
 m.require=id=>{
  if(id==='libp013renderer.so')return {};
  if(id==='@ohos.hilog')return {info(){},warn(){},error(){}};
  if(id==='@ohos.systemDateTime')return {getUptime:()=>Number(process.hrtime.bigint()),TimeType:{ACTIVE:0}};
  if(id==='@ohos.file.fs'||id==='@ohos.util'||id==='@ohos.security.cryptoFramework')return {};
  if(id.startsWith('.'))return load(path.relative(root,path.resolve(path.dirname(file),id)));
  return require(id);
 };
 m._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),{compilerOptions:{
  module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020,esModuleInterop:true
 }}).outputText,file);
 return m.exports;
}
const {P051DerivedDemoV2}=load('physics/derived/P051DerivedDemoV2');
const {P013SingleSandController}=load('render/P013SingleSandController');
const {T394Input}=load('render/T394Phase');
const {inspectWindowFixture}=load('render/RoiFixture');
const {parseWindowRoi}=load('render/SingleSandWindowCold');
let surface=2,prepareOK=true,nativeOK=true,frame=0,readbacks=0,normal=0,reads=null;
const window={contextEpoch:1,widthPx:1200,heightPx:1050,sizeSource:'EGL_QUERY_SURFACE'};
const native={
 init(){return {code:0,sessionId:1};},
 getState(){return {...window,surfaceEpoch:surface};},
 prepareDualFieldSurface(){return {...this.getState(),code:prepareOK?0:12};},
 registerSingleSandConfiguration(){return {reason:'OK',shaderHash:'shader'};},
 stageSingleSandGrain(){return {reason:'OK'};},
 drawSingleSandScene(packet,profile,xy,meta,roi){
  assert.equal(packet.count*2<=xy.length,true);
  const receipt={currentFrameSucceeded:nativeOK,swapSucceeded:nativeOK,presentationState:nativeOK?'CURRENT':'STALE',
   firstFailureStage:nativeOK?'OK':'SWAP',reason:nativeOK?'OK':'SINGLE_DRAW_FAILED',
   requestedToken:packet.token,actualToken:nativeOK?packet.token:'last-good',rawGL:0,rawEGL:nativeOK?0:0x300d};
  if(roi){
   readbacks++;
   reads={manifest:{requestId:packet.probeRequestId,actualToken:receipt.actualToken,requestedToken:packet.token,
    captureStatus:nativeOK?'CAPTURED':'CAPTURE_FAILED',captureStage:nativeOK?'COMPLETE':'SWAP'},
    coords:Array.from(roi)};
  }else normal++;
  return receipt;
 },
 takeSingleSandWindowProbe(){const result=reads;reads=null;return result;}
};
const model=new P051DerivedDemoV2(32),ctl=new P013SingleSandController(native);
const draw=(dt=0)=>{
 if(dt){model.t394StartTrace();assert(model.tick(1e9+frame*16666667,dt),model.status);}
 const steps=dt?model.t394FinishTrace():[];
 const p=model.createPublicationCold();
 assert.equal(model.copyPublicationInto(p),'OK');
 const c={boxX:0,boxY:0,boxW:400,boxH:350,fieldEpoch:p.stateEpoch,runIdentity:p.runId,
  physicsCommitId:p.physicsCommitId,geometryEpoch:1,continuityEpoch:1,frameSerial:++frame,
  particleLayerEnabled:true,visualInverted:false};
 if(ctl.t394WantsEvidence())ctl.t394Frame(new T394Input(frame*16666667,.01,dt,p.effectiveTiltDeg,steps,model.t394InputReceipts()));
 return p.drawSingleSand(ctl,c,3);
};
ctl.beginT394('LOCKED',true);
assert.equal(ctl.t394PhaseStatus(),'WAITING_LOCKED_FRAME');
assert(draw().currentFrameSucceeded);
assert.equal(ctl.t394PhaseStatus(),'EMPTY_BED');
assert.equal(readbacks,0);
assert(ctl.armWindowProbe(parseWindowRoi('0,0,16,16'),''));
draw();
assert.equal(ctl.takeWindowProbe().manifest.captureStatus,'CAPTURED');
assert(ctl.t394Hold());
ctl.ackWindowProbe();

model.startT394Fixture(true,100);
const staticArms=[];
for(const neutral of [true,false]){
 ctl.setProbeNeutral(neutral);
 ctl.beginT394(neutral?'STATIC_NEUTRAL':'STATIC_TEXTURE',true);
 assert.equal(ctl.windowFixture(),'');
 assert(draw().currentFrameSucceeded);
 const frozen=ctl.t394.sample;
 assert.equal(frozen.authority.bedMass,6144);
 assert.equal(frozen.authority.initialBed,6144);
 assert.equal(frozen.authority.inFlight,.1);
 const roi=parseWindowRoi(ctl.windowFixture());
 assert.equal(roi?.length,12,ctl.t394PhaseStatus());
 assert(roi[2]>=16&&roi[6]>=16&&roi[10]>=16);
 assert(ctl.armWindowProbe(roi,ctl.windowFixtureKey()));
 assert(!ctl.armWindowProbe(roi,ctl.windowFixtureKey()));
 draw();
 assert.equal(ctl.t394.sample,frozen);
 const evidence=ctl.takeWindowProbe();
 assert.equal(evidence.phase.sample.token,evidence.manifest.requestedToken);
 assert.equal(evidence.phase.hold.token,evidence.manifest.actualToken);
 assert.equal(evidence.phase.sample.authority.bedMass,6144);
 staticArms.push(evidence.phase.sample);
 ctl.ackWindowProbe();
}
assert.notEqual(staticArms[0].token,staticArms[1].token);
assert.notEqual(staticArms[0].digest,staticArms[1].digest);
assert.equal(staticArms[0].rawHash,staticArms[1].rawHash);
assert.equal(staticArms[0].particleHash,staticArms[1].particleHash);
const prep=ctl.preparation,p=prep.packet;
assert.equal(inspectWindowFixture(p,new Float32Array(256).fill(.01),prep.slot.particleXY,prep.slot.particleMeta).status,'BED_ROI_UNSUPPORTED');
assert.equal(inspectWindowFixture(p,prep.profile,prep.slot.particleXY,new Uint32Array(prep.slot.particleMeta.length)).status,'AIR_ROI_UNSUPPORTED');
assert.equal(inspectWindowFixture({...p,width:1},prep.profile,prep.slot.particleXY,prep.slot.particleMeta).status,'GEOMETRY_UNSUPPORTED');

ctl.beginT394('LOCKED',true);draw();
assert(ctl.windowFixture());
assert(ctl.armWindowProbe(parseWindowRoi(ctl.windowFixture()),ctl.windowFixtureKey()));
prepareOK=false;
assert.equal(draw(),null);
assert.equal(ctl.windowFixture(),'');
assert.equal(ctl.lastReceipt.currentFrameSucceeded,true);
assert.equal(ctl.takeWindowProbe().manifest.captureStage,'TARGET:SINGLE_TARGET_UNSUPPORTED');
assert.equal(ctl.takeWindowProbe().phase.prepareNativeCode,12);
ctl.ackWindowProbe();
prepareOK=true;draw();
assert(ctl.windowFixture());
surface++;nativeOK=false;draw();
assert.equal(ctl.windowFixture(),'');
assert.equal(JSON.parse(ctl.exportT394Phase()).receipt.rawEGL,0x300d);
nativeOK=true;

for(const arm of ['CONTROL','LOCK']){
 model.startT394Fixture(false,100);
 ctl.beginT394(arm+'_A',arm==='LOCK',true);
 draw(arm==='CONTROL'?1/60:0);
 const a=ctl.takeWindowProbe();
 assert(a.phase.hold);
 assert.equal(a.phase.sample.authority.bedMass,0);
 assert.deepEqual(a.phase.sample.authority.particles.map(p=>p.id),[1]);
 ctl.ackWindowProbe();
 ctl.beginT394(arm+'_B',false,true,true);
 model.t394InjectB(101);draw(1/60);
 const b=ctl.takeWindowProbe();
 assert(b.phase.hold,JSON.stringify(b.phase));
 assert.deepEqual(b.coords,a.coords);
 assert.equal(b.phase.sample.digest,a.phase.sample.digest);
 assert.notEqual(b.phase.sample.particleHash,a.phase.sample.particleHash);
 assert.equal(b.phase.input.steps.length,1);
 assert.equal(b.phase.input.admissions[1].admittedH,.1);
 assert.deepEqual(b.phase.sample.authority.particles.map(p=>p.id),[1,2]);
 assert.equal(b.phase.sequence,a.phase.sequence+1);
 assert.equal(ctl.t394VerifyHold().token,b.phase.hold.token);
 ctl.ackWindowProbe();
 if(arm==='LOCK'){
  ctl.beginT394('LOCK_RELOCK',true,false,true);draw();
  assert.equal(ctl.t394.sample.authority.commit,b.phase.sample.authority.commit);
 }
}
model.reset();
assert(model.owner.inject({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:1,inputTimeMs:100,
 x:63.5,y:.004,seed:51,requestedH:.1,requestedL:0}).ok);
let positiveLand=null;
for(let i=0;i<12&&positiveLand===null;i++){
 model.t394StartTrace();
 assert(model.tick(2e9+i*16666667,1/60),model.status);
 for(const step of model.t394FinishTrace()){
  if(step.lands.length)positiveLand=step.lands[0];
 }
}
assert.equal(positiveLand.id,1);
assert.equal(positiveLand.column,63);
assert.equal(positiveLand.granted,.1);
console.log(JSON.stringify({staticArms:2,authoritativeBed:6144,dynamicArms:2,
 staleRevocation:true,nativeFailureCodePreserved:true,readbacks,ordinaryDraws:normal,positiveLand}));
