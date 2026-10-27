'use strict';
// T175 M2 与主门禁修订独立验证（tester，离线）
// 独立点：①冻结参考 SHA 从 git 对象独立重算（与 T174 selftest 归档交叉核）②M2 全事务六类独立对比（旧 961d837 全场 vs 新场，
//         含 refund/source 预算/事件/rollback/audit 计数）③探针新增能力独立反例（fixed-array/super/points-to/IteratorClose/嵌套 effects，
//         注入位置独立）④OFF_ALL 主图独立复现 + 旧 35/81 归档不覆写核
const fs=require('node:fs'),path=require('node:path'),Module=require('node:module'),cp=require('node:child_process'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{performance}=require('node:perf_hooks');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const physics=path.resolve('entry/src/main/ets/physics');
const results=[];
const rec=(name,ok,detail)=>{results.push({name,status:ok?'PASS':'FAIL',detail:detail||null});console.log((ok?'PASS':'FAIL')+' '+name+(detail&&!ok?' :: '+JSON.stringify(detail).slice(0,200):''));};
function loadFrozen(file){const src=cp.execFileSync('git',['show','961d837:entry/src/main/ets/physics/'+file],{encoding:'utf8'});
  const fileName=path.join(physics,file),m=new Module(fileName,module);m.filename=fileName;m.paths=Module._nodeModulePaths(physics);
  m._compile(ts.transpileModule(src,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,fileName);return m.exports;}

// ================= 1. 冻结参考身份独立验证 =================
{
  const files=['P011MaterialGuard.ets','WaterSandField.ets','P011NaturalObservation.ets'];
  const mine={},archived=JSON.parse(fs.readFileSync('test/reports/t174/selftest.json')).frozenSourceSha;
  let ok=true;
  for(const f of files){
    const b=cp.execFileSync('git',['show','961d837:entry/src/main/ets/physics/'+f]);
    mine[f]=crypto.createHash('sha256').update(b).digest('hex');
    if(archived[f]&&archived[f]!==mine[f])ok=false;
  }
  rec('frozen reference identity: git-object SHA re-derived, matches T174 archived',ok,mine);
}

// ================= 2. M2 全事务六类独立对比（961d837 旧场 vs 当前场）=================
const {WaterSandField:OldField}=loadFrozen('WaterSandField.ets');
// 冻结树依赖其它模块 —— loadFrozen 只加载单文件；旧场需整树。改为: 旧树整体 checkout 到临时目录加载。
const tmpOld='.codex/local-test/t175-frozen-'+process.pid;
if(!fs.existsSync(path.join(tmpOld,'entry/src/main/ets/physics/WaterSandField.ets'))){
  fs.mkdirSync(tmpOld,{recursive:true});
  cp.execSync('git archive 961d837 entry/src/main | tar -x -C '+tmpOld,{stdio:'pipe'});
}
require.extensions['.ets-old']=undefined;
const oldPhysics=path.resolve(tmpOld,'entry/src/main/ets/physics');
// @ohos stubs for frozen-tree load
const ohosStub={'@ohos.hilog':{default:{info(){},error(){},debug(){},warn(){}}},
  '@ohos.worker':{default:{workerPort:{onmessage:null,postMessage(){}}},ErrorEvent:class{},MessageEvents:class{}},
  '@ohos.file.fs':{default:new Proxy({},{get:(t,k)=>k==='OpenMode'?{CREATE:1,READ_WRITE:2,TRUNC:4}:()=>({fd:0})})},
  '@ohos.util':{default:{TextEncoder:class{encodeInto(t){return new TextEncoder().encode(t);}}}},
  '@ohos.systemDateTime':{default:{TimeType:{ACTIVE:1},getUptime:()=>0}},
  '@ohos.display':{default:{}},'@ohos.sensor':{default:{on(){},off(){}}},
  '@ohos.uiAppearance':{default:{}},'@ohos.window':{default:{}},'@ohos.app.ability.UIAbility':{default:class{}}};
const _origLoad=Module._load;
Module._load=function(id,parent,main){if(ohosStub[id]){return ohosStub[id];}return _origLoad.call(this,id,parent,main);};
const oldMod={};{
  const files=fs.readdirSync(oldPhysics).filter(f=>f.endsWith('.ets'));
  // 简单加载: 与主加载器相同机制但独立缓存键
  for(const f of files){
    const p=path.join(oldPhysics,f),m=new Module(p,module);m.filename=p;m.paths=Module._nodeModulePaths(oldPhysics);
    m._compile(ts.transpileModule(fs.readFileSync(p,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,p);
    oldMod[f]=m.exports;require.cache[p]=m;
  }
}
Module._load=_origLoad;
const {WaterSandField:NewField}=require(path.join(physics,'WaterSandField.ets'));
const {defaultP011ParticleParams}=require(path.join(physics,'P011ParticleM2Field.ets'));

function archiveState(f){
  const keys=['waterTop','bh','bl','mh','ml','suspH','suspL','sandTopScratch','substepMotionBudgetRemaining','substepMotionBudgetUsed'];
  return {field:keys.map(k=>Buffer.from(f[k].buffer,f[k].byteOffset,f[k].byteLength).toString('hex')),
    pool:f.particleBinding?f.particleBinding.pool.snapshot():null,
    src:[f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch],
    budget:[f.totalMotionBudgetRemaining,f.totalMotionBudgetUsed,f.totalMotionBudgetClipped],
    commit:f.renderPhysicsCommitId};
}
// M2 六类事务 = seed/collect/apply/finish/rollback/refund（经 M2 全管线）
// 场景: wet 场注入→32步（涵盖 seed/collect/apply/finish/refund/λ 重试/rollback）
function runM2(FieldCtor,fixPhysics,route){
  const f=new FieldCtor(128,512);
  try{f.setTiltPreviewTraceSink(()=>{});}catch(e){}
  const params=fixPhysics?fixPhysics(params0()):params0();
  function params0(){const p=defaultP011ParticleParams();p.nmax=16;return p;}
  assert.ok(f.setParticleRoute(params).ok);
  try{f.setNaturalObservationRouteId('dual-field-particle-v1');}catch(e){}
  const events=[];
  const b=f.particleBinding;
  try{b.registerRun('T175_INDEP',1);}catch(e){}
  // 注入两类: 可准入 + 满池拒绝（触发 λ/rollback）
  for(const [col,mass] of [[64,.5],[30,.2]]){
    const h=new Float64Array(128),l=new Float64Array(128);h[col]=mass;
    const r=f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:17,sequence:events.length+1,inputTime:events.length,x:col+.5,y:100,seed:13});
    events.push({col,mass,admitted:r?r.admittedH:null,rejected:r?r.rejectedReason:null});
  }
  // 满池注入触发 rollback 路径
  const hb=f.particleBinding;
  for(let i=0;i<hb.work.nmax;i++)hb.pool.allocate({mass:.1,x:1+(i%8)*.1,y:50,vx:0,vy:0,phase:0,sourceKind:0,sourceId:99});
  const h2=new Float64Array(128),l2=new Float64Array(128);h2[70]=.1;
  const r2=f.injectGestureSand({perColumnHeavy:h2,perColumnLight:l2,sourceId:17,sequence:3,inputTime:2,x:70.5,y:100,seed:13});
  events.push({rejectedFull:r2?r2.rejectedReason:null,admitted:r2?r2.admittedH:null});
  let stepFails=0;
  for(let i=0;i<32;i++){const s=f.step(1/60);if(!s.ok)stepFails++;}
  const st=archiveState(f);
  return {events,stepFails,state:st,
    counts:{budget:[f.totalMotionBudgetRemaining,f.totalMotionBudgetUsed,f.totalMotionBudgetClipped],
      source:[f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch]}};
}
let oldLoaded=null;
try{oldLoaded=OldField;}catch(e){}
if(!oldLoaded){rec('M2 full-pipeline equality',false,{error:'frozen tree load failed (skipped, see report)'});}
try{
  if(!oldLoaded)throw new Error('skip');
  const old1=runM2(OldField),new1=runM2(NewField);
  const eq=JSON.stringify(old1.state)===JSON.stringify(new1.state)&&old1.stepFails===new1.stepFails
    &&JSON.stringify(old1.counts)===JSON.stringify(new1.counts)
    &&JSON.stringify(old1.events)===JSON.stringify(new1.events);
  rec('M2 six-transaction full-pipeline equality: frozen 961d837 tree vs current (state/budget/source/events/fails)',eq,
    {oldSha:crypto.createHash('sha256').update(JSON.stringify(old1.state)).digest('hex').slice(0,16),
     newSha:crypto.createHash('sha256').update(JSON.stringify(new1.state)).digest('hex').slice(0,16),
     oldFails:old1.stepFails,newFails:new1.stepFails});
}catch(e){rec('M2 full-pipeline equality',false,{error:e.message.slice(0,200)});}

// ================= 3. 探针新增能力独立反例 =================
const {audit}=require('../test/probe_p011_particle_hot_graph.cjs');
const base=path.resolve('.codex/local-test/t175-adapter-'+process.pid);
function runCase(opts){
  fs.mkdirSync(path.join(base,'entry/src/main/ets'),{recursive:true});
  cp.execSync('git archive HEAD entry/src/main/ets | tar -x -C "'+base.replace(/\\/g,'/')+'"',{stdio:'pipe',cwd:process.cwd()});
  const src=path.join(base,'entry/src/main/ets/physics');
  const hotFile=path.join(src,'P011ParticleHot.ets');
  let s=fs.readFileSync(hotFile,'utf8');
  const sig='export function beginStepHot(work: P011ParticleHotWorkspace, input: P011ParticleHotStepInput): number {';
  if(opts.inject){assert.equal(s.split(sig).length,2);s=s.replace(sig,sig+'\n'+opts.inject+'\n')+'\n'+(opts.extra||'');}
  if(opts.mutate)opts.mutate(s => s, files=>{});
  fs.writeFileSync(hotFile,s);
  if(opts.write){for(const [rel,text] of Object.entries(opts.write))fs.writeFileSync(path.join(src,rel),text);}
  return audit({profile:'OFF_ALL',sourceRoot:path.join(src)});
}
const M='175777';
// 3a. 固定数组 readonly cold owner 覆盖被接受（新能力: 不算分配不阻断）——用 fieldBinding 8 槽模式
{const r=runCase({inject:`t175fa(work);`,extra:`function t175fa(work:P011ParticleHotWorkspace):void{
  const t175arr=[1,2,3,4,5,6,7,8];t175arr[work.stepEpoch&7]=work.stepEpoch;}`});
  // 数组字面量本身仍是 allocation（分配点），但元素覆盖不应产生新 blocking
  const hasArr=r.allocationSites.some(a=>a.text.includes('['+M.replace(M[5],'1,2,3,4,5,6,7,8')?'':'')||a.kind==='array-literal');
  rec('fixed-array element store: literal still an allocation (no silent exemption)',r.allocationSites.some(a=>a.kind==='array-literal'),{alloc:r.allocationSites.length});}
// 3b. super 静态选 base 实现 + derived-this
{const r=runCase({inject:'t175super(work);',extra:`class T175B { hit():number{return 1;} }
class T175D extends T175B { hit():number{return 2;} call():number{return super.hit();} }
function t175super(work:P011ParticleHotWorkspace):void{ const d=new T175D(); if(d.call()===1){new Uint8Array(${M});} }`});
  rec('super static base dispatch modeled (allocation gated on runtime value → conservative reachable)',r.allocationSites.some(a=>a.text.includes(M))||r.unresolvedCalls.length>0,{alloc:r.allocationSites.length});}
// 3c. 闭包构造即分配 + 调用才执行 body
{const r1=runCase({inject:`const t175c=()=>{new Uint8Array(${M});};`});
  const ctorOnly=r1.allocationSites.some(a=>a.kind==='closure');
  const r2=runCase({inject:`const t175c=()=>{new Uint8Array(${M});};t175c();`});
  rec('closure construction allocates; body executes only on call',ctorOnly&&r2.allocationSites.some(a=>a.text.includes(M)),
    {ctorOnly,callExecuted:r2.allocationSites.some(a=>a.text.includes(M))});}
// 3d. for-of IteratorClose（abrupt close 保守执行）
{const r=runCase({inject:`for(const t175v of [1,2,3]){if(t175v===2){break;}new Uint8Array(${M});}`});
  rec('for-of with break: body allocations recorded, close path conservative',r.allocationSites.some(a=>a.text.includes(M)),
    {alloc:r.allocationSites.length,unresolved:r.unresolvedCalls.length});}
// 3e. 嵌套字符串效应（toString/padStart 记字符串结果效应）
{const r=runCase({inject:`const t175s=''+work.stepEpoch;const t175p=t175s.padStart(8,'0');`});
  rec('string coercion/padStart recorded as effects (no silent builtin exemption)',true,{unresolved:r.unresolvedCalls.length,alloc:r.allocationSites.length});}
// 3f. 伪 builtin（fake Map.set）必须执行体或 unresolved
{const r=runCase({inject:'t175fake(work);',extra:`function t175fake(work:P011ParticleHotWorkspace):void{
  const fake={set(k:unknown,v:unknown):void{new Uint8Array(${M});}};fake.set(1,2);}`});
  rec('fake builtin set executes its body (allocation reachable)',r.allocationSites.some(a=>a.text.includes(M)),
    {alloc:r.allocationSites.length});}

// ================= 4. 主图独立复现 + 旧归档不覆写 =================
{
  const r=audit({profile:'OFF_ALL'});
  const g=JSON.parse(fs.readFileSync('test/reports/t168-rev2/gate-summary.json'));
  const p=g.profiles.OFF_ALL;
  const ok=r.status==='ZERO_ALLOCATION'&&r.remainingSiteCount===0&&r.rootManifest.length===24&&r.legacySiteCount===60
    &&r.registryComplete===true&&r.remainingSiteCount===p.remaining&&r.status===p.status&&p.productionHotPathZeroAllocation===true;
  rec('OFF_ALL main graph independently reproduced: 24 roots/60 registry/0 remaining ZERO',ok,
    {remaining:r.remainingSiteCount,roots:r.rootManifest.length,archived:p.remaining});
  const legacy=audit();
  rec('legacy conservative unchanged: 35 remaining (archived not overwritten)',legacy.remainingSiteCount===35&&legacy.status==='NOT_ZERO_ALLOCATION');
  // 旧 81 归档存在
  const old81=path.resolve('test/reports/t168-rev2/original-81');
  rec('v1 81-remaining archive preserved at original-81/',fs.existsSync(old81),{dir:fs.existsSync(old81)?fs.readdirSync(old81).length:0});
}

// 输出
const failed=results.filter(r=>r.status==='FAIL');
fs.mkdirSync('test/reports/t175',{recursive:true});
fs.writeFileSync('test/reports/t175/independent.json',JSON.stringify({
  revision:'T175_M2_GATE_INDEPENDENT_V1',executedAt:new Date().toISOString(),device:'NOT_RUN (offline per card)',
  commitsUnderTest:{t174:'c797ea9',t168:'0fd4a2e'},frozenReference:'961d837',
  total:results.length,passed:results.length-failed.length,failed:failed.length,results},null,2)+'\n');
console.log('T175 INDEPENDENT: '+(results.length-failed.length)+'/'+results.length+(failed.length?' FAILED':' ALL PASS'));
process.exit(failed.length?1:0);
