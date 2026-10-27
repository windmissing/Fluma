'use strict';
const fs=require('fs'),assert=require('assert/strict'),path=require('path');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P051EmptySandDemo:D}=require('../entry/src/main/ets/physics/P051EmptySandDemo.ets');
const results=[];function test(name,f){try{f();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:e.stack});}}
function inject(d,code){let calls=0;d.field.step=()=>{calls++;d.field.lastError=code;return {ok:false};};return ()=>calls;}
for(const code of [1,2,3,4,5,6,null])test('actual failure raw '+code,()=>{
 const d=new D(),calls=inject(d,code),before=d.field.getRenderPhysicsCommitId();
 // Fail if the new diagnostic path tries to obtain a physical snapshot/audit.
 for(const k of ['snapshot','updateSandTop','audit','copyPublishedRenderStateInto'])d.field[k]=()=>{throw Error('FORBIDDEN_DIAGNOSTIC_'+k);};
 assert.equal(d.tick(1e9,1/60),false);const b=d.exportPhysicsFailureV2();assert.equal(b.firstFailure.rawCode,code);
 assert.equal(b.firstFailure.beforeCommit,before);assert.equal(b.firstFailure.afterCommit,before);assert.equal(b.firstFailure.stage,null);assert.equal(b.firstFailure.m2Code,null);assert.equal(b.firstFailure.acceptedTiltDeg,null);
 const id=b.firstFailure.firstFailureId;for(let i=0;i<3;i++){assert.equal(d.tick(2e9+i,1/60),false);d.stop('page-physics-failed');}
 assert.equal(calls(),1);assert.equal(d.exportPhysicsFailureV2().firstFailure.repeatCount,3);assert.equal(d.exportPhysicsFailureV2().firstFailure.firstFailureId,id);
 b.firstFailure.rawCode=999;assert.equal(d.exportPhysicsFailureV2().firstFailure.rawCode,code);
});
test('reset isolates old identity and last good',()=>{const d=new D();d.recordDiagnosticPublication('prior-publication',12,8,0);inject(d,5);d.tick(1e9,1/60);const old=d.exportPhysicsFailureV2();assert.equal(old.firstFailure.lastGoodCommit,12);d.reset();assert.equal(d.exportPhysicsFailureV2().firstFailure,null);assert.equal(d.exportPhysicsFailureV2(true).firstFailure.firstFailureId,old.firstFailure.firstFailureId);inject(d,2);d.tick(2e9,0);const n=d.exportPhysicsFailureV2();assert.notEqual(n.firstFailure.runId,old.firstFailure.runId);assert.ok(n.firstFailure.modelGeneration>old.firstFailure.modelGeneration);assert.equal(n.firstFailure.lastGoodCommit,null);assert.equal(n.firstFailure.dtUnit,'seconds');assert.equal(n.firstFailure.frameTimeUnit,'nanoseconds');});
test('bounded lifecycle and two process identities',()=>{const a=new D(),b=new D();inject(a,3);inject(b,5);a.tick(0,0);b.tick(0,0);for(let i=0;i<100;i++)a.stop('page-physics-failed');const x=a.exportPhysicsFailureV2();assert.equal(x.lifecycle.length,8);assert.ok(x.dropped>0);assert.equal(x.firstFailure.repeatCount,100);assert.notEqual(x.firstFailure.firstFailureId,b.exportPhysicsFailureV2().firstFailure.firstFailureId);assert.ok(x.lifecycle.every(r=>r.firstFailureId===x.firstFailure.firstFailureId));});
test('successful admission before failed step retained',()=>{const d=new D();d.start(64,150);d.tick(0,0);inject(d,4);d.tick(1e9/60,1/60);const f=d.exportPhysicsFailureV2().firstFailure;assert.equal(f.sourceSequenceAfter,f.sourceSequenceBefore+1);assert.equal(f.sourceEpochAfter,f.sourceEpochBefore+1);assert.equal(f.externalAdmissionCountAfter,f.externalAdmissionCountBefore+1);assert.ok(d.field.getTotals().heavy>0);});
test('stale trace never relabeled current',()=>{const d=new D();d.field.tiltTraceLast={runId:d.field.getP011RunId(),commitBefore:0,firstFailure:{origin:'OLD',m2Code:0}};inject(d,5);d.tick(0,0);assert.equal(d.exportPhysicsFailureV2().firstFailure.stage,null);});
const r={suite:'T185',device:'NOT_RUN',passed:results.filter(x=>x.status==='PASS').length,failed:results.filter(x=>x.status==='FAIL').length,results};console.log(JSON.stringify(r,null,2));if(r.failed)process.exitCode=1;
