'use strict';
// T167 FIELD_COLUMN 修复独立测试（tester，独立于 T166 自测）
// 消费: docs/developer-T166-field-column-residual-v1.md, T146 §5 F1, commit 1f8e16c
// 独立点: 边界 oracle 用 DataView 位操作自算 nextafter；残差值独立构造（非仅 C1/C2 冻结值）；
//         拒绝分类由独立谓词函数驱动； Hot/audit 双通道逐值核对。
const fs=require('node:fs'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');

const EPS=1e-9;
const results=[];
function rec(name,status,detail){results.push({name,status,detail:detail||null});console.log(status+' '+name+(detail?(' :: '+JSON.stringify(detail)):''));}

// ---------- 独立 oracle: nextafter via raw bits ----------
const dv=new DataView(new ArrayBuffer(8));
function bitsOf(n){dv.setFloat64(0,n);return dv.getBigUint64(0);}
function fromBits(b){dv.setBigUint64(0,b);return dv.getFloat64(0);}
function nextAfter(x,direction){ // direction>0 → toward +Inf, <0 → toward -Inf
  const b=bitsOf(x);
  if(x===0)return fromBits(direction>0?1n:(1n<<63n)|1n);
  const positive=(b>>63n)===0n;
  const goUp=positive===(direction>0);
  const nb=goUp?b+1n:b-1n;
  return fromBits(nb);
}
const EPS_DOWN=nextAfter(-EPS,-Infinity);  // 恰在 -EPS 之下第一个可表示值 → 必拒
const EPS_UP=nextAfter(-EPS,Infinity);     // (-EPS,0) 内部 → 必受
// 独立分类谓词: 仅依据已批准语义（waterTop<-EPS 拒, 非有限拒, 其余受）
function expectWaterTop(v){
  if(!Number.isFinite(v))return 'REJECT';
  if(v<-EPS)return 'REJECT';
  return 'ACCEPT';
}
// 六分量独立谓词: 严格 <0 拒（audit 的 -EPS 容差仅属于 audit 链，Hot 仍严格）
function expectOther(v){return (v<0)?'REJECT':'ACCEPT';}

function bitsHex(n){return bitsOf(n).toString(16).padStart(16,'0');}

// ---------- fixtures ----------
function dry(withSink=true){
  const f=new WaterSandField(128,256);
  assert.ok(f.initializeDryParticleDemo({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,particleParams:defaultP011ParticleParams()}).ok);
  if(withSink)f.setTiltPreviewTraceSink(()=>{});
  return f;
}
function begin(f){return f.particleBinding.beginCollect(1/60,0,1,1,f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch);}
function snapshot(f){
  const keys=['waterTop','bh','bl','mh','ml','suspH','suspL','sandTopScratch'];
  return {
    field:keys.map(k=>Buffer.from(f[k].buffer,f[k].byteOffset,f[k].byteLength).toString('hex')),
    pool:f.particleBinding.pool.snapshot(),
    src:[f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch],
    budget:[f.totalMotionBudgetRemaining,f.totalMotionBudgetUsed,[...f.substepMotionBudgetRemaining],[...f.substepMotionBudgetUsed]],
    commit:f.renderPhysicsCommitId};
}

// ---------- 1. C1/C2 冻结残差 + 独立派生残差 ----------
function residualCase(arm,masses,x){
  let cache=0;for(const m of masses)cache+=m;
  for(const m of masses)cache-=m; // 模拟 credit/debit 顺序记账
  const f=dry(true),p=f.particleBinding.pool;
  for(const m of masses)assert.ok(p.allocate({mass:m,x:49.5,y:100,vx:0,vy:0,phase:0,sourceKind:0,sourceId:51})>=0);
  p.colParticleMassH[48]=cache;f.waterTop[48]=p.particleMassAt(48);f.waterTop[49]=p.particleMassAt(49);
  f.sourceInjectedHeavy=x;f.sourceEpoch=1;
  const before=snapshot(f);
  const auditNull=(f.audit(true)===null);
  const hotCode=begin(f);
  const hotOk=(hotCode===0&&f.particleBinding.work.beginFailurePredicate==='');
  const residueKept=(bitsHex(f.waterTop[48])===bitsHex(cache)&&bitsHex(p.colParticleMassH[48])===bitsHex(cache));
  const noEvidence=(f.particleBinding.work.fieldColumnEvidenceV2.failure===null);
  f.particleBinding.rollback();
  const rb=(hotCode===0)?deepEq(snapshot(f),before):null;
  return {arm,x,masses,residual:cache,residualBits:bitsHex(cache),auditNull,hotOk,residueKept,noEvidence,rollbackIdentity:rb,
    expect:expectWaterTop(cache)};
}
function deepEq(a,b){return JSON.stringify(a)===JSON.stringify(b);}
for(const [arm,x,masses] of [
  ['C1',0.44466802978515624,[0.1,0.1,0.1,0.1,0.04466802978515624]],
  ['C2',0.4449276657104492,[0.1,0.1,0.1,0.1,0.0449276657104492]],
  // 独立派生: 不同分母与合成 x, 验证残差语义一般性
  ['D1',0.6449276657104492,[0.1,0.1,0.1,0.3449276657104492]],
  ['D2',0.34466802978515624,[0.1,0.1,0.14466802978515624]]]){
  const r=residualCase(arm,masses,x);
  const ok=r.expect==='ACCEPT'&&r.auditNull&&r.hotOk&&r.residueKept&&r.noEvidence&&(r.rollbackIdentity===true);
  rec('residual '+arm+' (residual='+r.residual+', bits='+r.residualBits+')',ok?'PASS':'FAIL',r);
}

// ---------- 2. waterTop 拒绝边界逐值（独立 nextafter oracle, Hot+audit 双通道）----------
const boundaryValues=[
  ['exact -EPS',-EPS],['interior (EPS..0)',EPS_UP],['nextBelow -EPS',EPS_DOWN],
  ['mid -5e-10',-5e-10],['-2^-55',-(2**-55)],['-MIN_VALUE',-Number.MIN_VALUE],
  ['-0',-0],['+0',0],
  ['NaN',NaN],['+Inf',Infinity],['-Inf',-Infinity]];
for(const [label,v] of boundaryValues){
  const exp=expectWaterTop(v);
  const f=dry(true);f.waterTop[48]=v;const before=snapshot(f);
  const aNull=(f.audit(true)===null);
  const code=begin(f);
  let ok;
  if(exp==='ACCEPT'){
    ok=aNull&&code===0&&bitsHex(f.waterTop[48])===bitsHex(v)&&f.particleBinding.work.fieldColumnEvidenceV2.failure===null;
    if(code===0){f.particleBinding.rollback();ok=ok&&deepEq(snapshot(f),before);}
  }else{
    const ev=f.particleBinding.work.fieldColumnEvidenceV2.failure;
    ok=code===3&&f.particleBinding.collectFailure.predicate==='FIELD_COLUMN'
      &&ev&&ev.column===48&&ev.rawBits[0]===bitsHex(v)&&(ev.badMask&(Number.isFinite(v)?(1<<8):(1<<0)))!==0
      &&deepEq(snapshot(f),before);
  }
  rec('waterTop boundary '+label+' v='+v+' expect='+exp,ok?'PASS':'FAIL',{audit:aNull,hot:code});
}

// ---------- 3. 六分量严格拒绝（含 audit 容差区间内的 -5e-10: Hot 仍拒）----------
// 夹具说明: dry 演示 mediumMode 要求 suspH/suspL 严格 0，负值会先触发 DRY 分支，
// 不适用 audit[-EPS,0) 容差语义；六分量统一用 wet 夹具（同 T149 容差用例构造）。
function wet(){
  const f=new WaterSandField(128,256);
  assert.equal(f.setParticleRoute(defaultP011ParticleParams()).ok,true);
  f.setTiltPreviewTraceSink(()=>{});
  return f;
}
for(const key of ['bh','bl','mh','ml','suspH','suspL']){
  const idx={'bh':1,'bl':2,'mh':3,'ml':4,'suspH':5,'suspL':6}[key];
  for(const v of [-Number.MIN_VALUE,-5e-10,-1e-300]){
    const f=wet();f[key][4]=v;const before=snapshot(f);
    const aTol=(f.audit(false)===null); // audit 链对 [-EPS,0) 容忍（合同: 审计-热不对称）
    const code=f.particleBinding.beginCollect(1/60,0,1,1,0,0,0);
    const ev=f.particleBinding.work.fieldColumnEvidenceV2.failure;
    const ok=aTol&&code===3&&f.particleBinding.collectFailure.failureIndex===4
      &&ev&&ev.column===4&&ev.rawBits[idx]===bitsHex(v)&&(ev.badMask&(1<<(8+idx)))!==0
      &&deepEq(snapshot(f),before);
    rec('six-component '+key+' v='+v+' (audit-tolerated, Hot-rejected)',ok?'PASS':'FAIL',{auditTolerated:aTol,hot:code});
  }
  // 合法负零不拒
  const f=wet();f[key][4]=-0;
  const okZ=f.particleBinding.beginCollect(1/60,0,1,1,0,0,0)===0;
  if(okZ)f.particleBinding.rollback();
  rec('six-component '+key+' -0 accepted',okZ?'PASS':'FAIL');
}

// ---------- 4. 首 错列与跨列竞争（waterTop 专属）----------
{
  const f=dry(true);f.waterTop[10]=-2e-9;f.bh[4]=-1; // 更早列其它分量坏
  const code=begin(f);const ev=f.particleBinding.work.fieldColumnEvidenceV2.failure;
  const ok=code===3&&ev&&ev.column===4; // 首错列胜
  rec('first-column-wins: bh col4 beats waterTop col10',ok?'PASS':'FAIL',{col:ev&&ev.column});
}
{
  const f=dry(true);f.waterTop[3]=-2e-9;f.waterTop[9]=-1; // 最早 waterTop 坏列胜
  const code=begin(f);const ev=f.particleBinding.work.fieldColumnEvidenceV2.failure;
  const ok=code===3&&ev&&ev.column===3&&ev.rawBits[0]===bitsHex(-2e-9);
  rec('earliest waterTop bad column wins among same-component',ok?'PASS':'FAIL',{col:ev&&ev.column});
}

// ---------- 5. raw sign 诊断与 Hot 接受并存（关键语义: raw legal=false 不等于产品失败）----------
{
  const f=dry(true),ev=f.particleBinding.work.fieldColumnEvidenceV2;
  f.observeFieldColumnV2('BEFORE','T167');
  f.waterTop[48]=-(2**-55);
  f.observeFieldColumnV2('RESIDUE','T167');
  const before=snapshot(f);
  const code=begin(f);
  f.particleBinding.rollback();
  const a=ev.exportEvidence();
  const ok=code===0
    &&a.status==='NO_FAILURE'&&a.failure===null
    &&a.windowStatus==='LEGAL_TO_INVALID'
    &&a.stages.at(-1).firstInvalidColumn===48
    &&a.stages.at(-1).legal===false
    &&Buffer.from(a.firstIllegalBytesHex,'hex').readDoubleLE(48*8)===-(2**-55)
    &&deepEq(snapshot(f),before);
  rec('raw-sign observer flags residue while Hot accepts (NO_FAILURE + LEGAL_TO_INVALID)',ok?'PASS':'FAIL',{hot:code,status:a.status,window:a.windowStatus});
}

// ---------- 6. 导出顺序/分片/首错锁存 ----------
{
  const f=dry(true),ev=f.particleBinding.work.fieldColumnEvidenceV2;
  f.observeFieldColumnV2('LEGAL','T167');f.waterTop[48]=-2e-9;begin(f);
  const a=ev.exportEvidence(),keys=Object.keys(a);
  const ok1=keys.indexOf('failure')<keys.indexOf('lastLegalBytesHex')&&keys.indexOf('stages')<keys.indexOf('lastLegalBytesHex');
  const s=JSON.stringify(a);
  const frags=[];for(let i=0;i<s.length;i+=512)frags.push(s.slice(i,i+512));
  const rt=JSON.stringify(JSON.parse(frags.join('')));
  // 首错锁存: 修复后再次不同错误不覆盖
  f.waterTop[48]=0;f.suspH[2]=NaN;begin(f);
  const same=ev.failure===a.failure&&ev.failure.column===48;
  const ok=ok1&&rt===s&&frags.every(x=>x.length<=512)&&same
    &&a.lastLegalBytesHex.length===16384&&a.firstIllegalBytesHex.length===16384
    &&a.failure.poolCanonicalBytesHex.length===13&&a.failure.poolStorageValid===true;
  rec('export priority failure/stages first; 512B fragment round-trip; first-failure latch',ok?'PASS':'FAIL',{keyOrderOK:ok1,latched:same,fragments:frags.length});
}

// ---------- 7. 观测开关等价（含 begin/rollback 循环后）----------
{
  const x=dry(false),y=dry(true);
  for(const f of [x,y])f.waterTop[48]=-(2**-55);
  y.observeFieldColumnV2('RAW_RESIDUE','T167');
  let eq1=deepEq(snapshot(x),snapshot(y));
  for(const f of [x,y]){const c=begin(f);assert.equal(c,0);f.particleBinding.rollback();}
  let eq2=deepEq(snapshot(x),snapshot(y));
  y.observeFieldColumnV2('POST_ROLLBACK','T167');
  const a=y.particleBinding.work.fieldColumnEvidenceV2.exportEvidence();
  for(const f of [x,y]){const c=begin(f);assert.equal(c,0);f.particleBinding.rollback();assert.equal(f.audit(true),null);}
  let eq3=deepEq(snapshot(x),snapshot(y));
  rec('observation on/off equivalence incl. post-rollback cycles (source/bit-preserving)',eq1&&eq2&&eq3?'PASS':'FAIL',{eq1,eq2,eq3,windowStatus:a.windowStatus});
}

// ---------- 8. 源码身份: 唯一物理改动+常量一致 ----------
{
  const hot=fs.readFileSync('entry/src/main/ets/physics/P011ParticleHot.ets','utf8');
  const field=fs.readFileSync('entry/src/main/ets/physics/WaterSandField.ets','utf8');
  const evSrc=fs.readFileSync('entry/src/main/ets/physics/P011FieldColumnEvidenceV2.ets','utf8');
  const ok=/const WATER_TOP_AUDIT_EPS = 1e-9;/.test(hot)
    &&/f\.waterTop\[col\] < -WATER_TOP_AUDIT_EPS/.test(hot)
    &&!/f\.waterTop\[col\] < 0 \|\|/.test(hot)
    &&/const EPS = 1e-9;/.test(field)
    &&/this\.waterTop\[c\] < -EPS/.test(field)
    &&/T146 §4: place failure details\/window before raw snapshots/.test(evSrc);
  rec('source identity: WATER_TOP_AUDIT_EPS=1e-9 aligns audit EPS; export reorder comment present',ok?'PASS':'FAIL');
}
{
  // patch 范围独立核验: 1f8e16c 产品源码改动仅两个文件
  const names=require('node:child_process').execSync('git show 1f8e16c --name-only --format=').toString().trim().split('\n');
  const product=names.filter(n=>n.startsWith('entry/'));
  const ok=product.length===2&&product.includes('entry/src/main/ets/physics/P011ParticleHot.ets')
    &&product.includes('entry/src/main/ets/physics/P011FieldColumnEvidenceV2.ets');
  rec('patch scope: 1f8e16c product changes limited to Hot+EvidenceV2',ok?'PASS':'FAIL',{product});
}

// ---------- 输出 ----------
const failed=results.filter(r=>r.status==='FAIL');
fs.mkdirSync('test/reports/t167',{recursive:true});
fs.writeFileSync('test/reports/t167/independent.json',JSON.stringify({
  revision:'T167_FIELD_COLUMN_INDEPENDENT_V1',commitUnderTest:'1f8e16c',
  executedAt:new Date().toISOString(),device:'NOT_RUN (offline per card)',
  total:results.length,passed:results.length-failed.length,failed:failed.length,
  oracle:{EPS,EPS_DOWN,EPS_UP,method:'nextafter via DataView bit manipulation'},
  results},null,2)+'\n');
console.log('T167 INDEPENDENT: '+(results.length-failed.length)+'/'+results.length+(failed.length?' FAILED':' ALL PASS'));
process.exit(failed.length?1:0);
