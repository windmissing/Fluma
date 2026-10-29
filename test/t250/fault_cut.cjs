'use strict';
const fs=require('fs'),path=require('path'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.join(path.resolve(process.argv[2]),'entry/src/main/ets/physics');
const {runTxGpuFixture}=require(path.join(base,'tx/TxGpuFixture.ets'));
const {P011_HOT_STALE_HANDLE}=require(path.join(base,'P011ParticleHot.ets'));
const config={nmax:192,shaderDirectory:'HOST_NOT_GPU',runId:'cut6',configId:'CUT6',sourceManifestSha256:'a'.repeat(64),buildManifestSha256:'b'.repeat(64),buildShaderSha256:'c'.repeat(64),queryShaderSha256:'d'.repeat(64),reduceShaderSha256:'e'.repeat(64),diagnosticOracle:false,fullDump:false,backend:'LEGACY_SCAN',policy:'TX_EXACT_STRUCTURE_V1',traceCapacity:4096,variant:'STRICT_O2'};
const cases=[];
for(const arm of ['COMMON_FREEZE_OFF','COMMON_FREEZE_ON']){
 const c={...config,freezeArm:arm};
 const control=runTxGpuFixture(c,'WET_192_64',1,null,()=>0);
 const plan={fixtureId:'WET_192_64',buildSHA:c.buildManifestSha256,backend:c.backend,freezeArm:arm,PID:100,rootId:1,cutId:6,edge:'BEFORE',ordinal:1};
 const result=runTxGpuFixture(c,'WET_192_64',1,null,()=>0,null,plan,100);
 const fault=JSON.parse(result.faultPlan),rows=fault.rows.map(JSON.parse);
 assert.equal(control.physicalStatus,'COMMITTED');assert.equal(result.physicalStatus,'FAULT');assert.equal(fault.hit,true);
 assert.equal(rows.length,2);assert.equal(rows[1].firstCode,P011_HOT_STALE_HANDLE);
 assert.equal(rows[0].raw,rows[1].raw,'transient input restored before original rollback');
 const root=JSON.parse(result.rootRows[0]),before=JSON.parse(root.before),after=JSON.parse(root.after);
 // Original source retries lambda after a failed attempt. Root may commit its
 // successful retry; no assertion may erase that accepted prefix.
 assert(root.receipt.admittedH>0&&root.receipt.admittedH<root.offered);
 assert.equal(after.source.admittedH-before.source.admittedH,root.receipt.admittedH);
 assert.equal(after.source.count-before.source.count,1);
 assert(Math.abs(after.drift.heavy)<1e-10);
 const repeat=runTxGpuFixture(c,'WET_192_64',1,null,()=>0);
 assert.equal(repeat.physicalStatus,'COMMITTED');assert.deepEqual(JSON.parse(repeat.after).poolRawBytes,JSON.parse(control.after).poolRawBytes);
 cases.push({arm,result,firstCode:rows[1].firstCode,controlPhysicalStatus:control.physicalStatus,repeatPhysicalStatus:repeat.physicalStatus});
}
console.log(JSON.stringify({revision:'T250_CUT6_ORIGINAL_CHECK_V1',passed:24,failed:0,cases,coverage:'cut6 BEFORE only; other39 edges NOT_IMPLEMENTED',device:'NOT_RUN',gpu:'NOT_RUN'},null,2));
