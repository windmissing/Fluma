'use strict';
const fs=require('node:fs'),a=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const {P011PoolProgressDiagnostic}=require('../entry/src/main/ets/physics/P011PoolProgressDiagnostic.ets');
const results=[],measurements=[];function test(name,fn){fn();results.push({name,status:'PASS'});console.log('PASS '+name);}
function make(nmax=16,on=true,clock=undefined){const f=new WaterSandField(128,512),p=defaultP011ParticleParams();p.nmax=nmax;
  a.equal(f.setParticleRoute(p).ok,true);f.setNaturalObservationRouteId('dual-field-particle-v1');
  let t=0;if(on)f.setPoolProgressDiagnosticEnabled(true,clock===undefined?()=>t+=.01:clock);
  return f;}
function inject(f,mass=.35){const h=new Float64Array(128),l=new Float64Array(128);h[64]=mass;
  return f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:8,sequence:1,inputTime:0,x:64.5,y:100,seed:1});}
function progress(f){return JSON.parse(f.getPoolProgressDiagnostic()).progressV3;}
function counters(p){return Object.fromEntries(p.counterNames.map((name,i)=>[name,p.counters[i]]));}
function fill(f,compatible){const b=f.particleBinding;for(let i=0;i<b.work.nmax;i++)
  a.ok(b.pool.allocate({mass:.1,x:compatible?64.1+(i%4)*.1:(i%128)+.5,y:100+(i%4)*.1,
    vx:0,vy:0,phase:0,sourceKind:0,sourceId:compatible?8:i+1})>=0);}
function samePhysics(x,y){for(const k of ['bh','bl','mh','ml','suspH','suspL','waterTop','sandTopScratch',
  'substepMotionBudgetRemaining','substepMotionBudgetUsed'])a.deepEqual(x[k],y[k],k);
  a.deepEqual(x.particleBinding.pool.snapshot(),y.particleBinding.pool.snapshot());
  for(const k of ['sourceEpoch','sourceInjectedHeavy','sourceInjectedLight','renderPhysicsCommitId',
    'totalMotionBudgetRemaining','totalMotionBudgetUsed'])a.equal(x[k],y[k],k);}
test('real successful seed measures three distinct audits and four actual birth packages',()=>{
  const f=make();a.equal(inject(f).admittedH,.35);const p=progress(f),c=counters(p);
  a.equal(p.revision,'P011_POOL_PROGRESS_DIAGNOSTIC_V3');
  a.equal(c.seedCalls,1);a.equal(c.auditSinkRow,1);a.equal(c.auditApplyTail,1);a.equal(c.auditFinish,1);
  a.equal(c.sortCollectCalls,1);a.equal(c.sortAuditCalls,3);a.equal(c.birthRequested,4);
  a.ok(c.poolCopies>=5);a.ok(c.sortComparisons>0);a.ok(c.sortSiftIterations>0);
  const row=p.ring.find(x=>x.stage==='SEED'&&x.status==='COMPLETE');a.ok(row);
  a.equal(row.nmax,16);a.equal(row.live,4);a.equal(row.free,12);a.equal(row.sourceKind,0);
  a.equal(row.column,64);a.equal(row.phase,0);a.equal(row.requestedBirths,4);
  a.equal(row.delta[p.counterNames.indexOf('auditFinish')],1);
  a.ok(row.endMs>=row.startMs);a.equal(p.clockStatus,'MONOTONIC_INJECTED_MS');
  measurements.push({fixture:'single seed .35 / package .1',nmax:16,counters:c,seedRow:row});
});
test('full-pool merge captures independent sort/tree/range workload',()=>{
  const f=make(192);fill(f,true);a.equal(inject(f,.1).admittedH,.1);
  const p=progress(f),c=counters(p);a.equal(c.indexRebuilds,1);a.equal(c.indexSortCalls,2);
  a.ok(c.indexComparisons>0);a.ok(c.indexSiftIterations>0);a.ok(c.indexTreeVisits>0);a.ok(c.indexRangeQueries>0);
  a.equal(f.particleBinding.work.mergeIndex.rebuilds,c.indexRebuilds);
  a.equal(f.particleBinding.work.mergeIndex.sortComparisons,c.indexComparisons);
  a.ok(p.first32.length<=32&&p.ring.length<=32);a.ok(p.dropped>0);
  a.ok(p.ring.some(x=>x.root>=0||x.parent>=0));
  measurements.push({fixture:'compatible full pool',nmax:192,counters:c});
});
test('lambda retries and rollback retain actual per-seed deltas, bounded first32 and no fake finish audit',()=>{
  const off=make(16,false),on=make(16);fill(off,false);fill(on,false);
  const before=on.particleBinding.pool.snapshot();a.equal(inject(off,.1).admittedH,0);a.equal(inject(on,.1).admittedH,0);
  samePhysics(off,on);a.deepEqual(on.particleBinding.pool.snapshot(),before);
  const p=progress(on),c=counters(p);a.equal(c.seedCalls,8);a.equal(c.indexRebuilds,8);
  a.equal(c.auditFinish,0);a.equal(c.auditSinkRow,8);a.equal(c.auditApplyTail,8);
  a.equal(p.first32.length,32);a.equal(p.ring.length,32);
  const last=p.ring.find(x=>x.stage==='SEED'&&x.status==='COMPLETE');a.ok(last);
  a.equal(last.error,-1);a.equal(last.lambdaAttempt,7);a.equal(last.lambda,1/128);
  a.equal(last.delta[p.counterNames.indexOf('indexRebuilds')],1);
  measurements.push({fixture:'incompatible full pool eight lambda attempts',nmax:16,counters:c,lastSeed:last});
});
test('first long window captures original pool bytes once and remains frozen after mutation',()=>{
  const f=make();const d=f.particleBinding.work.poolProgress;d.thresholdMs=0;
  a.equal(inject(f).admittedH,.35);let p=progress(f);
  a.ok(p.firstLong);a.equal(p.inputStatus,'EXACT_STAGE_POOL_BYTES');a.equal(p.inputPoolHex.length,13);
  a.equal(p.exactFullTransactionReplay,false);a.equal(p.inputHashSchema,'FNV32_RAW_BYTES_CORRELATION_ONLY');
  const hex=JSON.stringify(p.inputPoolHex),hash=p.inputHash;
  f.particleBinding.pool.reset();p=progress(f);a.equal(JSON.stringify(p.inputPoolHex),hex);a.equal(p.inputHash,hash);
  a.equal(p.configF64[1],16);
});
test('missing clock and open spans export NOT_CAPTURED/null rather than measured zero',()=>{
  const f=make(16,true,null),d=f.particleBinding.work.poolProgress;
  const span=d.begin('UNFINISHED_TEST');const p=progress(f);
  a.equal(p.clockStatus,'NOT_CAPTURED');const row=p.running.find(x=>x.stage==='UNFINISHED_TEST');
  a.equal(row.status,'RUNNING');a.equal(row.startMs,null);a.equal(row.endMs,null);a.equal(row.delta,null);
  d.endSpan(span,9);a.equal(progress(f).ring.at(-1).error,9);
});
test('diagnostic on/off and throwing clock preserve field, budgets, pool and commits',()=>{
  const off=make(16,false),on=make(16,true,()=>{throw Error('clock unavailable');});
  a.equal(inject(off).admittedH,.35);a.equal(inject(on).admittedH,.35);samePhysics(off,on);
  a.equal(progress(on).clockStatus,'CLOCK_UNAVAILABLE');
  for(let i=0;i<2;i++){const x=off.step(1/60),y=on.step(1/60);delete x.physicsTimeMs;delete y.physicsTimeMs;
    a.deepEqual(x,y);samePhysics(off,on);}
  a.equal(off.getPoolProgressDiagnostic(),null);
});
test('invalid ID early exit retains error and does not invent sorting/audit completion',()=>{
  const off=make(16,false),on=make(16);
  for(const f of [off,on]){const b=f.particleBinding;const slot=b.pool.allocate({mass:.1,x:64.5,y:100,vx:0,vy:0,
    phase:0,sourceKind:0,sourceId:8});b.pool.pId[slot]=0;}
  a.equal(inject(off,.1).admittedH,0);a.equal(inject(on,.1).admittedH,0);samePhysics(off,on);
  const c=counters(progress(on));a.equal(c.sortCollectCalls,0);a.equal(c.auditFinish,0);
  a.ok(progress(on).ring.some(x=>x.status==='COMPLETE'&&x.error!==0));
});
test('running long-span marker is partial, and a backwards clock is rejected',()=>{
  const f=make(),d=f.particleBinding.work.poolProgress;d.thresholdMs=0;
  const span=d.begin('LONG_RUNNING');d.increment(6,5);
  for(let i=0;i<128;i++)d.cursor(2,15,4,9,16);
  const p=progress(f);a.equal(p.firstLong.status,'RUNNING_PARTIAL');a.equal(p.firstLong.endMs,null);
  a.equal(p.firstLong.delta[6],5);a.equal(p.firstLong.parent,4);a.equal(p.firstLong.child,9);
  d.endSpan(span,0);
  let time=10;const backwards=make(16,true,()=>time--);const b=backwards.particleBinding.work.poolProgress;
  const handle=b.begin('BACKWARDS');b.endSpan(handle,0);
  a.equal(progress(backwards).clockStatus,'CLOCK_INVALID');
  a.equal(progress(backwards).ring.at(-1).elapsedMs,null);
});
fs.mkdirSync('test/reports/t154',{recursive:true});
fs.writeFileSync('test/reports/t154/selftest.json',JSON.stringify({suite:'T154',passed:results.length,failed:0,
  clock:'DETERMINISTIC_OFFLINE_TEST_NOT_DEVICE',results,measurements},null,2)+'\n');
console.log(`T154 ${results.length}/${results.length} PASS`);
