'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P011StrictWriterKernels,P011KernelFrozen,P011KernelRequest}=
  require('../entry/src/main/ets/physics/P011WriterKernels.ets');
const {p011KernelStatus}=require('../entry/src/main/ets/physics/P011WriterRegistry.ets');
let passed=0;function test(name,fn){fn();passed++;console.log('PASS '+name);}
function fixture(kernel){const f=new P011KernelFrozen(3,2),r=new P011KernelRequest();
  f.ownerId=7;f.snapshotSerial=5;f.identity.runId='r';f.identity.attemptId='a';
  f.identity.snapshotSerial=5;f.identity.writerKernelRevision=kernel;
  f.capacity=100;f.hardMax=10;f.eps=1e-9;
  f.waterTop.fill(50);r.kernel=kernel;r.column=1;
  return {f,r,k:new P011StrictWriterKernels(3)};}
test('registry excludes unproved writers and first airborne contact',()=>{
  assert.equal(p011KernelStatus('PERCOLATE','CONTACT_WAIT'),'STRICT_WRITER_UNSUPPORTED');
  assert.equal(p011KernelStatus('LAND_RN_GRAPH_V1','AIRBORNE'),'CONTACT_HOLD_UNSUPPORTED');});
test('settle uses actual RN phase sum, not qControl',()=>{
  const {f,r,k}=fixture('SETTLE_RN_GRAPH_V1');f.suspH[1]=1;f.suspL[1]=1;
  r.originalRequested=.3;r.qGranted=.15;r.actualDebit=.15;r.dt=.01;
  r.heavySpeed=10;r.lightSpeed=25;
  const s=k.preview(f,r);assert.equal(s.decision,'RAW');
  assert.notEqual(s.phaseHeavy+s.phaseLight,r.qGranted);
  const h=s.phaseHeavy,l=s.phaseLight;assert.equal(k.commit(f,s,7,5),'OK');
  assert.equal(f.mh[1],h);assert.equal(f.ml[1],l);
  assert.equal(f.suspH[1],1-h);assert.equal(f.suspL[1],1-l);});
test('top MIX preserves source split and receiver by phase',()=>{
  const {f,r,k}=fixture('TOP_SPLIT_RN_GRAPH_V1');f.mh[1]=.1;f.ml[1]=.2;
  f.surface[1]=.30000000000000004;r.receiver=2;r.qGranted=.1;r.actualDebit=.1;
  const s=k.preview(f,r);assert.equal(s.decision,'RAW');assert.equal(s.patch.branch,'MIX');
  assert.equal(s.patch.changedCount,2);const h=s.phaseHeavy,l=s.phaseLight;
  assert.equal(k.commit(f,s,7,5),'OK');assert.equal(f.mh[2],h);assert.equal(f.ml[2],l);});
test('LAND sub-ULP partial cannot increase bed without pool debit',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');f.poolMass[0]=1;f.poolId[0]=21;
  f.poolState[0]=2;f.poolColumn[0]=1;f.poolPhase[0]=0;
  r.slot=0;r.particleId=21;r.qGranted=2**-54;r.actualDebit=r.qGranted;
  const s=k.preview(f,r);assert.equal(s.decision,'ZERO');assert.equal(s.patch.changedCount,0);
  assert.equal(k.commit(f,s,7,5),'OK');assert.equal(f.poolMass[0],1);assert.equal(f.mh[1],0);});
test('LAND partial/full retains then retires same ID',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');f.poolMass[0]=1;f.poolId[0]=21;
  f.poolState[0]=2;f.poolColumn[0]=1;f.poolPhase[0]=0;
  r.slot=0;r.particleId=21;r.qGranted=2**-53;r.actualDebit=r.qGranted;
  let s=k.preview(f,r);assert.equal(s.decision,'RAW');assert.ok(s.poolRemainder<1);
  assert.equal(k.commit(f,s,7,5),'OK');assert.equal(f.poolId[0],21);
  r.qGranted=f.poolMass[0];r.actualDebit=r.qGranted;s=k.preview(f,r);
  assert.equal(s.decision,'RAW');assert.equal(s.poolRetired,true);
  assert.equal(k.commit(f,s,7,5),'OK');assert.equal(f.poolState[0],0);
  assert.equal(f.poolId[0],21);});
test('owner mismatch blocks commit before field write',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');f.poolMass[0]=1;f.poolId[0]=21;
  f.poolState[0]=2;f.poolColumn[0]=1;r.slot=0;r.particleId=21;
  r.qGranted=.5;r.actualDebit=.5;const s=k.preview(f,r);assert.equal(s.decision,'RAW');
  assert.equal(k.commit(f,s,8,5),'IDENTITY_MISMATCH');assert.equal(f.mh[1],0);});
test('unsupported first contact does not mutate anything',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');r.particleState='AIRBORNE';
  const s=k.preview(f,r);assert.equal(s.decision,'UNSUPPORTED');
  assert.equal(s.reason,'CONTACT_HOLD_UNSUPPORTED');assert.equal(s.patch.changedCount,0);});
console.log(`T135 ${passed}/${passed} PASS`);
