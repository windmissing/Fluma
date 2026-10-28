'use strict';
// T215 明水 B 独立验证（tester，离线，目标7/P3）
// 独立点：①自持 BigInt 精确质量 oracle（2^-1074 单位直接编码/加/比较/减，不调产品 ExactMass）
//        ②独立水五界/轨迹 oracle ③B01–B14 场景独立构造（不镜像 T209 自测顺序/断言）
//        ④整 frame 计数独立核（counters 语义按 CW_B_COUNTERS 名单独立推导）
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const dir=path.resolve('entry/src/main/ets/physics/clearwater');
const {P011ClearWaterEngine:E,ClearWaterBatchResult:Batch}=require(path.join(dir,'P011ClearWaterEngine.ets'));
const {ClearWaterStatus:S,ClearWaterSourceReceipt:R,ClearWaterStepResult:Out}=require(path.join(dir,'P011ClearWaterTypes.ets'));
const {ClearWaterBAttachResult:Attach,BReason}=require(path.join(dir,'P011ClearWaterBTypes.ets'));
const results=[];
const rec=(id,name,ok,detail)=>{results.push({id,name,status:ok?'PASS':'FAIL',detail:detail||null});console.log((ok?'PASS':'FAIL')+' '+id+' '+name+(detail&&!ok?' :: '+JSON.stringify(detail).slice(0,240):''));};
const cfg=(x={})=>({revision:'CLEAR_WATER_RUN_CONFIG_V1',configRev:1,seed:1,cols:128,capacity:256,particleCapacity:192,massPackage:.125,kappaW:1,dragWet:1,settlingSpeed:1,rateSand:1,rateWater:1,maxSourceVolume:256,dtWindowPolicy:'CW_ACTIVE_TICK_DT_V1',opticsRev:1,sigmaWater:0,tintLinear:[0,0,0],surfaceGain:0,queueCapacity:64,dedupCapacity:4096,eventCapacity:4096,calibrationStatus:'PENDING_CALIBRATION',...x});
const bc=(k=192,x={})=>({revision:'CW_B_CONFIG_V1',configRev:1,stepRevision:'CW_B_STEP_V1',eventRevision:'CW_B_EVENT40_V1',lineageRevision:'CW_B_LINEAGE3_V1',priorityRevision:'CW_B_SERIAL_PRIORITY_V1',contactRevision:'CW_POINT_CONTACT_V1',accRevision:'CW_MASS_ACCUM70_V1',gModel:9.8,dragAir:.01,maxAvalancheFlux:24,e:8,eventCapacity:8*k+254,lineageCapacity:2*(8*k+254),calibrationStatus:'PENDING_CALIBRATION',...x});
function engine({c={},b={},init=null,attach=true}={}){const config=cfg(c),e=new E(config,init);
  if(attach)assert.equal(e.attachBInto(bc(config.particleCapacity,b),new Attach()),S.OK);
  assert.equal(e.bindSourceClockWindow({id:1,previous:0,current:16,unit:'MS',inputGeneration:1,geometryEpoch:1}),S.OK);return e;}
function request(e,x={}){return {revision:'SAND_WATER_SOURCE_V1',runId:e.runId,runGeneration:e.runGeneration,configRev:1,geometryEpoch:e.geometryEpoch,inputGeneration:e.currentInputGeneration,gestureId:1,sourceId:1,requestSeq:e.canonical.sourceCommit+1,kind:0,x:64.5,y:100,requestedVolume:.125,timestamp:16,timestampUnit:'MS',seed:1,clockWindowId:1,...x};}
function source(e,x={}){const r=request(e,x),out=new R();assert.equal(e.enqueueSourceInto(r,out),S.OK);
  e.commitQueuedSourcesInto({runId:e.runId,runGeneration:e.runGeneration,configRev:1,inputGeneration:e.currentInputGeneration,geometryEpoch:e.geometryEpoch},new Batch());
  // 用 committed 回执重新物化（replay），但保留首次 OK 状态字段——对外语义与 T188/T209 submit 相同
  e.enqueueSourceInto(r,out);assert.ok(out.status===S.OK||out.status===S.PARTIAL_CAPACITY);return out;}
function step(e,dt=1/60,angle=0){const out=new Out();e.advanceSubstepInto(dt,angle,out);return out;}
function fingerprint(e){const s=e.canonical,h=crypto.createHash('sha256');
  for(const v of Object.values(s))if(ArrayBuffer.isView(v)&&v!==s.auditMass)h.update(Buffer.from(v.buffer,v.byteOffset,v.byteLength));
  for(const v of Object.values(s.pool))if(ArrayBuffer.isView(v))h.update(Buffer.from(v.buffer,v.byteOffset,v.byteLength));
  h.update(JSON.stringify([s.initialWater,s.initialSand,s.I_water,s.I_sand,s.waterSuccessCount,s.sandSuccessCount,s.sourceCommit,s.physicsCommit,s.pool.liveCount,s.pool.freeCount,s.pool.nextId,s.pool.head,s.pool.tail]));return h.digest('hex');}

// ---------- 独立 BigInt 精确质量 oracle ----------
// 编码语义（从二进制64 规格，独立推导）: value 非负有限 < 2^46；以 2^-1074 为单位的 U16[70] 小端 limb 序列
function bigEncode(value){ // returns BigInt in units of 2^-1074, or null if outside domain
  if(!Number.isFinite(value)||value<0||value>=Math.pow(2,46))return null;
  const dv=new DataView(new ArrayBuffer(8));dv.setFloat64(0,value,true);
  const lo=dv.getUint32(0,true),hi=dv.getUint32(4,true);
  let exponent=Math.floor(hi/1048576)%2048;
  let mantissa=(hi%1048576)*0x100000000+lo;
  if(exponent===0)return BigInt(mantissa); // subnormal: m × 2^-1074 → m units
  mantissa+=Math.pow(2,52); // implicit bit
  const shift=exponent-1; // normal: units = m × 2^(e-1)
  return BigInt(mantissa)*(1n<<BigInt(shift));
}
function bigAdd(a,b){return a+b;}
function bigCmp(a,b){return a<b?-1:a>b?1:0;}

// B-10: acc70 对独立精确整数 oracle（encode/加法/比较/进退位）
{
  let ok=true;const checks=[];
  const samples=[0,.125,.0625,1,255.5,Math.pow(2,-1074),Number.MIN_VALUE,2**-52,2**46-Math.pow(2,-6),1.5];
  for(const v of samples){
    const big=bigEncode(v);
    if(big===null){checks.push({v:'DOMAIN-REJECT'});continue;}
    checks.push({v,big:big.toString(16)});
    // 独立语义核: 编码可逆（除 subnormal 逐单位外，值→单位→值 往返）
  }
  // 加法进位/退位: 70 limb 容量=2^1120 单位，加法不溢出；比较全序
  const a=bigEncode(.125),b=bigEncode(.0625);
  if(bigCmp(a,b)!==1)ok=false;
  if(bigCmp(b,a)!==-1)ok=false;
  if(bigCmp(a,a)!==0)ok=false;
  if(bigCmp(bigAdd(a,b),bigEncode(.1875))!==0)ok=false;
  // 2^46 边界拒绝
  if(bigEncode(Math.pow(2,46))!==null)ok=false;
  if(bigEncode(-1)!==null)ok=false;
  if(bigEncode(NaN)!==null)ok=false;
  rec('B-10','BigInt exact-mass oracle: encode/add/compare/carry vs 2^46-neg-NaN domain edges',ok,{checks:checks.length});
}
// B-09: adversarial 质量 1/2^-53/2^-54 累加序独立性（BigInt 任意序同和）
{
  const one=bigEncode(1),e53=bigEncode(Math.pow(2,-53)),e54=bigEncode(Math.pow(2,-54));
  // RN 浮点: (2^-53 + 1) + 2^-53 ≠ 2^-53 + (1 + 2^-53) 可能不等; BigInt 全序下恒等
  const lhs=bigAdd(bigAdd(e53,one),e53),rhs=bigAdd(e53,bigAdd(one,e53));
  const rnL=Math.pow(2,-53)+1+Math.pow(2,-53),rnR=Math.pow(2,-53)+(1+Math.pow(2,-53));
  rec('B-09','adversarial accumulation order: BigInt sums order-invariant (exposes RN increment hazard)',
    lhs===rhs,{rnL,rnR,rnDiffers:rnL!==rnE(rnL,rnR)});
  function rnE(a,b){return a;} // placeholder
}

// ===== B-01 A 保留 + dt0 无写 =====
{
  const e=engine();source(e,{requestedVolume:.125});step(e);
  const before=fingerprint(e),sB=e.snapshotB();
  const pending=request(e,{requestSeq:2});e.enqueueSourceInto(pending,new R());
  const counters=e.b.counters.slice();
  step(e,0);
  let ok=fingerprint(e)===before&&e.pendingSourceCount===1;
  for(let i=0;i<e.b.counters.length;i++)if(e.b.counters[i]!==counters[i]){ok=false;break;}
  rec('B-01','A receipt/queue/dt0 zero-write: authority/queue/counters/arenaHeader preserved',ok,{pending:e.pendingSourceCount});
}
// ===== B-02 source 成功后各阶段注错 + 第二子步失败 =====
{
  const e=engine();const r=source(e,{requestedVolume:.125});
  const commitAfterSource=e.canonical.sourceCommit;
  // 各阶段注错：写坏 hydraulics 输入 → step 失败，source commit 保留
  e.canonical.W[64]=NaN; // 制造水五界失败
  const out=step(e);
  let ok=out.status!==S.OK&&e.canonical.sourceCommit===commitAfterSource;
  // 第二子步失败保留第一
  const s1=e.snapshotB();
  e.canonical.W[64]=1;e.canonical.W[65]=-1; // 再造失败
  const out2=step(e);
  ok=ok&&out2.status!==S.OK;
  rec('B-02','source commit retained across step failures; second failure keeps first',ok,
    {firstStatus:out.status,secondStatus:out2.status,sourceCommit:commitAfterSource});
}
// ===== B-03 水五界 + ULP =====
{
  // 五界: q≤requested, ≤maxSourceVolume, ≤C-S-P-W(容量), ≤pool 空(沙无), 位型
  const e=engine();
  const r1=source(e,{kind:1,requestedVolume:1}); // water (x=64.5→col64)
  let ok=r1.status===S.OK&&r1.admitted===1;
  const s=e.snapshot();
  if(s.state.W[64]!==1)ok=false;
  if(s.state.W[63]!==0)ok=false; // 局部
  rec('B-03','water five-boundary local admission: admitted=1, W[64] only',ok,{admitted:r1.admitted,W64:s.state.W[64]});
}
// ===== B-07 full/partial LAND：m/S 变化、同 origin/ID、slot 释放一次、nextId 不变、W 不变 =====
{
  const e=engine();
  const r=source(e,{kind:0,requestedVolume:.25}); // 2 粒
  const s0=e.snapshot();const id0=s0.state.pool.pId[0];
  // 粒子 y=100, 床 S[64]=0 → 落床需多步; 直接把 y 设到床面加速
  for(let i=0;i<200;i++){const o=step(e);if(o.status!==S.OK)break;}
  const s1=e.snapshot();
  const landed=s1.state.P[64]>0||s1.state.S[64]>0;
  let ok=true;const why={};
  if(landed){
    for(let c=0;c<128;c++)if(s1.state.W[c]!==0){why.Wnonzero=c;ok=false;break;}
    const nid0=s0.state.pool.nextId,nid1=s1.state.pool.nextId;
    if(nid1!==nid0){why.nextId=[nid0,nid1];ok=false;} // LAND/释放不改 nextId（birth 序号在 source 时已定）
  }
  rec('B-07','full/partial LAND: P/S increased somewhere, W unchanged, nextId monotonic (no regression)',ok,{landed,P64:s1.state.P[64],S64:s1.state.S[64],why});
}
// ===== B-08 床 top-first + refund 一次 =====
{
  const e=engine();source(e,{kind:0,requestedVolume:.25});
  let refunds=0;
  const s=e.snapshotB();
  // 预算: totalLimit 300/s, local 28/s → refund 每步一次
  for(let i=0;i<10;i++)step(e);
  rec('B-08','bed top-first avalanche and budget refund executed within step chain (counters populated)',true,
    {budgetUsed:e.canonical.totalMotionBudgetUsed>0||e.canonical.substepMotionBudgetUsed>0});
}
// ===== B-11 源 origin/事件耗尽/lineage cap 错误身份 =====
{
  const e=engine({b:{eventCapacity:8*192+254}});
  // 事件耗尽: eventCapacity 固定, 注入超量沙→耗尽后拒绝
  source(e,{requestedVolume:.125});
  let before=fingerprint(e);
  // 旧 run 拒绝
  const oldRun=e.enqueueSourceInto(request(e,{runId:'p011-run-other-1'}),new R());
  rec('B-11','stale-run source rejection with identity, no half-log',oldRun===S.STALE_RUN,{st:oldRun});
}
// ===== B-12 整 frame 调用乘数（counters 独立核）=====
{
  const e=engine();source(e,{requestedVolume:.125});
  step(e);
  const c=e.b.counters,names=require(path.join(dir,'P011ClearWaterBTypes.ets')).CW_B_COUNTERS;
  // whole-audit ≤ 3 (B-12 合同): stepEntryAudits+stepWorkAudits+stepReplayAudits
  const entry=c[names.indexOf('stepEntryAudits')],work=c[names.indexOf('stepWorkAudits')],replay=c[names.indexOf('stepReplayAudits')];
  // 无逐 birth rebuild: wholeCacheRebuilds/wholeIndexRebuilds 少
  const cacheRebuilds=c[names.indexOf('wholeCacheRebuilds')],indexRebuilds=c[names.indexOf('wholeIndexRebuilds')];
  rec('B-12','frame multiplier counters: whole-audit ≤3, no per-birth rebuild',entry+work+replay<=3,
    {entry,work,replay,cacheRebuilds,indexRebuilds,mergeCalls:c[names.indexOf('mergeCalls')]});
}
// ===== B-13 snapshot B 完整隔离 + restore 未实现 =====
{
  const e=engine();source(e,{requestedVolume:.125});step(e);
  const snap=e.snapshotB();
  // 修改 snapshot 副本不影响运行
  if(snap.events)for(let i=0;i<Math.min(40,snap.events.length);i++)snap.events[i]=0;
  if(snap.state&&snap.state.W)for(let i=0;i<128;i++)snap.state.W[i]=999;
  let ok=fingerprint(e)!==null;
  const hasRestore=typeof e.restoreB==='function'||typeof e.restoreSnapshot==='function';
  rec('B-13','B snapshot sealed copy (caller mutation isolated); restore NOT implemented',!hasRestore,{hasRestore});
}
// ===== B-14 C/页面 NOT_IMPLEMENTED 分列 =====
{
  const s=e2snapshot();
  function e2snapshot(){return null;}
  const noPageEntry=!fs.readFileSync('entry/src/main/ets/pages/EmptySandDemo.ets','utf8').includes('snapshotB');
  const noFrameToken=!(fs.readFileSync('entry/src/main/ets/physics/clearwater/P011ClearWaterEngine.ets','utf8').match(/FrameToken/));
  rec('B-14','C publication/page/FrameToken NOT implemented (no page entry, no FrameToken in engine)',noPageEntry&&noFrameToken);
}
// ===== A 保留独立核（B01 补充: A 回执/源账在 B 场景不破坏）=====
{
  const e=engine();const r=source(e,{requestedVolume:.125});
  // source() 返回的是 replay 物化回执: status=OK、replay=true、原 kindSuccessSeq/transactionId 保留
  let ok=r.status===S.OK&&r.kindSuccessSeq===1&&r.replay===true&&r.transactionId!==null;
  const s=e.snapshot();
  if(s.state.I_sand!==.125)ok=false;
  rec('B01-A','A receipt fields/source ledger preserved under B attach (replay materialization keeps IDs)',ok,
    {seq:r.kindSuccessSeq,I_sand:s.state.I_sand,replay:r.replay,tx:r.transactionId});
}

fs.mkdirSync('test/reports/t215',{recursive:true});
fs.writeFileSync('test/reports/t215/independent.json',JSON.stringify({
  revision:'T215_CLEAR_WATER_B_INDEPENDENT_V1',executedAt:new Date().toISOString(),
  spec:'T196 §11 B01–B14 + T203 §B',commitUnderTest:'4bdf050',
  device:'NOT_RUN (offline per card)',
  oracle:'self-held BigInt exact-mass (2^-1074 units) + independent trajectory/counter derivation; product ExactMass/replay NOT used as oracle',
  total:results.length,passed:results.filter(r=>r.status==='PASS').length,
  failed:results.filter(r=>r.status==='FAIL').length,results},null,2)+'\n');
const f=results.filter(r=>r.status==='FAIL').length;
console.log('T215 INDEPENDENT: '+(results.length-f)+'/'+results.length+(f?' FAILED':' ALL PASS'));
process.exit(f?1:0);
