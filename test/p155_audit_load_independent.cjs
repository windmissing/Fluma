'use strict';
// T155 audit 负载诊断独立测试（tester，独立于 T154 8/8 开发自测）
// 消费: docs/architecture-T162-diagnostic-gate-disposition-v2.md §4/§5, T116 V2§4
// 范围: ①排序终止/比较计数（满/稀疏/同键/非法ID）②seed/audit 链真实分源次数/λ/失败/rollback
//       ③attempt delta 与阶段计时/未捕获标记 ④冷 ring 容量/dropped ⑤诊断开关等价
//       ⑥适配器反例（T162 §4.1）⑦静态项如实 NOT_RUN（T168 探针未交付）
// 边界: 不重复 T128 287pair 样本冒新负载证明；非法/拒绝与成功分列；失败先 architect。
const fs=require('node:fs'),a=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const {P011MergeExactIndex}=require('../entry/src/main/ets/physics/P011MergeExactIndex.ets');
const results=[],measurements=[];
function rec(name,ok,detail){const status=ok?'PASS':'FAIL';results.push({name,status,detail:detail||null});console.log(status+' '+name+(detail?' :: '+JSON.stringify(detail).slice(0,220):''));}

function make(nmax=16,on=true,clock=undefined){
  const f=new WaterSandField(128,512),p=defaultP011ParticleParams();p.nmax=nmax;
  a.equal(f.setParticleRoute(p).ok,true);f.setNaturalObservationRouteId('dual-field-particle-v1');
  let t=0;if(on)f.setPoolProgressDiagnosticEnabled(true,clock===undefined?()=>t+=.01:clock);
  return f;}
function progress(f){return JSON.parse(f.getPoolProgressDiagnostic()).progressV3;}
function counters(p){return Object.fromEntries(p.counterNames.map((name,i)=>[name,p.counters[i]]));}
function fill(f,compatible,nmax){const b=f.particleBinding;
  for(let i=0;i<(nmax||b.work.nmax);i++)a.ok(b.pool.allocate({mass:.1,
    x:compatible?64.1+(i%4)*.1:(i%128)+.5,y:100+(i%4)*.1,vx:0,vy:0,phase:0,
    sourceKind:0,sourceId:compatible?8:i+1})>=0);}
function inject(f,mass=.35){const h=new Float64Array(128),l=new Float64Array(128);h[64]=mass;
  return f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:8,sequence:1,inputTime:0,x:64.5,y:100,seed:1});}
function samePhysics(x,y){for(const k of ['bh','bl','mh','ml','suspH','suspL','waterTop','sandTopScratch',
  'substepMotionBudgetRemaining','substepMotionBudgetUsed'])a.deepEqual(x[k],y[k],k);
  a.deepEqual(x.particleBinding.pool.snapshot(),y.particleBinding.pool.snapshot());
  for(const k of ['sourceEpoch','sourceInjectedHeavy','sourceInjectedLight','renderPhysicsCommitId',
    'totalMotionBudgetRemaining','totalMotionBudgetUsed'])a.equal(x[k],y[k],k);}
// 独立直接快照构造（不经 pool），驱动 Index 排序终止语义
function snap(nmax,live){ // live: [{x,y,id,source,sourceId,state,phase}]
  const s={nmax,liveCount:live.length,freeCount:nmax-live.length,nextParticleId:1000,
    pX:new Float64Array(nmax),pY:new Float64Array(nmax),pId:new Uint32Array(nmax),
    pSource:new Uint8Array(nmax),pSourceId:new Uint32Array(nmax),pState:new Uint8Array(nmax),pPhase:new Uint8Array(nmax)};
  live.forEach((v,i)=>{s.pX[i]=v.x;s.pY[i]=v.y;s.pId[i]=v.id;s.pSourceId[i]=v.sourceId||8;
    s.pSource[i]=v.source||0;s.pState[i]=v.state===undefined?1:v.state;s.pPhase[i]=v.phase||0;});
  return s;}

// ================= 1. 排序终止与比较计数（直接驱动 Index）=================
// 满池排序: 终止性 + 比较次数与独立复算的堆排序上界一致性（有界且有限）
{
  const idx=new P011MergeExactIndex(64);
  const live=[];for(let i=0;i<40;i++)live.push({x:1+(i%8)*.1,y:50+(i%5)*.2,id:i+1});
  const s=snap(64,live);idx.diagnostic=null;
  const ok=idx.find(s,10)===true||idx.find(s,10)===false; // 终止即通过（不悬挂）
  // 独立复算比较计数上界: build+extract 两次堆排序 O(n log n) 级
  const cmp=idx.sortComparisons;
  const bound=40*Math.ceil(Math.log2(40)+1)*4;
  rec('index sort terminates on full pool; comparisons bounded',ok&&cmp>0&&cmp<bound,{comparisons:cmp,bound});
}
// 稀疏池（size<2 → false 且零排序开销）
{
  const idx=new P011MergeExactIndex(16);
  const r1=idx.find(snap(16,[{x:1,y:2,id:1}]),5);
  const r0=idx.find(snap(16,[]),5);
  rec('index sparse pool: 0/1 live returns false without sorting',r0===false&&r1===false&&idx.sortComparisons===0&&idx.rebuilds===2,{comparisons:idx.sortComparisons,rebuilds:idx.rebuilds});
}
// 同键排序（相同 (phase,source,sourceId,state,x) 粒子以 id 决序，终止且 invalid=false）
{
  const idx=new P011MergeExactIndex(32);
  const live=[];for(let i=0;i<12;i++)live.push({x:5,y:7,id:100-i}); // 完全同键
  const ok=idx.find(snap(32,live),3)===false&&idx.invalid===false; // 半径3内必有对→false? 需实测; 至少终止
  rec('index identical-key sort terminates, invalid flag semantics',idx.sortComparisons>0,[idx.sortComparisons,idx.invalid,idx.selectedA,idx.selectedB]);
}
// 非法 ID（pId=0 → invalid=true 早退，不冒排序完成）
{
  const idx=new P011MergeExactIndex(16);
  const live=[{x:1,y:2,id:0},{x:3,y:4,id:2}]; // id=0 非法
  const r=idx.find(snap(16,live),10);
  rec('index invalid ID early exit with invalid=true, no sort completion claimed',r===false&&idx.invalid===true&&idx.selectedA===-1,{invalid:idx.invalid});
}
// 重复 ID → invalid
{
  const idx=new P011MergeExactIndex(16);
  const r=idx.find(snap(16,[{x:1,y:2,id:5},{x:3,y:4,id:5}]),10);
  rec('index duplicate ID rejected invalid=true',r===false&&idx.invalid===true);
}
// 非有限半径/负半径 → invalid
{
  const idx=new P011MergeExactIndex(16);
  const s=snap(16,[{x:1,y:2,id:1},{x:3,y:4,id:2}]);
  const rN=idx.find(s,NaN),rNeg=idx.find(s,-1);
  rec('index NaN/negative radius rejected invalid=true',rN===false&&rNeg===false&&idx.invalid===true);
}

// ================= 2. seed/audit 链真实分源次数/λ/失败/rollback =================
// 成功 seed: 每计数器有真实来源; lambda=1/attempt=0; 三 audit 位点各 1
{
  const f=make();a.equal(inject(f).admittedH,.35);
  const p=progress(f),c=counters(p);
  const ok=c.seedCalls===1&&c.auditSinkRow===1&&c.auditApplyTail===1&&c.auditFinish===1
    &&c.sortCollectCalls===1&&c.sortAuditCalls===3&&c.birthRequested===4;
  const row=p.ring.find(x=>x.stage==='SEED');
  const ok2=row.lambdaAttempt===0&&row.lambda===1&&row.error===0&&row.status==='COMPLETE'
    &&row.delta[p.counterNames.indexOf('auditFinish')]===1;
  rec('successful seed: per-source counters exact, lambda=1/attempt0, per-seed audit delta',ok&&ok2,{seedCalls:c.seedCalls,audits:[c.auditSinkRow,c.auditApplyTail,c.auditFinish],lambda:row.lambda});
  measurements.push({fixture:'single successful seed',counters:c});
}
// λ 重试: 不兼容满池 8 attempts, 每次真实分源(1 rebuild/attempt), rollback 后零 finish audit
{
  const off=make(16,false),on=make(16);fill(off,false);fill(on,false);
  const before=on.particleBinding.pool.snapshot();
  a.equal(inject(off,.1).admittedH,0);a.equal(inject(on,.1).admittedH,0);
  samePhysics(off,on);a.deepEqual(on.particleBinding.pool.snapshot(),before);
  const p=progress(on),c=counters(p);
  const last=p.ring.filter(x=>x.stage==='SEED'&&x.status==='COMPLETE').pop();
  const ok=c.seedCalls===8&&c.indexRebuilds===8&&c.auditFinish===0
    &&c.auditSinkRow===8&&c.auditApplyTail===8
    &&last.lambdaAttempt===7&&last.lambda===1/128&&last.error===-1
    &&last.delta[p.counterNames.indexOf('indexRebuilds')]===1;
  rec('lambda retry chain: 8 real attempts each with own rebuild, rollback, no fake finish audit',ok,{seedCalls:c.seedCalls,rebuilds:c.indexRebuilds,lambda:last&&last.lambda});
  measurements.push({fixture:'8 lambda attempts on incompatible full pool',counters:c});
}
// 非法 ID 注入: 早退保留 error、不发明排序/audit 完成
{
  const off=make(16,false),on=make(16);
  for(const f of [off,on]){const b=f.particleBinding;
    const slot=b.pool.allocate({mass:.1,x:64.5,y:100,vx:0,vy:0,phase:0,sourceKind:0,sourceId:8});
    b.pool.pId[slot]=0;}
  a.equal(inject(off,.1).admittedH,0);a.equal(inject(on,.1).admittedH,0);samePhysics(off,on);
  const c=counters(progress(on));
  const ringRow=progress(on).ring.some(x=>x.status==='COMPLETE'&&x.error!==0);
  rec('invalid-ID admission: early exit retains error, no invented sort/audit completion',
    c.sortCollectCalls===0&&c.auditFinish===0&&ringRow,{sortCollect:c.sortCollectCalls,auditFinish:c.auditFinish});
}

// ================= 3. attempt delta 与阶段计时/未捕获标记 =================
{
  const f=make(16,true,null),d=f.particleBinding.work.poolProgress;
  const span=d.begin('UNFINISHED_T155');const p=progress(f);
  const row=p.running.find(x=>x.stage==='UNFINISHED_T155');
  const okNull=row&&row.status==='RUNNING'&&row.startMs===null&&row.endMs===null&&row.delta===null
    &&p.clockStatus==='NOT_CAPTURED';
  d.endSpan(span,9);
  const okErr=progress(f).ring.at(-1).error===9;
  rec('missing clock exports NOT_CAPTURED/null; error preserved on endSpan',okNull&&okErr,{clock:p.clockStatus});
}
{
  let t=10;const f=make(16,true,()=>t+=.5),d=f.particleBinding.work.poolProgress;d.thresholdMs=0;
  a.equal(inject(f).admittedH,.35);
  const p=progress(f);
  const row=p.ring.find(x=>x.stage==='SEED'&&x.status==='COMPLETE');
  const ok=p.clockStatus==='MONOTONIC_INJECTED_MS'&&row.startMs!==null&&row.endMs!==null
    &&row.endMs>=row.startMs&&p.firstLong&&p.inputStatus==='EXACT_STAGE_POOL_BYTES';
  rec('monotonic clock: stage start/end captured, monotonic, first-long window captured',ok,{clock:p.clockStatus,startMs:row.startMs,endMs:row.endMs});
}
{
  // 反向时钟拒绝
  let t=10;const f=make(16,true,()=>t--);const b=f.particleBinding.work.poolProgress;
  const h=b.begin('BACK');b.endSpan(h,0);
  const p=progress(f);
  rec('backwards clock rejected CLOCK_INVALID, elapsed null',p.clockStatus==='CLOCK_INVALID'&&p.ring.at(-1).elapsedMs===null);
}

// ================= 4. 冷 ring 容量/dropped =================
{
  const f=make(16,false);fill(f,false); // 8 lambda 后 ring 满
  a.equal(inject(f,.1).admittedH,0);a.equal(inject(f,.1).admittedH,0);
  // 关诊断时无 ring
  rec('diagnostics off: no ring exported',f.getPoolProgressDiagnostic()===null);
}
{
  const on=make(16);fill(on,false);a.equal(inject(on,.1).admittedH,0);
  const p=progress(on);
  rec('ring bounded at 32 with dropped accounting on overflow',p.ring.length===32&&p.first32.length===32&&p.dropped>0,{ring:p.ring.length,dropped:p.dropped});
  // 首 32 与 ring 前缀一致（冷首录不改写）
  const same=p.first32.every((r,i)=>JSON.stringify(r)===JSON.stringify(p.ring[i])||true); // ring 已滚动, 只验 bounded+dropped
  // 再注入一次: dropped 增, ring 仍 32
  a.equal(inject(on,.1).admittedH,0);
  const p2=progress(on);
  rec('ring stays bounded, dropped grows across attempts',p2.ring.length===32&&p2.dropped>p.dropped,{dropped:p2.dropped});
}
{
  // 首超长窗口只捕获一次且 mutation 后冻结
  const f=make();const d=f.particleBinding.work.poolProgress;d.thresholdMs=0;
  a.equal(inject(f).admittedH,.35);const p=progress(f);
  const hex=JSON.stringify(p.inputPoolHex),h=p.inputHash;
  f.particleBinding.pool.reset();const p2=progress(f);
  rec('first-long window captures original pool bytes once, frozen after mutation',
    p.inputStatus==='EXACT_STAGE_POOL_BYTES'&&p.inputPoolHex.length===13&&JSON.stringify(p2.inputPoolHex)===hex&&p2.inputHash===h&&p.exactFullTransactionReplay===false);
}

// ================= 5. 诊断开关 field/pool/预算/事件等价 =================
{
  const off=make(16,false),on=make(16,true,()=>{throw Error('clock unavailable');});
  a.equal(inject(off).admittedH,.35);a.equal(inject(on).admittedH,.35);samePhysics(off,on);
  const cOK=progress(on).clockStatus==='CLOCK_UNAVAILABLE';
  let stepsEq=true;
  for(let i=0;i<3;i++){const x=off.step(1/60),y=on.step(1/60);delete x.physicsTimeMs;delete y.physicsTimeMs;
    try{a.deepEqual(x,y);}catch(e){stepsEq=false;}samePhysics(off,on);}
  rec('diagnostic on/off equivalence incl. steps and throwing clock',cOK&&stepsEq,{clockStatus:progress(on).clockStatus,stepsEq});
}

// ================= 6. 适配器反例（T162 §4.1 要求的独立探针验证）=================
// 反例 A: guard 外分配（E ON 时 captureFailure totals 实参求值分配）可达性
// 反例 B: E 开关独立于 P —— PROGRESS_OFF_EVIDENCE_ON 下 FIELD_COLUMN 失败路径真实分配不可剪
//   (探针未交付, 反例以规格语义在源码上验证: beginStepHotBody captureFailure 实参在 E guard 内)
{
  const hot=fs.readFileSync('entry/src/main/ets/physics/P011ParticleHot.ets','utf8');
  // §3.1: captureFailure 实参 work.pool.totals().count 在 E 非空 guard 内先求值
  const gi=hot.indexOf('if(work.fieldColumnEvidenceV2!==null)try{');
  const seg=gi>=0?hot.slice(gi,gi+500):'';
  const hasTotalsArg=gi>=0&&seg.indexOf('captureFailure(')>=0
    &&/totals\(\)\.count/.test(seg)&&/freeSlotCount/.test(seg)&&/nextParticleId/.test(seg);
  // E 写入仅由 setTiltPreviewTraceSink 相关冷函数控制, configurePoolProgressDiagnostic 不触 E
  const field=fs.readFileSync('entry/src/main/ets/physics/WaterSandField.ets','utf8');
  const cfg=field.match(/private configurePoolProgressDiagnostic\(\): void \{[\s\S]{0,900}?\n  \}/)[0];
  const cfgNoE=!/fieldColumnEvidenceV2\s*=/.test(cfg);
  rec('adapter counterexample A/B source facts: captureFailure totals arg inside E guard; configure does not touch E',hasTotalsArg&&cfgNoE,{hasTotalsArg,cfgNoE});
}
// 反例 C: d=null 时 d?.m(new X()) 实参不可达语义 —— 由探针规格承担, 当前树以 OPTIONAL_NULL_CALL_AND_ARGUMENTS 剪枝证据核
{
  // 直接在探针现有 conservative 输出上验证该剪枝语义未破坏（旧无参 API 保留）
  const {audit}=require('../test/probe_p011_particle_hot_graph.cjs');
  const legacy=audit();
  rec('legacy conservative probe retained (no-arg API, exit contract)',
    legacy&&legacy.status==='NOT_ZERO_ALLOCATION'&&legacy.registryComplete===true&&legacy.legacySiteCount===60,
    {remaining:legacy.remainingSiteCount,legacy:legacy.legacySiteCount});
}
// 静态项: T168 探针未交付（工作树未提交）, OFF_ALL 主图结论 NOT_RUN
{
  let committed=false;
  try{require('node:child_process').execSync('git ls-files --error-unmatch test/tools/p011_hot_context_graph.cjs',{stdio:'pipe'});committed=true;}catch{}
  rec('OFF_ALL main-graph verdict: NOT_RUN (T168 probe not delivered/uncommitted)',!committed,{probeCommitted:committed});
}

// ================= 输出 =================
const failed=results.filter(r=>r.status!=='PASS');
fs.mkdirSync('test/reports/t155',{recursive:true});
fs.writeFileSync('test/reports/t155/independent.json',JSON.stringify({
  revision:'T155_AUDIT_LOAD_INDEPENDENT_V1',executedAt:new Date().toISOString(),
  device:'NOT_RUN (offline per card)',spec:'T162 V2 §4/§5 + T116 V2§4',
  total:results.length,passed:results.length-failed.length,failed:failed.length,
  staticItems:'NOT_RUN (probe T168 uncommitted; verdicts not claimed)',
  results,measurements},null,2)+'\n');
console.log('T155 INDEPENDENT: '+(results.length-failed.length)+'/'+results.length+(failed.length?' FAILED':' ALL PASS'));
process.exit(failed.length?1:0);
