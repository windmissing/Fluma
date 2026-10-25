'use strict';
const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const hot=require('../entry/src/main/ets/physics/P011ParticleHot.ets');
const {P011ParticleHotPrototypeSink,guardAdmitInto,P011ParticleHotGuardBuffer}=require('../entry/src/main/ets/physics/P011ParticleHotPrototype.ets');
const {P011ParticleHotTicketReceipt,P011ParticleHotRefundReceipt}=require('../entry/src/main/ets/physics/P011ParticleHotBudget.ets');
const legacy=require('../entry/src/main/ets/physics/P011ParticleField.ets');
const {P011ParticlePrototypeSink,P011ParticlePrototypeBudget}=require('../entry/src/main/ets/physics/P011ParticlePrototype.ets');
const guard=require('../entry/src/main/ets/physics/P011MaterialGuard.ets');
const params={massPackage:.1,nmax:8,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:2,dragLight:2,
  liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0};
const arrays=['waterTop','bh','bl','mh','ml','suspH','suspL','surface'];
function setup(extra={},scale=1){
  const w=hot.configureHot({cols:128,nmax:8,requestCapacity:64,motionCapacity:8,eventCapacity:64,lineageCapacity:128,params,...extra});
  const f={cols:128,capacity:512,sourceInjectedH:0,sourceInjectedL:0,sourceEpoch:0};
  for(const n of arrays)f[n]=new Float64Array(128);f.bh.fill(20);f.surface.fill(20);f.waterTop.fill(400);
  const sink=new P011ParticleHotPrototypeSink(w,scale);
  const input={runToken:1,substepId:1,attemptToken:1,dt:1/60,effectiveTiltRad:.2,seed:17,paramsRevision:1,inputRevision:1,field:f};
  return {w,f,sink,input,start:()=>{assert.equal(hot.beginStepHot(w,input),0);return w.stepEpoch;}};
}
function seed(p,extra={}){p.allocate({mass:1,x:20.5,y:30,vx:2,vy:-3,phase:0,sourceKind:0,sourceId:1,...extra});}
const kinds={spawn:1,deposit:2,crossing:3,lift:4,degrade:5,merge:6};
function planFor(w,kind,extra={}){
  const r={requestKey:'r1',substepId:1,ordinal:1,kind,particleId:1,sourceId:1,phase:0,sourceKind:kind==='lift'?1:0,
    column:20,targetColumn:kind==='crossing'?21:-1,requested:1,totalWeight:kind==='deposit'||kind==='crossing'?1:0,
    predictedX:kind==='crossing'?21.5:20.5,predictedY:30,predictedVx:2,predictedVy:-3,mergedIds:[],...extra};
  for(const p of [w.planA,w.planB]){
    Object.assign(p,{ownerId:w.ownerId,runToken:w.runToken,stepEpoch:w.stepEpoch,substepId:w.substepId,attemptToken:w.attemptToken,
      paramsRevision:w.paramsRevision,inputRevision:w.inputRevision,seed:w.seed,dt:w.dt,effectiveTiltRad:w.effectiveTiltRad,
      requestCount:1,motionCount:0,totalRequested:r.totalWeight,valid:1,consumed:0});
    p.kind[0]=kinds[kind];for(const k of ['phase','sourceKind','column','targetColumn','particleId','sourceId','ordinal','requested',
      'totalWeight','predictedX','predictedY','predictedVx','predictedVy'])p[k][0]=r[k];
    p.substep[0]=r.substepId;p.mergeStart[0]=0;p.mergeCount[0]=r.mergedIds.length;p.lineage.set(r.mergedIds);
  }
  return r;
}
function reference(w,f,r,scale=1){
  const pool=new legacy.P011ParticleField(128,8);pool.restoreSnapshot(w.pool.snapshot());
  const id={runId:'1',substepId:1,dt:1/60,effectiveTiltRad:.2,seed:17,paramsRevision:1,inputRevision:1};
  const sink=new P011ParticlePrototypeSink(pool,params,id,new P011ParticlePrototypeBudget(128,1/60,scale,1),512,64);
  for(const k of arrays.filter(k=>k!=='surface'))sink[k].set(f[k]);
  const journal=new legacy.P011ParticleJournal(id,64);journal.record(r);const plan=journal.finish();
  return {sink,result:sink.executeParticlePlan(plan)};
}
function state(s){return {pool:s.w.pool.snapshot(),field:arrays.map(k=>s.f[k].slice()),source:[s.f.sourceInjectedH,s.f.sourceInjectedL,s.f.sourceEpoch]};}
function run(s){return hot.applyParticlePlanInto(s.w,s.w.stepEpoch,s.w.planA,s.sink,s.w.results);}
let passed=0;function test(n,f){f();passed++;console.log('PASS '+n);}
for(const kind of Object.keys(kinds))test('S3 '+kind+' agrees with cold pool/field/budget/results/events',()=>{
  const s=setup();if(!['spawn','lift'].includes(kind))seed(s.w.pool,{mass:kind==='crossing'?.2:1});
  if(kind==='merge')seed(s.w.pool,{mass:.7,x:20.7,y:31});
  if(kind==='lift'){s.f.mh[20]=.12;s.f.ml[20]=.13;s.f.surface[20]=20.25;}
  s.start();const r=planFor(s.w,kind,kind==='spawn'||kind==='lift'?{particleId:0,requested:.25}:
    kind==='crossing'?{requested:.2,totalWeight:.2}:kind==='merge'?{mergedIds:[1,2]}:{});
  const ref=reference(s.w,s.f,r);assert.equal(ref.result.ok,true,ref.result.errorCode);
  assert.equal(run(s),0);assert.deepEqual(s.w.pool.snapshot(),ref.sink.pool.snapshot());
  for(const k of arrays.filter(k=>k!=='surface'))assert.deepEqual(s.f[k],ref.sink[k],k);
  assert.deepEqual(s.sink.budget.localUsed,ref.sink.budgets.localUsed);assert.deepEqual(s.sink.budget.localRemaining,ref.sink.budgets.localRemaining);
  assert.equal(s.sink.budget.totalUsed,ref.sink.budgets.totalUsed);assert.equal(s.sink.budget.totalRemaining,ref.sink.budgets.totalRemaining);
  for(const k of ['granted','remainder','budgetClipped','guardRejected','capacityClipped','poolClipped'])assert.equal(s.w.results[k][0],ref.result.results[0][k],k);
  assert.equal(s.w.events.count,ref.sink.events.length);
  for(let i=0;i<s.w.events.count;i++){
    const e=ref.sink.events[i];assert.equal(s.w.events.kind[i],({SPAWN:1,LAND:2,CROSS_COLUMN:3,LIFT:4,DEGRADE:5,MERGE:6})[e.kind]);
    for(const [hk,ck] of Object.entries({particleId:'particleId',sourceId:'sourceId',phase:'phase',sourceKind:'sourceKind',column:'column',
      targetColumn:'targetColumn',requested:'requested',granted:'granted',remainder:'remainder',deltaS:'deltaS',
      budgetClipped:'budgetClipped',guardRejected:'guardRejected',capacityClipped:'capacityClipped',poolClipped:'poolClipped'}))assert.equal(s.w.events[hk][i],e[ck],hk);
    if(e.guardPreview){
      assert.equal(s.w.events.guardAccepted[i],e.guardPreview.accepted);assert.equal(s.w.events.guardRejectedAmount[i],e.guardPreview.rejected);
      assert.equal(!!s.w.events.guardAllLegal[i],e.guardPreview.allLegalBefore);
      assert.equal(s.w.events.guardLimitingEdge[i],e.guardPreview.limitingEdge===null?-1:e.guardPreview.limitingEdge);
    }
  }
  assert.equal(hot.finishStepHot(s.w,s.w.stepEpoch),0);
});
test('S3 capacity crossing preflight debits no budget and retains whole grain',()=>{
  const s=setup();seed(s.w.pool,{mass:.2});s.f.waterTop[21]=20;s.start();planFor(s.w,'crossing',{requested:.2,totalWeight:.2});
  assert.equal(run(s),0);assert.equal(s.w.results.capacityClipped[0],.2);assert.equal(s.sink.budget.totalUsed,0);
  assert.equal(s.w.pool.snapshot().pX[0],21-1e-9);assert.equal(s.w.pool.snapshot().pMass[0],.2);
});
test('S3 pool-full lift degrades once without extra budget debit; spawn refusal preserves mass',()=>{
  for(const kind of ['lift','spawn']){
    const s=setup();for(let i=0;i<8;i++)seed(s.w.pool,{mass:.1,sourceId:i+10});
    s.start();const r=planFor(s.w,kind,{particleId:0,sourceId:99,requested:.25}),ref=reference(s.w,s.f,r);
    assert.equal(ref.result.ok,true,ref.result.errorCode);assert.equal(run(s),0);
    assert.deepEqual(s.w.pool.snapshot(),ref.sink.pool.snapshot());assert.deepEqual(s.f.suspH,ref.sink.suspH);
    assert.equal(s.sink.budget.totalUsed,ref.sink.budgets.totalUsed);assert.equal(s.w.events.count,ref.sink.events.length);
    assert.equal(s.w.results.poolClipped[0],kind==='spawn'?.25:0);
  }
});
test('S3 numeric R9 multiple refunds debit each end and total once; old primitive rejected after close',()=>{
  const s=setup({},.5);seed(s.w.pool);s.start();planFor(s.w,'crossing');
  const ticket=new P011ParticleHotTicketReceipt(),out=new P011ParticleHotRefundReceipt(),b=s.sink.budget;
  assert.equal(b.reserveInto(s.w.stepEpoch,s.w.planA,0,ticket),0);const token=ticket.ticketToken,a=ticket.reserved;
  assert.equal(b.refundInto(token,.1,out),0);assert.equal(b.refundInto(token,.05,out),0);
  assert.deepEqual([...out.localRefunded],[.15000000000000002,.15000000000000002]);
  assert.equal(b.localUsed[20],a-.1-.05);assert.equal(b.localUsed[21],a-.1-.05);assert.equal(b.totalUsed,a-.1-.05);
  assert.equal(out.bottomReserved,0);assert.equal(out.bottomRefunded,0);
  assert.equal(b.closeInto(token,a-.15,out),0);const frozen=JSON.stringify(out),used=b.totalUsed;
  assert.equal(b.refundInto(token,.01,out),4);assert.equal(JSON.stringify(out),frozen);assert.equal(b.totalUsed,used);
});
test('S3 over-refund, stale epoch and aliased receipt refuse before every ledger/out write',()=>{
  const s=setup();seed(s.w.pool);const epoch=s.start();planFor(s.w,'crossing');const b=s.sink.budget;
  const t=new P011ParticleHotTicketReceipt(),out=new P011ParticleHotRefundReceipt();assert.equal(b.reserveInto(epoch,s.w.planA,0,t),0);
  const token=t.ticketToken,used=b.totalUsed,before=JSON.stringify(out);
  assert.equal(b.refundInto(token,t.reserved+1,out),2);assert.equal(JSON.stringify(out),before);assert.equal(b.totalUsed,used);
  out.localReserved=out.localRefunded;assert.equal(b.refundInto(token,.1,out),3);assert.equal(b.totalUsed,used);
  assert.equal(hot.rollbackStepHot(s.w,epoch),0);s.input.attemptToken++;s.start();planFor(s.w,'crossing');
  assert.equal(b.reserveInto(epoch,s.w.planA,0,t),4);assert.equal(b.refundInto(token,0,new P011ParticleHotRefundReceipt()),4);
  assert.equal(b.reserveInto(s.w.stepEpoch,s.w.planA,0,t),0);assert.ok(t.ticketToken>token);assert.equal(b.totalUsed,used);
});
test('S3 injected post-publish failure and journal overflow restore all canonical buckets and budgets',()=>{
  for(const failure of ['publish','journal']){
    const s=setup(failure==='journal'?{eventCapacity:1}:{});s.start();planFor(s.w,'spawn',{particleId:0,requested:.25});
    const before=state(s);s.sink.failAfterPublish=failure==='publish'?1:0;
    assert.equal(run(s),failure==='publish'?13:9);assert.deepEqual(state(s),before);
    assert.equal(s.w.results.resultCount,0);assert.equal(s.w.events.count,0);assert.equal(s.w.activePatchToken,0);
    assert.equal(s.sink.budget.totalUsed,0);assert.equal(s.w.active,0);assert.equal(s.w.failureAttemptToken,1);
  }
});
test('S3 rollback restores fixed ticket, refund and guard outs with the canonical checkpoint',()=>{
  const s=setup();seed(s.w.pool);s.start();planFor(s.w,'deposit');s.sink.failAfterPublish=1;
  const outs=JSON.stringify([s.sink.ticket,s.sink.refund,s.sink.guard]);
  assert.equal(run(s),13);assert.equal(JSON.stringify([s.sink.ticket,s.sink.refund,s.sink.guard]),outs);
});
test('S3 dual collect mismatch rolls back before sink/real denominator; motions apply after transactions',()=>{
  const s=setup();seed(s.w.pool);s.start();
  assert.equal(hot.collectParticlePlanInto(s.w,s.w.stepEpoch,s.w.planA),0);assert.equal(hot.collectParticlePlanInto(s.w,s.w.stepEpoch,s.w.planB),0);
  const before=state(s);s.w.planB.motionX[0]+=.01;assert.equal(run(s),12);assert.deepEqual(state(s),before);assert.equal(s.sink.budget.denominator,0);
  s.input.attemptToken++;s.start();assert.equal(hot.collectParticlePlanInto(s.w,s.w.stepEpoch,s.w.planA),0);
  assert.equal(hot.collectParticlePlanInto(s.w,s.w.stepEpoch,s.w.planB),0);const x=s.w.planA.motionX[0];
  assert.equal(run(s),0);assert.equal(s.w.pool.snapshot().pX[0],x);assert.equal(hot.finishStepHot(s.w,s.w.stepEpoch),0);
});
test('S3 guard matches cold closed form on boundaries, legal/non-worsening edges, signs and zero grants',()=>{
  const s=setup();s.start();const out=new P011ParticleHotGuardBuffer();
  for(let i=0;i<1024;i++){
    const c=[0,127,60,61][i%4],sign=i%2?1:-1,amount=i%7===0?0:(i%31)/10;
    s.f.bh.fill(20);if(c>0)s.f.bh[c-1]=20+(i%13-6)*.13;s.f.bh[c]=20+(i%11-5)*.11;if(c<127)s.f.bh[c+1]=20+(i%17-8)*.12;
    const delta=new Map([[c,sign]]),residual=e=>s.f.bh[e+1]-s.f.bh[e]-Math.tan(s.w.effectiveTiltRad);
    const constraints=guard.p011GuardConstraintsForDelta(delta,residual,127),edges=guard.p011GuardAffectedEdgesForDelta(delta,127);
    const expected=guard.p011GuardAdmit(amount,constraints,Math.tan(Math.PI/6));
    assert.equal(guardAdmitInto(s.w,s.w.stepEpoch,c,sign,amount,out),0);assert.equal(out.accepted,expected.accepted);
    assert.equal(out.rejected,expected.rejected);assert.equal(!!out.allLegalBefore,expected.allLegalBefore);
    assert.equal(out.limitingEdge,expected.limitingEdge===null?-1:edges[expected.limitingEdge]);
    assert.deepEqual([...out.edgeId].slice(0,out.count),edges);
  }
});
console.log(JSON.stringify({suite:'run_p011_particle_hot_sink_units',passed,failed:0}));
