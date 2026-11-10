'use strict';
const fs=require('node:fs'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
function load(src,imports={}){
 const code=ts.transpileModule(src,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS,esModuleInterop:true}}).outputText;
 const m={exports:{}};
 new Function('require','exports','module',code)(key=>{assert(key in imports,key);return imports[key];},m.exports,m);
 return m.exports;
}
const results=[];
function check(target,name,f){f();results.push({target,name});}
for(const target of ['t410','t437','regular']){
 const read=name=>fs.readFileSync(`.codex/local-test/t450-${target}/entry/src/main/${name}`,'utf8');
 const profile=load(read('ets/physics/T450CaptureProfile.ets'));
 const helper=()=>load(read('ets/physics/GpuCaptureLaunch.ets'),{'./T450CaptureProfile':profile});
 const raw=read('ets/pages/GpuResidentM1.ets'),ability=read('ets/entryability/EntryAbility.ets');
 check(target,'cold launch only and diagnostic display is read-only',()=>{
  assert.match(ability,/initializeCaptureLaunch\(want.parameters\?\.grsm1, want.parameters\?\.gpusha\)/);
  assert.equal(ability.match(/initializeCaptureLaunch\(/g).length,1);
  assert.doesNotMatch(raw,/TextInput\(\{placeholder:'诊断签包 SHA256/);
  assert.equal(raw.includes('setLedgerPrefix('),target==='t437');
 });
 const valid='AB'.repeat(32),scope={epoch:2,readyEpoch:2,runHi:0,runLo:1,pageSession:'7',progressClockRevision:'GRS_M1_PROGRESS_CLOCK_V2'};
 for(const [route,sha,reason] of [[true,valid,'SHA_DECLARED'],[true,undefined,'CAPTURE_SHA_MISSING'],
   [true,'','CAPTURE_SHA_MISSING'],[true,'a'.repeat(63),'CAPTURE_SHA_INVALID'],[true,'a'.repeat(65),'CAPTURE_SHA_INVALID'],
   [true,' '+valid.slice(1),'CAPTURE_SHA_INVALID'],[true,valid.slice(1)+'\n','CAPTURE_SHA_INVALID'],
   [true,'Ａ'+valid.slice(1),'CAPTURE_SHA_INVALID'],[true,1,'CAPTURE_SHA_INVALID'],[false,valid,'CAPTURE_WRONG_ROUTE'],
   ['true',valid,'CAPTURE_WRONG_ROUTE']]){
  check(target,`${reason}/${typeof sha}/${String(sha).length}`,()=>{
   const h=helper();h.initializeCaptureLaunch(route,sha);const cold=h.captureLaunch();
   assert.equal(cold.reason,target==='regular'?'CAPTURE_UNSUPPORTED':reason);
   h.initializeCaptureLaunch(true,'0'.repeat(64));assert.equal(h.captureLaunch(),cold);
   assert.equal(cold.sha,target!=='regular'&&reason==='SHA_DECLARED'?valid.toLowerCase():'');
  });
 }
 const pageJs=raw.slice(0,raw.indexOf(' build(){')).replace('@Entry @Component','')
  .replaceAll('@State ','').replace(/@StorageLink\([^\n]+?private foreground/,'private foreground')
  .replace('struct GpuResidentM1','export class GpuResidentM1')+'}\n';
 const h=helper();h.initializeCaptureLaunch(true,valid);
 let status={...scope},called=[],native={state:'RESERVED',requestId:'91'};
 const port={status:()=>JSON.stringify(status),beginEvidenceCapture:arg=>{called.push(JSON.parse(arg));return JSON.stringify(native);}};
 const P=load(pageJs,{'../physics/GpuCaptureLaunch':h,'libgrsm1.so':port,
  '@ohos.process':{},'@kit.AbilityKit':{},'@ohos.sensor':{},'@ohos.systemDateTime':{},
  '../physics/GpuEvidenceCaptureCold':{GpuEvidenceCaptureCold:class {}},'../physics/GpuBridgeTimelineCold':{GpuBridgeTimelineCold:class {}},
  '../physics/DiagnosticColdFiles':{},'@ohos.arkui.UIContext':{FrameCallback:class {}},
  '../physics/P051GravityInputAdapter':{P051GravityConfig:class {},P051GravityInputAdapter:class {}},
  '../physics/GpuM1DemoController':{GpuM1DemoController:class {}}}).GpuResidentM1;
 const p=new P();p.active=true;p.frame.session='7';
 check(target,'generated page only submits explicit arm with native current scope',()=>{
  assert.equal(called.length,0);p.armCapture();assert.equal(called.length,target==='regular'?0:1);
  if(target==='regular')return;
  assert.deepEqual(called[0],{packageSha256:valid.toLowerCase(),expectedRevision:scope.progressClockRevision,epoch:'2',
   runLo:1,runHi:0,pageSession:'7',durationNs:'2000000000',maxRows:4096,maxBytes:1048576});
  p.updateCapture({state:'FROZEN',requestId:'91',captureId:'6'});
  assert.equal(p.archivedLaunchAttempt.requestId,'91');assert.equal(p.captureId,'6');
 });
 check(target,'stale scope and late or rejected replies leave frozen identity alone',()=>{
  if(target==='regular')return;
  status={...scope,pageSession:'8'};p.armCapture();assert.equal(called.length,1);assert.match(p.captureText,/STALE_PAGE_SESSION/);
  status={...scope,epoch:3};p.armCapture();assert.equal(called.length,1);assert.match(p.captureText,/STALE_OR_UNREADY_EPOCH/);
  status={...scope,runLo:0};p.armCapture();assert.equal(called.length,1);assert.match(p.captureText,/INVALID_RUN/);
  p.updateCapture({state:'FROZEN',requestId:'90',captureId:'5'});assert.equal(p.captureId,'6');
  status={...scope,runLo:2};native={state:'REJECTED',reason:'IDENTITY_MISMATCH'};p.armCapture();
  assert.equal(called.length,2);assert.equal(p.captureId,'6');assert.equal(p.archivedLaunchAttempt.requestId,'91');
  native={state:'REJECTED',reason:'INVALID_CAPTURE_CONFIG',invalidField:'epoch'};p.armCapture();
  assert.equal(p.lastLaunchAttempt.invalidField,'epoch');assert.equal(p.captureId,'6');
 });
}
fs.mkdirSync('test/reports/t450',{recursive:true});
fs.writeFileSync('test/reports/t450/page-host.json',JSON.stringify({passed:results.length,failed:0,device:'NOT_RUN',results},null,2)+'\n');
console.log(`${results.length}/0`);
