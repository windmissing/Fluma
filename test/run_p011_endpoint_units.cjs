'use strict';
const fs=require('fs'),assert=require('assert/strict'),Module=require('module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
let uptime=1000,clockCalls=0;
const original=Module._load;
Module._load=function(name,parent,main){return name==='@ohos.systemDateTime'?{default:{
  TimeType:{STARTUP:1},getUptime:(type,nano)=>{assert.equal(type,1);assert.equal(nano,false);clockCalls++;return uptime;}
}}:original.call(this,name,parent,main);};
require.extensions['.ets']=(mod,file)=>mod._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,file);
const ep=require('../entry/src/main/ets/physics/P011EndpointMetrics.ets');
const {P011EndpointStartupClock}=require('../entry/src/main/ets/physics/P011EndpointStartupClock.ets');
const {WaterSandField,defaultWaterSandParams,WaterSandErrorCode}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {P011NaturalObservationProbe}=require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
const {P011ParticleField}=require('../entry/src/main/ets/physics/P011ParticleField.ets');
const results=[];
function test(name,fn){try{fn();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:e.stack});}}
function phase(field){return {occurrenceId:'phase-occ-1',commandSequence:field.p011InputSequence,commandPhaseId:field.commandPhaseId,segmentId:'segment-1'};}
function field(){const f=new WaterSandField(128,512); const par=defaultWaterSandParams();par.heavyDiffusion=0;par.lightDiffusion=0;par.angleDeg=4;f.configure(par);
  f.initialize({waterRegions:[{startCol:0,endColInclusive:127,bottom:300,height:160}],
    heavyDepositedRegions:[{startCol:0,endColInclusive:127,bottom:0,height:100}],
    lightDepositedRegions:[{startCol:0,endColInclusive:127,bottom:100,height:100}],heavySuspendedRegions:[],lightSuspendedRegions:[]});return f;}
test('three displayed fractions use E; missing C P remains null; invalid E not EPS PASS',()=>{
  const r=ep.p011EndpointMassFractions(900,40,60,1000,'CANONICAL');
  assert.equal(r.depositedFraction,0.9);assert.equal(r.suspFraction,0.04);assert.equal(r.airborneFraction,0.06);
  assert.equal(ep.p011EndpointMassFractions(0,0,0,0,'STRUCTURAL_ZERO_OFF').suspFraction,null);
  assert.equal(ep.p011EndpointMassFractions(900,40,null,1000,'NOT_IMPLEMENTED').airborneFraction,null);
});
test('clock adapter uses actual STARTUP ms; equal ms allowed; no guessed resolution',()=>{
  uptime=1000;clockCalls=0;const c=new P011EndpointStartupClock('r');uptime=1005;
  assert.equal(c.nowElapsedMs(),5);assert.equal(c.nowElapsedMs(),5);assert.equal(clockCalls,3);
  assert.equal(c.endpointClockResolutionMs,null);assert.equal(c.runStartClockMs,1000);
});
test('sampler duplicate/backward clock rejection and immutable same-ms commit identity',()=>{
  let now=0;const clock={clockId:'r:startup-elapsed-v1',nowElapsedMs:()=>now};const s=new ep.P011EndpointSampler('r',clock);
  const k={runId:'r',sequence:1,frameId:1,substepId:1,attemptId:'a',committedOrdinal:1};
  const ph={occurrenceId:'occ-1',commandSequence:0,commandPhaseId:0,segmentId:'s'};
  const a=s.stampEndpointPostCommit(k,1/60,ph,clock);k.committedOrdinal=2;k.substepId=2;
  const b=s.stampEndpointPostCommit(k,1/60,ph,clock);assert.equal(a.endpointSampleElapsedMs,b.endpointSampleElapsedMs);
  assert.equal(a.key.substepId,1);ph.occurrenceId='changed';assert.equal(a.phase.occurrenceId,'occ-1');
  assert.equal(s.committedOrdinal,2);assert.equal(s.integratedPhysicsSeconds,2/60);
  assert.throws(()=>s.stampEndpointPostCommit(k,1/60,ph,clock));assert.equal(s.committedOrdinal,2);
  now=-1;k.committedOrdinal=3;k.substepId=3;assert.throws(()=>s.stampEndpointPostCommit(k,1/60,ph,clock),/CLOCK/);
  assert.equal(s.integratedPhysicsSeconds,2/60); // invalid elapsed is not a recordable commit stamp
});
test('real field two substeps same frame stamp POST_COMMIT, cumulative dt exactly once',()=>{
  const f=field(),records=[];f.setNaturalObservationProbe(new P011NaturalObservationProbe(128,r=>records.push(r)));
  const clock={clockId:f.getP011RunId()+':startup-elapsed-v1',nowElapsedMs:()=>1000};
  f.setEndpointSampling(clock,phase(f));const result=f.step(1/30);
  assert.equal(result.ok,true);assert.equal(records.length,2);
  assert.deepEqual(records.map(r=>r.context.endpointCommittedOrdinal),[1,2]);
  assert.deepEqual(records.map(r=>r.context.endpointSampleElapsedMs),[1000,1000]);
  assert.equal(records[0].context.frameId,records[1].context.frameId);
  assert.equal(records[1].context.substepId,records[0].context.substepId+1);
  assert.equal(records[1].context.integratedPhysicsSeconds,1/30);
  assert.ok(records.every(r=>r.context.endpointSampleStage==='POST_COMMIT'));
});
test('failed physical audit records attempt without stamp/ordinal/integration advance',()=>{
  const f=field(),records=[];f.setNaturalObservationProbe(new P011NaturalObservationProbe(128,r=>records.push(r)));
  f.setEndpointSampling({clockId:'fixture',nowElapsedMs:()=>0},phase(f));
  f.audit=()=>WaterSandErrorCode.UNKNOWN;assert.equal(f.step(1/60).ok,false);
  assert.equal(records.length,1);assert.equal(records[0].context.endpointSampleStage,null);
  assert.equal(records[0].context.endpointCommittedOrdinal,null);assert.equal(records[0].context.integratedPhysicsSeconds,0);
});
test('reset clears sampling; disabled writer leaves old ledger fields absent',()=>{
  const f=field(),records=[];f.setNaturalObservationProbe(new P011NaturalObservationProbe(128,r=>records.push(r)));
  assert.equal(f.step(1/60).ok,true);assert.ok(!('endpointSampleStage' in records[0].context));
  f.reset();f.setEndpointSampling({clockId:'new',nowElapsedMs:()=>0},phase(f));
  f.reset();assert.equal(f.endpointSampler,null);assert.equal(f.step(1/60).ok,true);
  assert.ok(!('endpointSampleStage' in records.at(-1).context));
});
test('legacy suspensionFraction formula is unchanged; explicit C missing summary not fabricated',()=>{
  const f=field(),m=f.getMetrics();const total=f.getTotals();const s=f.snapshot();
  const u=Array.from(s.suspH).reduce((a,b)=>a+b,0)+Array.from(s.suspL).reduce((a,b)=>a+b,0);
  assert.equal(m.suspensionFraction,u/Math.max(total.heavy+total.light,1e-9));
  assert.equal(m.particleEvidenceKind,'STRUCTURAL_ZERO_OFF');
  f.setNaturalObservationRouteId('dual-field-particle-v1');assert.equal(f.getMetrics().particleMass,null);
  assert.equal(f.getMetrics().particleEvidenceKind,'NOT_IMPLEMENTED');
});
test('byState uses exact positive remainder, ignores empty slots and checks cache closure',()=>{
  const pool=new P011ParticleField(128,8);
  const a=pool.allocate({mass:1,x:60.5,y:200,vx:0,vy:-64,phase:0,sourceKind:0,sourceId:1});
  pool.allocate({mass:2,x:61.5,y:200,vx:0,vy:-38,phase:1,sourceKind:1,sourceId:2});
  pool.retainAfterDeposit(a,0.25,100,1e-6);const by=pool.endpointByState();
  assert.equal(by.CONTACT_WAIT.massH,0.75);assert.equal(by.FLYING.massL,2);
  assert.equal(by.CONTACT_WAIT.count+by.FLYING.count,2);
  pool.pState[a]=9;assert.throws(()=>pool.endpointByState(),/INVALID_LEDGER/);
});
const failed=results.filter(r=>r.status==='FAIL');console.log(JSON.stringify({suite:'p011_endpoint_native',passed:results.length-failed.length,failed:failed.length,results},null,2));
if(failed.length)process.exitCode=1;
