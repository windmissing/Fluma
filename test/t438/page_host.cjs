'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
function load(source,imports={}){
 const js=ts.transpileModule(source,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS,esModuleInterop:true}}).outputText;
 const mod={exports:{}};
 new Function('require','module','exports',js)(name=>{assert(name in imports,name);return imports[name];},mod,mod.exports);
 return mod.exports;
}
const rows=[];
function check(profile,name,fn){fn();rows.push({profile,name,passed:true});}
for(const profile of ['regular','experiment']){
 const dir=process.argv[profile==='regular'?2:3]??`.codex/local-test/t438-${profile}`;
 const main=path.join(dir,'entry/src/main');
 const read=rel=>fs.readFileSync(path.join(main,rel),'utf8');
 const experiment=profile==='experiment';
 const controller=load(read('ets/physics/GpuM1DemoController.ets')).GpuM1DemoController;
 const raw=read('ets/pages/GpuSandWaterExperience.ets');
 const lifecycle=raw.slice(0,raw.indexOf('  build() {')).replace('@Entry @Component','')
  .replace(/@StorageLink\([^\n]+?private foreground/,'private foreground').replaceAll('@State ','')
  .replace('struct GpuSandWaterExperience','export class GpuSandWaterExperience')+'}\n';
 const queue=[], packets=[], samples=[], angles=[], order=[];
 let requested='V1',clock='V1',resets=0,session=0,subscription=null;
 const status={state:'READY',ready:true,permissionState:'READY',epoch:1,readyEpoch:1,runLo:1,runHi:1,
  geometryLo:1,geometryHi:0,inputLo:1,inputHi:0,configLo:1,configHi:0,sourceUiRevision:'GRS_M1_DEMO_UI_V4',
  sourceOpen:false,sourceKind:-1,sourceControlLo:1,sourceControlHi:0,sourceAdmittedQ:0,sourceRejectedQ:0,
  flightLo:0,flightHi:0,settledLo:0,settledHi:0,sourceClosePending:false};
 const port={
  status:()=>JSON.stringify({...status,progressClockRevision:'GRS_M1_PROGRESS_CLOCK_'+clock}),
  experienceCapabilities:()=>JSON.stringify({revision:'GRS_M1_EXPERIENCE_V1',buildProfile:profile,K:192,Kw:128,features:3,GRS_PROGRESS_V2_EXPERIMENT:experiment}),
  newRun(){resets++;status.runLo++;clock=requested;order.push('newRun');return 'NEW_RUN_REQUESTED';},
  setProgressV2(on){if(!experiment)return 'PROGRESS_V2_UNSUPPORTED';requested=on?'V2':'V1';order.push('select'+requested);return `PROGRESS_${requested}_NEXT_RUN`;},
  setSource(kind){status.sourceKind=kind;status.sourceOpen=kind>=0;status.sourceControlLo++;return 'OK';},
  setActive(value){order.push(value?'resume':'pause');return 'OK';},setGravityMode(){return 'OK';},
  setGravitySample(...args){samples.push(args);return 'SENSOR_TARGET_ENQUEUED_NOT_ACCEPTED';},
  setAngle(angle){angles.push(angle);return 'ANGLE_ENQUEUED';},
  offer(packet){packets.push(Buffer.from(new Uint8Array(packet)));return 'ENQUEUED_NOT_ACK';},
  beginPageSession(){return String(++session);},endPageSession(value){assert.equal(value,String(session));order.push('endSession');},
  pagePulse(s,seq,ns){assert.equal(s,String(session));assert(Number(seq)>0);assert(Number(ns)>=0);return 'PAGE_PULSE_ACCEPTED';}
 };
 const sensor={SensorId:{ACCELEROMETER:1},on(_,fn){assert.equal(subscription,null);subscription=fn;},off(_,fn){assert.equal(subscription,fn);subscription=null;}};
 const Page=load(lifecycle,{'libgrsm1.so':port,'@ohos.sensor':sensor,
  '@ohos.systemDateTime':{TimeType:{ACTIVE:0},getUptime:()=>1000000},
  '@ohos.arkui.UIContext':{FrameCallback:class {}},'../physics/GpuM1DemoController':{GpuM1DemoController:controller},
  '../physics/P051GravityInputAdapter':{P051GravityConfig:class {},P051GravityInputAdapter:class {
   consumeAccelerometer(x){return {valid:Number.isFinite(x),targetTiltDeg:x,shakeInput:0};}reset(){}
  }}}).GpuSandWaterExperience;
 const page=new Page();page.getUIContext=()=>({postFrameCallback(frame){queue.push(frame);}});
 global.TouchType={Down:0,Move:1,Up:2,Cancel:3};
 const touch=(type,x,y)=>page.touch({type,changedTouches:[{id:1,x,y}]});
 const tick=ns=>{const frame=queue.shift();assert(frame);frame.onFrame(ns);};
 check(profile,'isolated ordinary menu and same XComponent owner',()=>{
  assert.match(read('ets/entryability/EntryAbility.ets'),/P013P1G0Probe' : 'pages\/Index'/);
  assert.match(read('ets/pages/Index.ets'),/router.pushUrl\(\{ url: 'pages\/GpuSandWaterExperience'/);
  assert(JSON.parse(read('resources/base/profile/main_pages.json')).src.includes('pages/GpuSandWaterExperience'));
  assert.match(raw,/XComponent\(.*libraryname: 'grsm1'/);
  assert.doesNotMatch(raw,/Canvas|loadSnapshot|configureM0/);
  assert.match(read('cpp/CMakeLists.txt'),new RegExp('option\\(T438_M1_EXPERIMENT_V2 .* '+(experiment?'ON':'OFF')+'\\)'));
 });
 check(profile,'first explicit entry requests one empty run',()=>{
  page.onPageShow();page.onPageShow();assert.equal(resets,1);assert.equal(session,1);
  page.model.geometry(128,256);tick(100);assert.equal(page.progress,'V1');assert.equal(page.model.snapshot.runLo,2);
 });
 check(profile,'DOWN/MOVE/UP routes both source kinds on one run',()=>{
  touch(0,64,100);tick(1000);touch(1,32,150);tick(10001000);touch(2,32,150);
  page.model.select(1);touch(0,64,100);tick(20001000);tick(30001000);touch(2,64,100);tick(40001000);
  assert.equal(status.sourceKind,-1);assert.equal(resets,1);assert.equal(packets.length,2);
  assert.deepEqual(packets.map(b=>b.readUInt32LE(8)),[0,1]);
  assert(packets.every(b=>b.readUInt32LE(16)===2));assert.equal(packets[0].readDoubleLE(64),32);
 });
 check(profile,'background cancels old callback and resume requires new DOWN',()=>{
  page.foreground=false;page.syncActive();assert.equal(subscription,null);const old=queue.shift();
  old.onFrame(50001000);assert.equal(queue.length,0);
  page.foreground=true;page.syncActive();tick(60001000);assert.equal(resets,1);assert.equal(packets.length,2);
  assert.equal(session,2);assert(subscription);
 });
 check(profile,'manual and sensor paths are mutually exclusive',()=>{
  subscription({x:4,y:0,z:0});page.model.manualAngle(6);assert.equal(samples.length,1);assert.equal(angles.length,0);
  const old=subscription;page.manual=true;page.model.gravityMode(false);page.updateSensor();assert.equal(subscription,null);
  old({x:7,y:0,z:0});page.model.manualAngle(6);assert.equal(samples.length,1);assert.deepEqual(angles,[1536]);
 });
 check(profile,'V2 rejects or cold-selects with no optimistic label',()=>{
  page.selectProgress(true);assert.equal(page.progress,'V1');
  if(experiment){assert.equal(resets,2);assert.deepEqual(order.slice(-2),['selectV2','newRun']);tick(80001000);assert.equal(page.progress,'V2');}
  else {assert.equal(page.operation,'PROGRESS_V2_UNSUPPORTED');assert.equal(resets,1);}
 });
 check(profile,'native resource fault preserves reason and blocks DOWN',()=>{
  Object.assign(status,{state:'FAULT',stage:'TRANSPORT_CREATE',code:5,ready:false});
  tick(120001000);assert.match(page.stateText,/MEMORY_PROFILE_UNSUPPORTED #5/);
  touch(0,64,100);assert.equal(page.operation,'NOT_READY');assert.equal(packets.length,2);
 });
 check(profile,'hide/show closes and renews page session without resetting run',()=>{
  const before=resets;page.onPageHide();page.onPageShow();assert.equal(resets,before);assert.equal(session,3);
  page.onPageHide();assert.equal(page.frame.owner,null);
 });
}
const report={schema:'T438_DEVELOPER_PAGE_HOST_V1',device:'NOT_RUN',passed:rows.length,failed:0,rows};
fs.mkdirSync('test/reports/t438',{recursive:true});
fs.writeFileSync('test/reports/t438/page-host.json',JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({passed:rows.length,failed:0}));
