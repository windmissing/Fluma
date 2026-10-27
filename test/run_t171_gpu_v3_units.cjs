'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{performance}=require('node:perf_hooks');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.resolve('.codex/local-test/t171-project/entry/src/main/ets/physics');
const {WaterSandField}=require(path.join(base,'WaterSandField.ets')),{defaultP011ParticleParams}=require(path.join(base,'P011ParticleM2Field.ets'));
const selector=require(path.join(base,'T159Selector.ets')),metadata=require(path.join(base,'T159Metadata.ets')),{t171Evidence:evidence}=require(path.join(base,'T171Evidence.ets'));
const {fixtureFor,scheduleFor,t171ConfigId}=require(path.join(base,'T171Protocol.ets')),{T171_FIXTURES}=require(path.join(base,'T171Fixtures.ets'));
const {T159_SOURCE_SHA,T159_SHADER_SHA}=require(path.join(base,'T159BuildIdentity.ets'));
const results=[],measurements=[];let reference;
const test=(name,fn)=>{fn();results.push({name,status:'PASS'});console.log('PASS '+name);};
function command(fixture,extra={}){const c={revision:'T171_RUN_BLOCK_V3',blockId:'T171_OFFLINE',fixtureId:fixture.fixtureId,fixtureSha256:fixture.fixtureSha256,route:'GPU',steps:2,oracleMode:'ON',timingMode:'COLD_AND_STEADY_SPLIT',repeatId:0,mode:'CORRECTNESS',fault:'NONE',queuedUptimeMs:1,output:'offline',...extra};
 c.config=extra.config||{nmax:fixture.nmax,diagnosticOracle:c.oracleMode==='ON',fullDump:c.oracleMode==='ON',sourceManifestSha256:T159_SOURCE_SHA,buildManifestSha256:'a'.repeat(64),configId:t171ConfigId(fixture,c.steps,c.oracleMode,c.fault),buildShaderSha256:T159_SHADER_SHA[0],queryShaderSha256:T159_SHADER_SHA[1],reduceShaderSha256:T159_SHADER_SHA[2]};return c;}

function normalized(v){return JSON.parse(JSON.stringify(v,(k,v)=>k==='physicsTimeMs'?0:k==='runId'?'RUN_ID_SEPARATE':v));}
function pairRows(common){const rows=Object.values(common.pairRows),out=[];for(let i=0;i<rows.length;i+=32){const r=rows.slice(i,i+32);for(const k of [4,5,12,30,31])r[k]=0;out.push(r);}return out;}
test('all four fixtures are cold manifest-bound and original physical inputs remain fixed',()=>{
 const manifest=JSON.parse(fs.readFileSync('.codex/local-test/t171-project/t171-source-manifest.json'));assert.equal(manifest.productionTouched,false);assert.equal(T171_FIXTURES.length,4);
 for(const f of T171_FIXTURES){assert.equal(fixtureFor(command(f)).fixtureId,f.fixtureId);const raw=fs.readFileSync('.codex/local-test/t171-project/entry/src/main/resources/rawfile/t171/'+f.fixtureId+'.json');assert.equal(crypto.createHash('sha256').update(raw).digest('hex'),f.fixtureSha256);assert.equal(f.massPackage,.1);assert.equal(f.column,64);}
});
test('protocol refuses invalid steps, hashes, oracle modes and arbitrary fault plans',()=>{
 const f=T171_FIXTURES[0];for(const patch of [{steps:0},{steps:33},{steps:NaN},{fixtureSha256:'0'.repeat(64)},{mode:'PERFORMANCE',oracleMode:'ON'},{fault:'FAKE_GL_ERROR'},{mode:'CORRECTNESS',fault:'QUARANTINE_GL'}])assert.throws(()=>fixtureFor(command(f,patch)));
});
test('performance plan has 3 warmups per route and six permutations each five blocks',()=>{
 const runs=scheduleFor(command(T171_FIXTURES[0],{mode:'PERFORMANCE',oracleMode:'OFF'}));assert.equal(runs.length,99);
 assert.equal(runs.filter(x=>x.warmup).length,9);const counts=new Map();for(let i=9;i<runs.length;i+=3){const key=runs.slice(i,i+3).map(x=>x.route).join('/');counts.set(key,(counts.get(key)||0)+1);}assert.equal(counts.size,6);assert.ok([...counts.values()].every(x=>x===5));assert.deepEqual(runs,scheduleFor(command(T171_FIXTURES[0],{mode:'PERFORMANCE',oracleMode:'OFF'})));
});
test('retained context ordinals stay monotonic while per-attempt counts reset',()=>{
 metadata.t159ResetMetadata();metadata.t159Role(1);const before=metadata.t159CandidateOrdinal();metadata.t171ResetAttemptMetadata();metadata.t159Role(1);assert.equal(metadata.t159CandidateOrdinal(),before+1);assert.equal(metadata.t171RelativeOrdinal(),1);assert.equal(metadata.t171RoleCounts()[1],1);
});
test('test injection interface is absent from non-test preprocessed binding',()=>{
 const release=fs.readFileSync('.codex/local-test/t171/release-preprocessed.i','utf8');assert.ok(!release.includes('armTestInjectionV3'));assert.ok(!release.includes('testIllegalUpload_'));
});
for(const fixture of T171_FIXTURES)test(fixture.fixtureId+' original public injection, three-route pair/F64/page/state equality',()=>{
 let original;
 for(const route of ['LEGACY_SCAN','T123','GPU']){
  metadata.t159ResetMetadata();evidence.reset();evidence.clock=()=>performance.now();let calls=0;
  selector.t159Arm(route,route==='GPU'?{selectPair(p,radius,w){calls++;const found=w.mergeIndex.find(p,radius);return {status:found?'PAIR':'NO_PAIR',slotA:w.mergeIndex.selectedA,slotB:w.mergeIndex.selectedB,errorCode:'OFFLINE_CPU_STAND_IN'};}}:null);
  try{
   const field=new WaterSandField(128,512),params=defaultP011ParticleParams();params.nmax=fixture.nmax;assert.ok(field.setParticleRoute(params).ok);field.setNaturalObservationRouteId('dual-field-particle-v1');
   const before=field.t171ArchiveState(),receipts=[];
   for(let input=0;input<2;input++){const heavy=new Float64Array(128),light=new Float64Array(128);heavy[64]=(input===0?fixture.nmax:fixture.packages)*.1;
    const receipt=field.injectGestureSand({perColumnHeavy:heavy,perColumnLight:light,sourceId:17,sequence:input+1,inputTime:10+input,x:64.5,y:100,seed:13});assert.ok(receipt);assert.ok(Math.abs(receipt.admittedH-heavy[64])<1e-10,JSON.stringify(receipt));receipts.push(receipt);
   }
   for(let i=0;i<32;i++)assert.ok(field.step(1/60).ok);
   const common=JSON.parse(evidence.exportJson(true)),after=JSON.parse(field.t171ArchiveState());assert.equal(common.overflow,false);assert.ok(common.selectorCalls>=fixture.packages);assert.ok(common.mergeCommitted>=fixture.packages);
   assert.equal(after.poolRawBytes.length,13);const physical={after:normalized(after),pairs:pairRows(common),events:common.eventRawF64Hex,refundRaw:common.refundRawF64Hex,lineage:common.lineageRows,selectorCalls:common.selectorCalls,mergeCommitted:common.mergeCommitted,audits:common.auditCalls,rollbacks:common.rollbackCalls,refunds:common.refundRows,lambdaCalls:common.lambdaCalls};
   if(route==='LEGACY_SCAN')original=physical;else assert.deepEqual(physical,original);
   if(route==='GPU')assert.ok(calls>0);measurements.push({fixture:fixture.fixtureId,route:route==='GPU'?'GPU_INTERFACE_CPU_STAND_IN':route,selectorCalls:common.selectorCalls,mergeCommitted:common.mergeCommitted,audits:common.auditCalls,rollbackCalls:common.rollbackCalls,pages:common.pages,admitted:receipts.map(x=>x.admittedH),steps:32,gpuExecution:'NOT_RUN'});
  }finally{selector.t159Disarm();}
 }
});
test('V3 overlay is reversibly whitelisted and physical merge arithmetic is unchanged',()=>{
 const manifest=JSON.parse(fs.readFileSync('.codex/local-test/t171-project/t171-source-manifest.json'));
 for(const file of new Set(manifest.edits.map(x=>x.file))){let text=fs.readFileSync(path.join(base,'..',file),'utf8');for(const e of manifest.edits.filter(x=>x.file===file).toReversed()){
   assert.equal(crypto.createHash('sha256').update(text).digest('hex'),e.generatedSha256);text=text.replaceAll('\r\n','\n').replace(e.to,e.from).replaceAll('\n',e.eol);assert.equal(crypto.createHash('sha256').update(text).digest('hex'),e.originalSha256);
   // Initial V2 file can have CRLF; the declared pre-image raw SHA is archived separately.
  }}
 const source=fs.readFileSync('entry/src/main/ets/physics/P011ParticleHot.ets','utf8'),generated=fs.readFileSync(path.join(base,'P011ParticleHot.ets'),'utf8');
 const arithmetic=text=>text.replaceAll('\r\n','\n').slice(text.replaceAll('\r\n','\n').indexOf('  const ma = p.pMass[a];'),text.replaceAll('\r\n','\n').indexOf('function mergeOldestHot')).replace('  t159Mutation(p);\n','');
 assert.equal(arithmetic(source),arithmetic(generated));
});
fs.mkdirSync('test/reports/t171',{recursive:true});fs.writeFileSync('test/reports/t171/selftest.json',JSON.stringify({revision:'T171_DEVELOPER_OFFLINE_V3',passed:results.length,failed:0,measurements,results,gpuExecution:'NOT_RUN',faults:'HOST_DOUBLES_AND_SOURCE_ONLY_NOT_DEVICE'},null,2)+'\n');console.log(JSON.stringify({passed:results.length,failed:0}));
