'use strict';
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const project=path.resolve(process.argv[2]),base=path.join(project,'entry/src/main/ets');
const {T266Clock}=require(base+'/diagnostics/t248/T266Clock.ets');
const {TxWorkTrace}=require(base+'/physics/tx/TxWorkTrace.ets');
const {P051DerivedFacadeV2:F}=require(base+'/physics/derived/P051DerivedFacadeV2.ets');
let c=new T266Clock();c.bind('r1',1);c.committed(1,1/60);c.source(2,8,true,2,false);c.committed(1,1/60);assert.equal(JSON.parse(c.summary()).acceptedPhysicsSecondsSinceFirstBirth,1/60);c.committed(0,1);assert.equal(c.committedSubstepCount,2);c.source(1,0,false,2,true);assert.equal(c.sourceRejectedCount,1);assert.equal(c.sourceRequestedSeconds,3);assert.equal(c.sourceAdmittedMass,8);
c.bind('r1',2);assert.equal(c.acceptedSeconds,0);assert.equal(c.firstBirthCommit,-1);assert.equal(c.sourceAdmittedMass,0);
const cfg={isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:192,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'};
const on=new F(cfg),off=new F(cfg),trace=new TxWorkTrace(32);trace.detail=true;let time=0;trace.clock=()=>++time;on.t266Attach(trace,c);off.t266Attach(null,new T266Clock());
const req={revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:1,inputTimeMs:1,x:64.5,y:2,seed:51,requestedH:.3,requestedL:0};
const sr=on.inject(req),control=off.inject(req);assert.equal(sr.ok,control.ok);assert.deepEqual({...sr.original,runId:'OWN_RUN'},{...control.original,runId:'OWN_RUN'});
let land=0;for(let i=0;i<65;i++){const a=on.step(1/30),b=off.step(1/30);assert.equal(a.ok,b.ok);assert.equal(a.original.errorCode,b.original.errorCode);c.committed(a.original.substeps,a.original.substepDt);const aa=on.exportCheckpointCold(),bb=off.exportCheckpointCold();assert.deepEqual(aa.authority.particleState.pMass,bb.authority.particleState.pMass);assert.deepEqual(aa.authority.particleState.pY,bb.authority.particleState.pY);assert.deepEqual(aa.authority.mh,bb.authority.mh);assert.equal(aa.commit,bb.commit);assert.equal(on.t248LandCold(),off.t248LandCold());land+=JSON.parse(on.t248LandCold()).length;}
assert(land>0,'LAND equivalence must cover nonempty LAND');assert.equal(trace.count,32);assert.equal(trace.incomplete,true);assert.equal(trace.depth,0);on.reset();on.t266Attach(trace,c);assert.equal(c.acceptedSeconds,0);assert.equal(c.sourceRequestedSeconds,0);
const ability=fs.readFileSync(base+'/entryability/EntryAbility.ets','utf8');assert(ability.includes("const probePage: string = 'pages/P051DerivedV2Demo';"));const pages=JSON.parse(fs.readFileSync(project+'/entry/src/main/resources/base/profile/main_pages.json'));assert.deepEqual(pages.src,['pages/P051DerivedV2Demo']);
// Real original root with one committed substep and the second planning blocked.
const partial=new F(cfg),partialClock=new T266Clock();partial.t266Attach(null,partialClock);
const {WaterSandErrorCode}=require(base+'/physics/WaterSandField.ets');let calls=0;
const originalPlan=partial.field.planAndCommitTilt;
partial.field.planAndCommitTilt=function(dt){return ++calls===2?WaterSandErrorCode.TILT_PLANNING_BLOCKED:originalPlan.call(this,dt);};
const beforeCommit=partial.t266Commit(),r=partial.step(1/30);
assert.equal(r.ok,false);assert.equal(r.original.substeps,1);assert.equal(r.original.errorCode,WaterSandErrorCode.TILT_PLANNING_BLOCKED);
partialClock.stepEvidence(1,1/30,r.original.substeps,r.original.substepDt,beforeCommit,r.physicalCommit,r.ok,r.original.errorCode);
assert.equal(partialClock.acceptedSeconds,1/60);assert.equal(partialClock.stepRows[6],0);assert.equal(partialClock.stepRows[9],1/60);
console.log(JSON.stringify({revision:'T266_OFFLINE_V2',passed:24,failed:0,scopeOverflow:'INCOMPLETE',runEpochReset:'PASS',partialCommittedCount:'PASS_REAL_ORIGINAL_ROOT',authorityReceiptLandCommitEquivalence:'PASS',route:'PASS',landPageOccurrences:land,device:'NOT_RUN'},null,2));
