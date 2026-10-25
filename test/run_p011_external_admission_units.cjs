'use strict';
// T39: real WaterSandField admissions, no device and no alternate physics.
const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField,defaultWaterSandParams}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {P011NaturalObservationProbe}=require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
const {P011ExternalAdmissions}=require('../entry/src/main/ets/physics/P011ExternalAdmissions.ets');
const results=[],writerContexts={};
function test(name,fn){try{fn();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:e.stack});}}
function field(){const f=new WaterSandField(128,512),p=defaultWaterSandParams();
  p.heavyDiffusion=0;p.lightDiffusion=0;p.angleDeg=4;f.configure(p);
  f.initialize({waterRegions:[{startCol:0,endColInclusive:127,bottom:300,height:160}],
    heavyDepositedRegions:[{startCol:0,endColInclusive:127,bottom:0,height:100}],
    lightDepositedRegions:[{startCol:0,endColInclusive:127,bottom:100,height:100}],
    heavySuspendedRegions:[],lightSuspendedRegions:[]});
  // Bind the clock at run start, before any admitted source or substep.
  f.setEndpointSampling({clockId:f.getP011RunId()+':startup-elapsed-v1',nowElapsedMs:()=>1000},
    {occurrenceId:'admission-test',commandSequence:f.p011InputSequence,commandPhaseId:f.commandPhaseId,segmentId:'s'});
  return f;}
function request(sequence=1){const h=new Float64Array(128),l=new Float64Array(128);
  h[60]=.1;h[61]=.1;l[60]=.1;l[62]=.1;
  return {perColumnHeavy:h,perColumnLight:l,sourceId:7,sequence,inputTime:1000+sequence,x:500,y:240,seed:17};}
function admit(f,seq=1){const r=f.injectGestureSand(request(seq));assert.ok(r&&r.rejectedReason===null&&r.admittedH+r.admittedL>0);return r;}
function context(f){const records=[];f.setNaturalObservationProbe(new P011NaturalObservationProbe(128,r=>records.push(r)));
  assert.equal(f.step(1/60).ok,true);return records.at(-1).context;}

test('multi-column and two-phase transaction counts once with independent raw record',()=>{
  const f=field(),raw=[];f.setExternalAdmissionEventSink(r=>raw.push(r));const r=admit(f);
  const e=f.getExternalAdmissionEvidence();assert.equal(e.count,1);assert.equal(f.sourceEpoch,1);
  assert.equal(e.admittedH,r.admittedH);assert.equal(e.admittedL,r.admittedL);
  assert.equal(raw.length,1);assert.equal(raw[0].admissionTransactionId,1);
  assert.equal(raw[0].sourceId,7);assert.equal(raw[0].runId,f.getP011RunId());
  assert.equal(e.rawHistoryStatus,'STREAMED');writerContexts.healthy=context(f);
  assert.equal(writerContexts.healthy.externalAdmissionCount,1);
  assert.equal(writerContexts.healthy.externalAdmissionEvidence.count,1);
});
test('same source and request identity can be distinct successful transactions; replay does not recount',()=>{
  const f=field(),raw=[];f.setExternalAdmissionEventSink(r=>raw.push(r));admit(f);admit(f);
  assert.equal(f.getExternalAdmissionEvidence().count,2);assert.equal(f.sourceEpoch,2);
  assert.deepEqual(raw.map(r=>r.admissionTransactionId),[1,2]);
  f.externalAdmissions.exportLast();assert.equal(raw.length,3);
  assert.equal(raw[1].admissionTransactionId,raw[2].admissionTransactionId);
  assert.equal(f.getExternalAdmissionEvidence().count,2);
  raw[0].cumulativeCount=999;assert.equal(f.getExternalAdmissionEvidence().count,2);
});
test('zero request, dry rejection and exhausted rollback do not publish admissions',()=>{
  const f=field(),raw=[];f.setExternalAdmissionEventSink(r=>raw.push(r));
  const zero=request();zero.perColumnHeavy.fill(0);zero.perColumnLight.fill(0);
  assert.equal(f.injectGestureSand(zero),null);
  const before=f.snapshot();f.gestureInjectionAudit=()=>false;
  assert.equal(f.injectGestureSand(request()).rejectedReason,'CAPACITY_BLOCKED');
  assert.deepEqual(f.snapshot().waterTop,before.waterTop);assert.deepEqual(f.snapshot().suspH,before.suspH);
  assert.equal(f.sourceEpoch,0);assert.equal(f.getExternalAdmissionEvidence().count,0);
  f.waterTop.fill(200);assert.equal(f.injectGestureSand(request()).rejectedReason,'NO_WATER');assert.equal(raw.length,0);
});
test('failed lambda trial followed by success counts only the final admitted receipt',()=>{
  const f=field();const audit=f.gestureInjectionAudit.bind(f);let calls=0;
  f.gestureInjectionAudit=()=>++calls===1?false:audit();const r=admit(f);
  assert.ok(calls>1);assert.equal(f.getExternalAdmissionEvidence().count,1);
  assert.equal(f.getExternalAdmissionEvidence().admittedH,r.admittedH);
  assert.equal(f.sourceInjectedHeavy,r.admittedH);
});
test('shake, step, invert, export and render snapshot reads do not count external sources',()=>{
  const f=field();admit(f);f.setShakeInput(1);assert.equal(f.step(1/60).ok,true);
  f.invert180();f.snapshot();f.snapshot();f.externalAdmissions.exportLast();
  assert.equal(f.getExternalAdmissionEvidence().count,1);assert.equal(f.sourceEpoch,1);
});
test('scene reset clears independent counts and masses with the source epoch',()=>{
  const f=field();admit(f);const run=f.getP011RunId();f.reset();
  assert.notEqual(f.getP011RunId(),run);assert.equal(f.sourceEpoch,0);
  assert.equal(f.sourceInjectedHeavy,0);assert.equal(f.getExternalAdmissionEvidence().count,0);
  assert.equal(f.getExternalAdmissionEvidence().lastRecord,null);
});
test('restore checkpoint restores initial masses, source amounts, epoch and receipt count as a group',()=>{
  const f=field();admit(f);admit(f,2);const snap=f.snapshot(),g=field();admit(g);
  g.restoreFromSnapshot(snap);assert.equal(g.sourceEpoch,2);assert.equal(g.getExternalAdmissionEvidence().count,2);
  assert.equal(g.sourceInjectedHeavy,f.sourceInjectedHeavy);assert.equal(g.initialHeavy,f.initialHeavy);
  assert.equal(g.getExternalAdmissionEvidence().rawHistoryStatus,'SUMMARY_ONLY');
  snap.externalSourceState.admissions.lastRecord.admittedH=900;
  assert.notEqual(g.getExternalAdmissionEvidence().lastRecord.admittedH,900);
  admit(g,3);assert.equal(g.getExternalAdmissionEvidence().count,3);
});
test('legacy restore keeps count unknown, including later admissions, until an explicit scene reset',()=>{
  const f=field();admit(f);const snap=f.snapshot();delete snap.externalSourceState;
  const g=field();g.restoreFromSnapshot(snap);assert.equal(g.getExternalAdmissionEvidence().count,null);
  admit(g);assert.equal(g.getExternalAdmissionEvidence().count,null);
  writerContexts.legacy=context(g);assert.equal(writerContexts.legacy.externalAdmissionCount,null);
  g.reset();assert.equal(g.getExternalAdmissionEvidence().count,0);
});
test('invalid source checkpoint is rejected before field mutation',()=>{
  const f=field(),g=field();admit(f);const snap=f.snapshot();snap.externalSourceState.sourceEpoch=42;
  snap.waterTop.fill(1);const before=g.snapshot();
  assert.throws(()=>g.restoreFromSnapshot(snap),/MISMATCH/);
  assert.deepEqual(g.snapshot().waterTop,before.waterTop);assert.equal(g.getExternalAdmissionEvidence().count,0);
});
test('epoch corruption is visible in the real writer: count is not copied from epoch',()=>{
  const f=field();admit(f);f.sourceEpoch=2;writerContexts.epochFault=context(f);
  assert.equal(writerContexts.epochFault.externalAdmissionCount,1);
  assert.equal(writerContexts.epochFault.sourceEpoch,2);
});
test('counter loss and duplicate publication corruption reach the writer unchanged for checker rejection',()=>{
  for(const [name,count] of [['lost',0],['duplicate',2]]){
    const f=field();admit(f);f.externalAdmissions.evidence.count=count;
    writerContexts[name]=context(f);assert.equal(writerContexts[name].externalAdmissionCount,count);
    assert.equal(writerContexts[name].sourceEpoch,1);
  }
});
test('export failure is disclosed and cannot undo or double-count a successful transaction',()=>{
  const f=field();f.setExternalAdmissionEventSink(()=>{throw new Error('EXPORT_FAILED');});admit(f);
  const e=f.getExternalAdmissionEvidence();assert.equal(e.count,1);assert.equal(f.sourceEpoch,1);
  assert.equal(e.exportFailures,1);assert.equal(e.rawHistoryStatus,'INCOMPLETE');
});
test('prepared record is single-use and source overflow rolls back the field before publication',()=>{
  const ledger=new P011ExternalAdmissions(),r=ledger.prepare('r',1,1,1,.1,0);
  ledger.commit(r);assert.throws(()=>ledger.commit(r),/DUPLICATE/);assert.equal(ledger.count,1);
  const f=field();f.sourceEpoch=Number.MAX_SAFE_INTEGER;const before=f.snapshot();
  assert.throws(()=>f.injectGestureSand(request()),/OVERFLOW/);
  assert.deepEqual(f.snapshot().waterTop,before.waterTop);assert.equal(f.getExternalAdmissionEvidence().count,0);
});
const failed=results.filter(r=>r.status==='FAIL');
console.log(JSON.stringify({suite:'p011_external_admissions',passed:results.length-failed.length,
  failed:failed.length,results,writerContexts},null,2));
if(failed.length)process.exitCode=1;
