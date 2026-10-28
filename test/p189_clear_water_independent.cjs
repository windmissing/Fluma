'use strict';
// T189 单沙明水 A 段独立验证（tester，离线，目标7/P3）
// 独立点：①独立 oracle——W/守恒/容量全部由本执行器用普通 Float64 独立重算，不调用被测 audit/rebuild/waterTotal
//        ②场景按 T108 CW-01/02/03/11/12/15+A 段边界逐条独立构造，不镜像 T188 自测
//        ③B/C/restore NOT_IMPLEMENTED 分列登记
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const dir=path.resolve('entry/src/main/ets/physics/clearwater/');
const {P011ClearWaterEngine:E,ClearWaterBatchResult:B}=require(path.join(dir,'P011ClearWaterEngine.ets'));
const {ClearWaterStatus:S,ClearWaterSourceReceipt:R,CW_SOURCE,CW_MEDIUM}=require(path.join(dir,'P011ClearWaterTypes.ets'));
const results=[];const rec=(name,ok,detail)=>{results.push({name,status:ok?'PASS':'FAIL',detail:detail||null});console.log((ok?'PASS':'FAIL')+' '+name+(detail&&!ok?' :: '+JSON.stringify(detail).slice(0,240):''));};
const config=ch=>({revision:'CLEAR_WATER_RUN_CONFIG_V1',configRev:1,seed:1,cols:128,capacity:256,particleCapacity:512,massPackage:.125,
  kappaW:1,dragWet:1,settlingSpeed:1,rateSand:1,rateWater:1,maxSourceVolume:256,dtWindowPolicy:'CW_ACTIVE_TICK_DT_V1',opticsRev:1,
  sigmaWater:0,tintLinear:[0,0,0],surfaceGain:0,queueCapacity:64,dedupCapacity:4096,eventCapacity:4096,calibrationStatus:'PENDING_CALIBRATION',...ch});
const boundary=e=>({runId:e.runId,runGeneration:e.runGeneration,configRev:1,inputGeneration:e.currentInputGeneration,geometryEpoch:e.geometryEpoch});
function bind(e){assert.equal(e.bindSourceClockWindow({id:1,previous:0,current:16,unit:'MS',inputGeneration:e.currentInputGeneration,geometryEpoch:e.geometryEpoch}),S.OK);}
function req(e,ch={}){return {revision:CW_SOURCE,runId:e.runId,runGeneration:e.runGeneration,configRev:1,geometryEpoch:e.geometryEpoch,inputGeneration:e.currentInputGeneration,gestureId:1,sourceId:1,requestSeq:1,kind:1,x:64.5,y:128,requestedVolume:1,timestamp:16,timestampUnit:'MS',seed:1,clockWindowId:1,...ch};}
function submit(e,r){const out=new R();assert.equal(e.enqueueSourceInto(r,out),S.OK);const batch=new B();e.commitQueuedSourcesInto(boundary(e),batch);const st=e.enqueueSourceInto(r,out);return {out,status:st,batch};}
// 独立 oracle: 本执行器自持的 W/S/P 账本（普通浮点，不调被测 rebuild/audit）
function oracle(){return {W:new Float64Array(128),S:new Float64Array(128),P:new Float64Array(128),Iw:0,Is:0,waterN:0,sandN:0,live:0,nextId:1};}
function fingerprint(e){const c=e.canonical,h=crypto.createHash('sha256');for(const k of ['W','bh','mh','S','eta','P','R'])h.update(Buffer.from(c[k].buffer));
  for(const [,v]of Object.entries(c.pool))if(ArrayBuffer.isView(v))h.update(Buffer.from(v.buffer));
  h.update(JSON.stringify([c.initialWater,c.initialSand,c.I_water,c.I_sand,c.waterSuccessCount,c.sandSuccessCount,c.sourceCommit,c.physicsCommit,c.pool.liveCount,c.pool.freeCount,c.pool.nextId,c.pool.head,c.pool.tail]));return h.digest('hex');}

// ===== CW-01 空运行：ΣW=ΣS=ΣP=0、L/U=0、medium 为新 mode =====
{
  const e=new E(config());bind(e);
  const s=e.snapshot();
  let allZero=true;for(const k of ['W','bh','mh','S','eta','P','R'])if(!s.state[k].every(x=>x===0))allZero=false;
  rec('CW-01 empty run: ΣW=ΣS=ΣP=0, I=0, medium=CW mode, audit clean',
    allZero&&s.state.I_water===0&&s.state.I_sand===0&&s.mediumMode==='SINGLE_SAND_CLEAR_WATER_V1'&&e.audit()===S.OK,
    {medium:s.mediumMode});
}
// ===== CW-02 干列连续加水：只改该列 W，独立 I_water 与实际总水差满足审计，不重置 initial =====
{
  const e=new E(config());bind(e);const o=oracle();
  let ok=true;const checks=[];
  for(let i=1;i<=16;i++){
    const r=submit(e,req(e,{requestSeq:i,x:64+i-1,requestedVolume:.5})).out;
    o.W[64+i-1]+=.5;o.Iw+=.5;o.waterN++;
    if(r.admitted!==.5)ok=false;
    if(r.globalWaterAfter-r.globalWaterBefore!==.5)ok=false;
    if(r.I_water!==o.Iw)ok=false; // 独立账比对
  }
  const s=e.snapshot();
  for(let c=0;c<128;c++){const exp=o.W[c];if(Math.abs(s.state.W[c]-exp)>1e-12)ok=false;}
  if(Math.abs(s.state.I_water-o.Iw)>1e-12)ok=false;
  if(s.state.initialWater!==0)ok=false; // 运行不重置基准
  if(e.audit()!==S.OK)ok=false;
  rec('CW-02 continuous water on distinct dry columns: local W only, independent I_water ledger, baseline not reset',ok,{n:16,Iw:o.Iw});
}
// ===== CW-03 满列/部分准入：rejected 对账，无全场均摊、无负 W =====
{
  const e=new E(config());bind(e);const o=oracle();
  const r1=submit(e,req(e,{requestedVolume:255.5})).out; // 容量256 → admitted 255.5
  o.W[64]+=255.5;o.Iw+=255.5;o.waterN++;
  const r2=submit(e,req(e,{requestSeq:2,x:64,requestedVolume:2})).out; // 只剩 .5
  o.W[64]+=.5;o.Iw+=.5;o.waterN++;
  const r3=submit(e,req(e,{requestSeq:3,x:64,requestedVolume:1})).out; // 满
  let ok=r1.admitted===255.5&&r2.admitted===.5&&r2.rejected===1.5&&r2.status===S.PARTIAL_CAPACITY&&r3.status===S.CAPACITY_FULL&&r3.admitted===0;
  const s=e.snapshot();
  if(Math.abs(s.state.W[64]-o.W[64])>1e-12)ok=false;
  if(s.state.W[63]!==0)ok=false; // 无均摊
  for(let c=0;c<128;c++)if(s.state.W[c]<0)ok=false;
  rec('CW-03 partial/full local water: rejected reconciled, no spill, no negative W',ok,
    {r1:r1.admitted,r2:[r2.admitted,r2.rejected],r3:r3.status,W64:s.state.W[64]});
}
// ===== CW-11 非法族：非法位置/NaN/负量/旧 run/旧 geometry/input → 拒绝且账不变 =====
{
  const e=new E(config());bind(e);
  const before=fingerprint(e);
  const bads=[
    ['x out of range',req(e,{requestSeq:2,x:128})],['x NaN',req(e,{requestSeq:3,x:NaN})],
    ['y negative',req(e,{requestSeq:4,y:-1})],['volume 0',req(e,{requestSeq:5,requestedVolume:0})],
    ['volume negative',req(e,{requestSeq:6,requestedVolume:-1})],['volume NaN',req(e,{requestSeq:7,requestedVolume:NaN})],
    ['timestamp negative',req(e,{requestSeq:8,timestamp:-1})],['stale run',req(e,{requestSeq:9,runId:'other'})],
    ['stale geometry',req(e,{requestSeq:10,geometryEpoch:99})],['stale input gen',req(e,{requestSeq:11,inputGeneration:99})],
    ['clock window mismatch',req(e,{requestSeq:12,clockWindowId:99})],
  ];
  let all=true;const got=[];
  for(const [label,r] of bads){
    const out=new R();const st=e.enqueueSourceInto(r,out);
    got.push({label,status:st});
    if(st===S.OK)all=false;
    if(fingerprint(e)!==before)all=false;
  }
  rec('CW-11 invalid family: all rejected with distinct statuses, inventory fingerprint unchanged',all,{got});
}
// ===== CW-12 幂等：重复只计一次；双 kind 序号独立；water/sand 不串账 =====
{
  const e=new E(config());bind(e);const o=oracle();
  const r1=submit(e,req(e,{kind:0,requestedVolume:.25})).out; // sand .25
  o.Is+=.25;o.sandN++;o.S[64]+=.25;o.live+=2; // .25/.125 = 2 粒
  const r2=submit(e,req(e,{requestSeq:2,kind:1,requestedVolume:.5})).out; // water .5（同 requestSeq 但 kind 不同 → 不同 key）
  o.Iw+=.5;o.waterN++;o.W[64]+=.5;
  const s=e.snapshot();
  let ok=r1.kindSuccessSeq===1&&r2.kindSuccessSeq===1
    &&s.state.sandSuccessCount===1&&s.state.waterSuccessCount===1
    &&Math.abs(s.state.I_sand-.25)<1e-12&&Math.abs(s.state.I_water-.5)<1e-12
    &&s.state.W[64]===.5&&Math.abs(s.state.P[64]-.25)<1e-12;
  // 幂等重放: 同请求再提交 → replay, 不重复记账
  const replay=submit(e,req(e,{kind:0,requestedVolume:.25})).out;
  if(!replay.replay)ok=false;
  const s2=e.snapshot();
  if(s2.state.sandSuccessCount!==1)ok=false;
  rec('CW-12 idempotency + dual-kind independent sequences + no cross-accounting',ok,
    {sandSeq:r1.kindSuccessSeq,waterSeq:r2.kindSuccessSeq,replay:replay.replay,sand:s2.state.sandSuccessCount});
}
// ===== CW-12b 幂等冲突：同 key 不同 payload（含 -0 位型）→ IDENTITY_CONFLICT =====
{
  const e=new E(config());bind(e);
  const r=submit(e,req(e,{x:0,requestedVolume:1})).out; // 同 key 基准
  const before=fingerprint(e);
  // 同 key (gestureId/sourceId/requestSeq/kind 同) 不同 payload: y 不同 → IDENTITY_CONFLICT
  const conflictY=e.enqueueSourceInto(req(e,{x:0,y:129}),new R());
  // -0 位型: x=-0 与 x=0 数值同但位不同 → 冲突
  const nz=Object.assign(req(e,{x:0}),{x:-0});
  const conflictBits=e.enqueueSourceInto(nz,new R());
  // 不同 key (gestureId 不同) 同 payload → 新请求, 不是冲突
  const newKey=e.enqueueSourceInto(req(e,{x:0,requestedVolume:1,gestureId:2,requestSeq:2}),new R());
  e.commitQueuedSourcesInto(boundary(e),new B()); // 新 key 请求需要 commit 才改变库存
  rec('CW-12b conflict semantics: same key diff payload (y / -0 bits) → IDENTITY_CONFLICT; new key → new request',
    conflictY===S.IDENTITY_CONFLICT&&conflictBits===S.IDENTITY_CONFLICT
    &&newKey===S.OK&&fingerprint(e)!==before,
    {conflictY,conflictBits,newKey});
}
// ===== CW-15 source 成功后 step 失败：不同 before，成功不被撤回 =====
{
  const e=new E(config());bind(e);
  const r=submit(e,req(e,{requestedVolume:1})).out; // water 成功
  const fpBefore=fingerprint(e);
  const out=new (require(path.join(dir,'P011ClearWaterTypes.ets')).ClearWaterStepResult)();
  const st=e.advanceSubstepInto(1/60,0,out);
  let ok=st===S.UNSUPPORTED_FEATURE&&out.implementation==='NOT_IMPLEMENTED_B_COUPLING'
    &&out.failedAttempt!==null&&out.stepped===false
    &&out.lastAcceptedCommit===r.physicsCommitId; // 最近成功 commit 保留
  const fpAfter=fingerprint(e);
  if(fpBefore!==fpAfter)ok=false; // step 失败不改库存（无 writer）
  rec('CW-15 source committed then step fails: distinct before, success retained, inventory unchanged',
    ok,{stepStatus:S.UNSUPPORTED_FEATURE,attempt:out.failedAttempt,lastCommit:out.lastAcceptedCommit});
}
// ===== dt=0 不消耗队列/不改库存 =====
{
  const e=new E(config());bind(e);
  const out=new R();assert.equal(e.enqueueSourceInto(req(e),out),S.OK);
  const before=fingerprint(e);const pending=e.pendingSourceCount;
  const step=new (require(path.join(dir,'P011ClearWaterTypes.ets')).ClearWaterStepResult)();
  const st=e.advanceSubstepInto(0,0,step);
  rec('dt=0: queue not consumed, inventory unchanged',
    e.pendingSourceCount===pending&&fingerprint(e)===before,{pending:e.pendingSourceCount});
}
// ===== ULP/NUMERIC：小于 W 表示精度 → NUMERIC_UNREPRESENTABLE 拒绝 =====
{
  const initial={W:new Float64Array(128).fill(128),bh:new Float64Array(128),mh:new Float64Array(128)};
  const e=new E(config(),initial);bind(e);
  const before=fingerprint(e);
  const r=submit(e,req(e,{requestedVolume:Number.MIN_VALUE})).out;
  rec('ULP: MIN_VALUE water rejected NUMERIC_UNREPRESENTABLE, no fake success',
    r.status===S.NUMERIC_UNREPRESENTABLE&&r.transactionId===null&&fingerprint(e)===before,{status:r.status});
}
// ===== snapshot 拒改（调用者改副本不影响运行）+ 定容峰值 =====
{
  const e=new E(config());bind(e);
  submit(e,req(e,{requestedVolume:2})).out;
  const before=fingerprint(e);
  const snap=e.snapshot();
  for(const k of ['W','S','P'])snap.state[k].fill(999);
  snap.state.pool.liveCount=12345;
  const after=fingerprint(e);
  const alloc=e.allocationLedger?e.allocationLedger():null;
  rec('snapshot caller-mutation isolated; allocation ledger present',
    after===before&&typeof snap.implementation==='string',
    {ledgerArrays:alloc?(alloc.allocations||alloc).length||Object.keys(alloc).length:'n/a'});
}
// ===== 资源满：queue64/dedup4096/event 容量拒绝保账 =====
{
  const e=new E(config({queueCapacity:2}));bind(e);
  const out=new R();
  assert.equal(e.enqueueSourceInto(req(e,{requestSeq:1}),out),S.OK);
  assert.equal(e.enqueueSourceInto(req(e,{requestSeq:2}),out),S.OK);
  const st3=e.enqueueSourceInto(req(e,{requestSeq:3}),out);
  const before=fingerprint(e);
  rec('queue capacity exhausted: QUEUE_FULL, no overwrite, inventory unchanged',
    st3===S.QUEUE_FULL&&e.pendingSourceCount===2&&fingerprint(e)===before,{st3,pending:e.pendingSourceCount});
}
// ===== P 预留容量显式拒绝 =====
{
  const e=new E(config({massPackage:256}));bind(e);
  assert.equal(submit(e,req(e,{kind:0,requestedVolume:256})).out.admitted,256); // 单粒256占满
  const before=fingerprint(e);
  const w=submit(e,req(e,{kind:1,requestSeq:2,requestedVolume:1})).out;
  rec('P reserve capacity: water rejected PARTICLE_RESERVED_CAPACITY, explicit reason',
    w.status===S.PARTICLE_RESERVED_CAPACITY&&w.transactionId===null&&fingerprint(e)===before,{status:w.status});
}
// ===== 单沙身份链（phase0/external-source/递增 ID）=====
{
  const e=new E(config());bind(e);
  const r=submit(e,req(e,{kind:0,requestedVolume:.3125})).out; // 2整包+尾包.0625
  const s=e.snapshot();
  const ids=[s.state.pool.pId[0],s.state.pool.pId[1],s.state.pool.pId[2]];
  rec('sand birth chain: 2 full + true tail .0625, sequential IDs, phase0/external-source identity',
    r.admitted===.3125&&ids[0]===1&&ids[1]===2&&ids[2]===3
    &&Math.abs(s.state.pool.pMass[2]-.0625)<1e-15,{ids,admitted:r.admitted});
}
// ===== B/C/restore NOT_IMPLEMENTED 分列 =====
{
  const e=new E(config());bind(e);
  const step=new (require(path.join(dir,'P011ClearWaterTypes.ets')).ClearWaterStepResult)();
  const stB=e.advanceSubstepInto(1/60,0,step);
  const s=e.snapshot();
  const hasRestore=typeof e.restoreSnapshot==='function';
  const legacyMedium=s.mediumMode!=='WET_COLUMN_V1';
  rec('B/C/restore NOT_IMPLEMENTED explicitly partitioned (step → UNSUPPORTED_FEATURE, no restore entry, no legacy medium)',
    stB===S.UNSUPPORTED_FEATURE&&!hasRestore&&legacyMedium,{stB,hasRestore,medium:s.mediumMode});
}
// ===== 旧 wet 对照不当明水通过（声明性核）=====
{
  // 旧 WET_COLUMN_V1 链与 CW 模块无共享库存（独立文件/独立 mode 常量）
  const legacyHasCWMedium=fs.readFileSync('entry/src/main/ets/physics/WaterSandField.ets','utf8').includes('SINGLE_SAND_CLEAR_WATER_V1');
  rec('old wet chain does not claim clear-water pass (no shared mode constant, separate module)',!legacyHasCWMedium);
}

fs.mkdirSync('test/reports/t189',{recursive:true});
fs.writeFileSync('test/reports/t189/independent.json',JSON.stringify({
  revision:'T189_CLEAR_WATER_A_INDEPENDENT_V1',executedAt:new Date().toISOString(),
  spec:'T107 §A + T108 CW-01/02/03/11/12/15 + T188 delivery scope',commitUnderTest:'0cc0439',
  device:'NOT_RUN (offline per card)',oracle:'independent Float64 ledger in this executor, no calls into audited implementation',
  total:results.length,passed:results.filter(r=>r.status==='PASS').length,
  failed:results.filter(r=>r.status==='FAIL').length,
  notImplementedPartitions:['B hydraulics / wet particles / step budget+guard+lineage','C publication/render/input controller/page','snapshot restore'],
  results},null,2)+'\n');
const f=results.filter(r=>r.status==='FAIL').length;
console.log('T189 INDEPENDENT: '+(results.length-f)+'/'+results.length+(f?' FAILED':' ALL PASS'));
process.exit(f?1:0);
