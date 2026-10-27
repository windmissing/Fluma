'use strict';
// Offline lifecycle contract tests; Worker/SDK are stubs, never device evidence.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const base=path.resolve('test/p013_compute/ets/t159');
function harness(options={}) {
  const workers=[],logs=[],resources=[],cache={};
  const context={cacheDir:'/cache',filesDir:'/files',resourceManager:{async getRawFileContent(name){resources.push(name);if(options.failResource)throw Error('RESOURCE_FAIL');return new Uint8Array([1,2]);}}};
  class Worker {constructor(url){this.url=url;workers.push(this);}postMessage(msg){this.message=msg;}}
  class Ability {constructor(){this.context=context;}}
  const fsStub={OpenMode:{CREATE:1,READ_WRITE:2,TRUNC:4},accessSync:()=>true,mkdirSync:()=>{},openSync:()=>({fd:1}),writeSync:(fd,b)=>b.byteLength,fsyncSync:()=>{},closeSync:()=>{}};
  function load(name){
    if(cache[name])return cache[name].exports;
    const m={exports:{}};cache[name]=m;
    const source=fs.readFileSync(path.join(base,name+'.ets'),'utf8');
    const output=ts.transpileModule(source,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText;
    vm.runInNewContext(output,{exports:m.exports,require(id){
      if(id.includes('T165PageLauncher'))return load('T165PageLauncher');
      if(id.includes('T159BuildIdentity'))return {T159_SOURCE_SHA:'a'.repeat(64),T159_SHADER_SHA:['b'.repeat(64),'c'.repeat(64),'d'.repeat(64)]};
      const sdk=id==='@ohos.worker'?{ThreadWorker:Worker}:id==='@ohos.file.fs'?fsStub:id==='@ohos.hilog'?{info:(...x)=>logs.push(x),error:(...x)=>logs.push(x)}:id==='@ohos.app.ability.UIAbility'?Ability:{};
      return {default:sdk};
    },console});return m.exports;
  }
  return {launcher:load('T165PageLauncher').t165PageLauncher,Ability:load('T159EntryAbility').default,workers,logs,context,resources};
}
const want={parameters:{t159buildManifestSha256:'e'.repeat(64),t159runId:'T165_OFFLINE',t159route:'GPU',t159nmax:192,t159steps:2}};
const flush=async()=>{for(let i=0;i<12;i++)await Promise.resolve();};
const results=[];
async function test(name,fn){await fn();results.push({name,status:'PASS'});}
(async()=>{
 await test('loadContent pending + page ready cannot create Worker',async()=>{
  const h=harness(),ability=new h.Ability();ability.onCreate(want);let resolve;
  const done=ability.onWindowStageCreate({loadContent(p){assert.equal(p,'pages/Index');return new Promise(r=>{resolve=r;});}});
  h.launcher.pageReady();await flush();assert.equal(h.workers.length,0);assert.equal(h.resources.length,0);
  resolve();await done;await flush();assert.equal(h.workers.length,1);
 });
 await test('loadContent complete waits for page; repeat page callbacks are one-shot',async()=>{
  const h=harness(),a=new h.Ability();a.onCreate(want);await a.onWindowStageCreate({loadContent:async()=>{}});
  assert.equal(h.workers.length,0);h.launcher.pageReady();h.launcher.pageReady();h.launcher.configure(h.context,want);await flush();
  assert.equal(h.workers.length,1);const w=h.workers[0];assert.equal(w.url,'entry/ets/workers/T159PhysicsWorker.ets');
  assert.equal(w.message.route,'GPU');assert.equal(w.message.config.nmax,192);assert.equal(w.message.config.sourceManifestSha256,'a'.repeat(64));
  assert.equal(w.message.config.buildManifestSha256,want.parameters.t159buildManifestSha256);assert.equal(h.resources.length,3);
  assert.equal(w.message.output,'/files/t159-T165_OFFLINE-GPU.jsonl');
  w.onerror({message:'LOAD_FAILED'});assert.ok(h.logs.some(x=>x.includes('WORKER_FAULT %{public}s')));
 });
 await test('loadContent failure prevents configuration and launch',async()=>{
  const h=harness(),a=new h.Ability();a.onCreate(want);h.launcher.pageReady();await a.onWindowStageCreate({loadContent:async()=>{throw Error('LOAD_FAIL');}});await flush();
  assert.equal(h.workers.length,0);assert.ok(h.logs.some(x=>x.includes('LOAD_CONTENT_FAULT %{public}s')));
 });
 await test('missing/invalid cold identity fails closed',async()=>{
  for(const bad of [null,{parameters:{...want.parameters,t159route:'BAD'}},{parameters:{...want.parameters,t159buildManifestSha256:'bad'}}]){
   const h=harness();h.launcher.pageReady();h.launcher.configure(h.context,bad);await flush();assert.equal(h.workers.length,0);assert.ok(h.logs.length>0);
  }
 });
 await test('resource failure cannot launch or silently retry',async()=>{
  const h=harness({failResource:true});h.launcher.pageReady();h.launcher.configure(h.context,want);await flush();h.launcher.pageReady();await flush();
  assert.equal(h.workers.length,0);assert.equal(h.resources.length,1);assert.ok(h.logs.some(x=>x.includes('RESOURCE_FAIL')));
 });
 await test('all route choices preserve cold config and diagnostic switch',async()=>{
  for(const route of ['LEGACY_SCAN','T123','GPU']){
   const h=harness();h.launcher.configure(h.context,{parameters:{...want.parameters,t159route:route,t159diagnosticOracle:false}});h.launcher.pageReady();await flush();
   assert.equal(h.workers.length,1);assert.equal(h.workers[0].message.route,route);assert.equal(h.workers[0].message.config.diagnosticOracle,false);
  }
 });
 await test('page callback and generator register only isolated launcher',async()=>{
  assert.match(fs.readFileSync(path.join(base,'T159Index.ets'),'utf8'),/onPageShow\(\): void \{ t165PageLauncher.pageReady\(\); \}/);
  const project=path.resolve('.codex/local-test/t159-project');
  const manifest=JSON.parse(fs.readFileSync(path.join(project,'t159-source-manifest.json')));
  assert.equal(manifest.productionTouched,false);assert.ok(manifest.inputs['test/p013_compute/ets/t159/T165PageLauncher.ets']);
  assert.deepEqual(manifest.changes.map(x=>x.edits.length),[6,4,3]);
  assert.ok(JSON.parse(fs.readFileSync(path.join(project,'entry/build-profile.json5'))).buildOption.sourceOption.workers.includes('./src/main/ets/workers/T159PhysicsWorker.ets'));
 });
 fs.mkdirSync('test/reports/t165',{recursive:true});fs.writeFileSync('test/reports/t165/selftest.json',JSON.stringify({status:'PASS',passed:results.length,failed:0,device:'NOT_RUN',worker:'SDK_STUB_ONLY',results},null,2)+'\n');
 console.log(JSON.stringify({passed:results.length,failed:0}));
})().catch(e=>{console.error(e);process.exitCode=1;});
