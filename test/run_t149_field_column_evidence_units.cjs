'use strict';
const fs=require('node:fs'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const {P011FieldColumnEvidenceV2}=require('../entry/src/main/ets/physics/P011FieldColumnEvidenceV2.ets');
const {P011TiltPreviewTrace}=require('../entry/src/main/ets/physics/P011TiltPreviewTrace.ets');
let passed=0;function test(name,run){run();passed++;console.log('PASS '+name);}
function field(){const f=new WaterSandField(128,512);assert.equal(f.setParticleRoute(defaultP011ParticleParams()).ok,true);
  f.setTiltPreviewTraceSink(()=>{});return f;}
function fail(f){const b=f.particleBinding;const x=b.beginCollect(1/60,0,1,1,0,0,0);
  assert.equal(x,3);assert.equal(b.collectFailure.predicate,'FIELD_COLUMN');
  return b.work.fieldColumnEvidenceV2.failure;}
test('all eight nonfinite causes retain their own mask bits',()=>{
  for(const [i,key] of ['waterTop','bh','bl','mh','ml','suspH','suspL','sandTopScratch'].entries()){
    const f=field();f[key][4]=Infinity;const e=fail(f);assert.equal(e.column,4);
    assert.ok((e.badMask&(1<<i))!==0,`${key} mask`);
    assert.equal(e.rawBits[i],'7ff0000000000000');
  }
});
test('waterTop below audit EPS and six strict negative bits retain raw signs; -0 passes',()=>{
  for(const [i,key] of ['waterTop','bh','bl','mh','ml','suspH','suspL'].entries()){
    // T166 approved waterTop-only predicate revision; the raw sign mask is unchanged.
    const f=field();f[key][4]=i===0?-2e-9:-Number.MIN_VALUE;const e=fail(f);
    assert.ok((e.badMask&(1<<(8+i)))!==0,`${key} negative`);
    assert.equal(e.rawBits[i],i===0?'be212e0be826d695':'8000000000000001');
  }
  const f=field();f.bh[4]=-0;assert.equal(f.particleBinding.beginCollect(1/60,0,1,1,0,0,0),0);
  f.particleBinding.rollback();
});
test('finite negative surface does not fail the original predicate',()=>{
  const f=field();f.sandTopScratch[4]=-1;
  assert.equal(f.particleBinding.beginCollect(1/60,0,1,1,0,0,0),0);
  f.particleBinding.rollback();
});
test('[-EPS,0) remains audit-tolerated but Hot rejected with exact negative bit',()=>{
  const f=field();f.bh[4]=-5e-10;
  assert.equal(f.audit(false),null);
  const e=fail(f);assert.equal(e.column,4);
  assert.ok(e.badMask&(1<<9));
});
test('finite components with left-associated overflow set bit15',()=>{
  const f=field();f.bh[4]=1e308;f.bl[4]=1e308;
  const e=fail(f);assert.equal(e.badMask,1<<15);
  assert.equal(e.sumBits[0],'7ff0000000000000');
});
test('first column wins; same-column multi-cause mask survives',()=>{
  const f=field();f.waterTop[9]=-1;f.bh[4]=-1;f.suspH[4]=Infinity;
  const e=fail(f);assert.equal(e.column,4);
  assert.ok(e.badMask&(1<<5));assert.ok(e.badMask&(1<<9));
});
test('NaN payload preserved from typed-array buffer and frozen before cleanup',()=>{
  const f=field(),v=new DataView(f.bh.buffer,f.bh.byteOffset);
  v.setUint32(4*8,0x56789abc,true);v.setUint32(4*8+4,0x7ff81234,true);
  const e=fail(f);assert.equal(e.rawBits[1],'7ff8123456789abc');
  assert.equal(e.poolCanonicalBytesHex.length,13);
  assert.equal(e.poolStorageValid,true);
  assert.ok(e.poolCanonicalBytesHex.every((h,i)=>h.length===2*e.poolCanonicalByteLengths[i]));
  f.bh[4]=0;f.particleBinding.rollback();
  assert.equal(e.rawBits[1],'7ff8123456789abc');
});
test('window stores last legal and first illegal bytes, then stops replacing first',()=>{
  const f=field(),b=f.particleBinding,ev=b.work.fieldColumnEvidenceV2;
  f.observeFieldColumnV2('LEGAL','INPUT');f.bh[3]=-1e-10;
  f.observeFieldColumnV2('BAD','INPUT');const first=ev.exportEvidence();
  assert.equal(first.windowStatus,'LEGAL_TO_INVALID');
  assert.equal(first.lastLegalBytesHex.length,8192*2);
  assert.equal(first.firstIllegalBytesHex.length,8192*2);
  f.bh[3]=-2;f.observeFieldColumnV2('LATER','INPUT');
  assert.equal(ev.exportEvidence().firstIllegalBytesHex,first.firstIllegalBytesHex);
});
test('already invalid first stage is PREEXISTING_IN_WINDOW and ring is bounded',()=>{
  const f=field(),ev=f.particleBinding.work.fieldColumnEvidenceV2;
  f.bh[2]=-1e-10;for(let i=0;i<35;i++)f.observeFieldColumnV2('S'+i,'TEST');
  const out=ev.exportEvidence();assert.equal(out.windowStatus,'PREEXISTING_IN_WINDOW');
  assert.equal(out.stages.length,32);assert.equal(out.dropped,3);
  assert.equal(out.stages[0].stage,'S3');
});
test('M2 and trace preserve first Hot evidence and explicit measurement flags',()=>{
  const f=field(),trace=new P011TiltPreviewTrace();
  f.tiltTracePending=[trace];f.planningPassService.trace=trace;f.bh[4]=-1e-10;
  fail(f);f.recordCollectFirstFailure('PARTICLE_BEGIN',2);
  assert.equal(trace.firstFailure.driftMeasured,false);
  assert.equal(trace.firstFailure.rollbackExecuted,false);
  assert.equal(trace.fieldColumnEvidenceV2.failure.column,4);
  assert.equal(trace.fieldColumnEvidenceV2.failure.surfaceIsSandTopScratch,true);
  const frozen=JSON.stringify(trace.fieldColumnEvidenceV2);
  f.bh[4]=0;f.recordCollectFirstFailure('CHANNEL',99);
  assert.equal(JSON.stringify(trace.fieldColumnEvidenceV2),frozen);
});
test('audit drift has measured provenance rather than FIELD_COLUMN default zero',()=>{
  const f=field(),trace=new P011TiltPreviewTrace();
  f.tiltTracePending=[trace];f.planningPassService.trace=trace;
  f.sourceInjectedHeavy+=4;f.audit(true);
  assert.equal(trace.firstFailure.driftMeasured,true);
  assert.equal(trace.firstFailure.driftSource,'AUDIT_GET_DRIFT');
  assert.equal(trace.firstFailure.rollbackExecuted,false);
});
test('diagnostics closed allocates no evidence; enabled probe is read-only',()=>{
  const x=new WaterSandField(128,512),y=new WaterSandField(128,512);
  for(const f of [x,y])assert.equal(f.setParticleRoute(defaultP011ParticleParams()).ok,true);
  assert.equal(x.particleBinding.work.fieldColumnEvidenceV2,null);
  y.setTiltPreviewTraceSink(()=>{});assert.ok(y.particleBinding.work.fieldColumnEvidenceV2);
  y.observeFieldColumnV2('READ','TEST');
  for(const k of ['waterTop','bh','bl','mh','ml','suspH','suspL','sandTopScratch'])
    assert.deepEqual(x[k],y[k]);
  assert.deepEqual(x.particleBinding.pool.snapshot(),y.particleBinding.pool.snapshot());
});
test('full evidence survives bounded transport fragments without bit loss',()=>{
  const f=field(),ev=f.particleBinding.work.fieldColumnEvidenceV2;
  f.observeFieldColumnV2('LEGAL','TEST');f.bh[4]=-5e-10;
  fail(f);const json=JSON.stringify(ev.exportEvidence());
  const fragments=[];for(let i=0;i<json.length;i+=1024)fragments.push(json.slice(i,i+1024));
  assert.ok(fragments.length>2);assert.ok(fragments.every(x=>x.length<=1024));
  const decoded=JSON.parse(fragments.join(''));
  assert.equal(decoded.failure.rawBits[1],ev.failure.rawBits[1]);
  assert.equal(decoded.firstIllegalBytesHex.length,8192*2);
});
console.log(`T149 ${passed}/${passed} PASS`);
