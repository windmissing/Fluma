'use strict';
const fs=require('node:fs'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const results=[],reconstructions=[];
function test(name,fn){fn();results.push({name,status:'PASS'});console.log('PASS '+name);}
function dry(on=false){const f=new WaterSandField(128,256);
 assert.ok(f.initializeDryParticleDemo({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,particleParams:defaultP011ParticleParams()}).ok);
 if(on)f.setTiltPreviewTraceSink(()=>{});return f;}
function begin(f){return f.particleBinding.beginCollect(1/60,0,1,1,f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch);}
function bits(n){const b=Buffer.alloc(8);b.writeDoubleBE(n);return b.toString('hex');}
function bytes(f){const keys=['waterTop','bh','bl','mh','ml','suspH','suspL','sandTopScratch'];
 return {field:keys.map(k=>Buffer.from(f[k].buffer,f[k].byteOffset,f[k].byteLength).toString('hex')),pool:f.particleBinding.pool.snapshot(),source:[f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch],budget:[f.totalMotionBudgetRemaining,f.totalMotionBudgetUsed,[...f.substepMotionBudgetRemaining],[...f.substepMotionBudgetUsed]],commit:f.renderPhysicsCommitId};}
for(const [arm,x] of [['C1',.44466802978515624],['C2',.4449276657104492]])test(arm+' credit/debit reconstruction retains -2^-55 through Hot and audit(true)',()=>{
 const masses=[.1,.1,.1,.1,x-4*.1];let cache=0;for(const mass of masses)cache+=mass;assert.equal(cache,x);
 for(const mass of masses)cache-=mass;assert.equal(bits(cache),'bc80000000000000');
 const f=dry(true),p=f.particleBinding.pool;
 // Reconstruct post-crossing state: all five live masses now in column 49;
 // source-column cache retains the same sequential subtraction residual.
 for(const mass of masses)assert.ok(p.allocate({mass,x:49.5,y:100,vx:0,vy:0,phase:0,sourceKind:0,sourceId:51})>=0);
 p.colParticleMassH[48]=cache;f.waterTop[48]=p.particleMassAt(48);f.waterTop[49]=p.particleMassAt(49);
 f.sourceInjectedHeavy=x;f.sourceEpoch=1;
 const before=bytes(f);assert.equal(f.audit(true),null);assert.equal(begin(f),0);assert.equal(f.particleBinding.work.beginFailurePredicate,'');
 assert.equal(bits(f.waterTop[48]),'bc80000000000000');assert.equal(bits(p.colParticleMassH[48]),'bc80000000000000');
 assert.equal(f.particleBinding.work.fieldColumnEvidenceV2.failure,null);
 f.particleBinding.rollback();assert.equal(f.particleBinding.work.active,0);assert.deepEqual(bytes(f),before);
 reconstructions.push({arm,x,masses,residual:cache,residualBits:bits(cache),hot:'OK',auditTrue:'OK'});
});
test('waterTop exact -EPS boundary, signed zero and finite interior agree with audit(true)',()=>{
 for(const value of [-1e-9,-5e-10,-(2**-55),-Number.MIN_VALUE,-0,0]){
  const f=dry();f.waterTop[48]=value;assert.equal(f.audit(true),null,String(value));assert.equal(begin(f),0,String(value));
  assert.equal(bits(f.waterTop[48]),bits(value));f.particleBinding.rollback();
 }
});
test('below -EPS and nonfinite waterTop rejected; exact first-column evidence retained',()=>{
 for(const value of [-1.0000000000000003e-9,-2e-9,-1,NaN,Infinity,-Infinity]){
  const f=dry(true);f.waterTop[48]=value;const before=bytes(f);assert.notEqual(f.audit(true),null);
  assert.equal(begin(f),3);assert.equal(f.particleBinding.collectFailure.predicate,'FIELD_COLUMN');
  const ev=f.particleBinding.work.fieldColumnEvidenceV2.failure;assert.equal(ev.column,48);assert.equal(ev.rawBits[0],bits(value));
  assert.ok(ev.badMask&(Number.isFinite(value)?1<<8:1));assert.deepEqual(bytes(f),before);
 }
});
test('six other components retain exact rejection even at -MIN_VALUE',()=>{
 for(const key of ['bh','bl','mh','ml','suspH','suspL']){
  const f=dry(true);f[key][4]=-Number.MIN_VALUE;const before=bytes(f);assert.equal(begin(f),3,key);
  assert.equal(f.particleBinding.collectFailure.failureIndex,4);assert.deepEqual(bytes(f),before);
 }
});
test('raw sign observer still captures tolerated residue without inventing Hot failure',()=>{
 const f=dry(true),ev=f.particleBinding.work.fieldColumnEvidenceV2;f.observeFieldColumnV2('BEFORE','TEST');
 f.waterTop[48]=-(2**-55);f.observeFieldColumnV2('RESIDUE','TEST');const before=bytes(f);
 assert.equal(begin(f),0);f.particleBinding.rollback();const a=ev.exportEvidence();assert.deepEqual(bytes(f),before);
 assert.equal(a.status,'NO_FAILURE');assert.equal(a.failure,null);assert.equal(a.windowStatus,'LEGAL_TO_INVALID');
 assert.equal(a.stages.at(-1).firstInvalidColumn,48);assert.equal(a.firstIllegalBytesHex.length,16384);
 assert.equal(Buffer.from(a.firstIllegalBytesHex,'hex').readDoubleLE(48*8),-(2**-55));
});
test('export order prioritizes failure/stages; full fragments and first latch unchanged',()=>{
 const f=dry(true),ev=f.particleBinding.work.fieldColumnEvidenceV2;f.observeFieldColumnV2('BEFORE','TEST');f.waterTop[48]=-2e-9;begin(f);
 const a=ev.exportEvidence(),s=JSON.stringify(a),keys=Object.keys(a);assert.ok(keys.indexOf('failure')<keys.indexOf('lastLegalBytesHex'));assert.ok(keys.indexOf('stages')<keys.indexOf('lastLegalBytesHex'));
 assert.equal(a.lastLegalBytesHex.length,16384);assert.equal(a.firstIllegalBytesHex.length,16384);assert.equal(a.failure.poolCanonicalBytesHex.length,13);
 const fragments=[];for(let i=0;i<s.length;i+=2048)fragments.push(s.slice(i,i+2048));assert.equal(JSON.stringify(JSON.parse(fragments.join(''))),s);
 f.waterTop[48]=0;f.bh[2]=NaN;begin(f);assert.equal(ev.failure,a.failure);assert.equal(ev.failure.column,48);
});
test('enabled versus disabled observation leaves identical source, arrays, pool, budgets and commit',()=>{
 const x=dry(),y=dry(true);for(const f of [x,y])f.waterTop[48]=-(2**-55);
 y.observeFieldColumnV2('RAW_RESIDUE','TEST');assert.deepEqual(bytes(x),bytes(y));
 for(const f of [x,y]){assert.equal(begin(f),0);f.particleBinding.rollback();assert.equal(f.particleBinding.work.active,0);assert.equal(f.audit(true),null);}
 assert.deepEqual(bytes(x),bytes(y));
});
test('J3 source constraints and existing audit EPS remain explicit',()=>{
 const hot=fs.readFileSync('entry/src/main/ets/physics/P011ParticleHot.ets','utf8');
 const field=fs.readFileSync('entry/src/main/ets/physics/WaterSandField.ets','utf8');
 assert.match(hot,/const WATER_TOP_AUDIT_EPS = 1e-9;/);assert.match(field,/const EPS = 1e-9;/);
 assert.match(field,/this.waterTop\[c\] < -EPS/);assert.match(hot,/f.waterTop\[col\] < -WATER_TOP_AUDIT_EPS/);
});
fs.mkdirSync('test/reports/t166',{recursive:true});fs.writeFileSync('test/reports/t166/selftest.json',JSON.stringify({revision:'T166_F1_SELFTEST_V1',passed:results.length,failed:0,device:'NOT_RUN',reconstructions,results},null,2)+'\n');
console.log(JSON.stringify({passed:results.length,failed:0}));
