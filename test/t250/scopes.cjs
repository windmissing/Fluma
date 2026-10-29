'use strict';
const fs=require('fs'),path=require('path'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.join(path.resolve(process.argv[2]),'entry/src/main/ets/physics');
const {runTxGpuFixture}=require(path.join(base,'tx/TxGpuFixture.ets'));
const c={nmax:192,shaderDirectory:'HOST_NOT_GPU',runId:'scope',configId:'SCOPES',sourceManifestSha256:'a'.repeat(64),buildManifestSha256:'b'.repeat(64),buildShaderSha256:'c'.repeat(64),queryShaderSha256:'d'.repeat(64),reduceShaderSha256:'e'.repeat(64),diagnosticOracle:false,fullDump:false,backend:'LEGACY_SCAN',policy:'TX_EXACT_STRUCTURE_V1',traceCapacity:4096,variant:'STRICT_O2'};
let clock=0;const on=runTxGpuFixture({...c,scopeTraceEnabled:true},'WET_192_64',1,null,()=>++clock);
const off=runTxGpuFixture({...c,scopeTraceEnabled:false},'WET_192_64',1,null,()=>++clock);
assert.equal(on.physicalStatus,'COMMITTED');assert.equal(off.physicalStatus,'COMMITTED');
const a=JSON.parse(on.after),b=JSON.parse(off.after);
for(const key of ['poolRawBytes','sourceBudgetRaw','localBudgetRaw','commit'])assert.deepEqual(a[key],b[key],key);
const trace=JSON.parse(on.scopeArchive),disabled=JSON.parse(off.scopeArchive);
assert.equal(trace.depth,0);assert(trace.maxDepth>1);assert(trace.rowCount>0&&trace.rowCount<=trace.capacity);
assert.equal(disabled.rowCount,0);assert.equal(disabled.enabled,false);
const rows=Object.values(trace.rows);let children=0;const stack=[];
for(let i=0;i<rows.length;i+=32){const kind=rows[i],id=rows[i+1],parent=rows[i+2];if(kind===1){assert.equal(parent,stack.at(-1)??0);stack.push(id);if(parent)children++;}else{assert.equal(stack.pop(),id);assert(rows[i+8]>=rows[i+9]&&rows[i+9]>=0);}}
if(stack.length)assert(trace.incomplete,'truncated stacks must be INCOMPLETE');assert(children>0);
console.log(JSON.stringify({revision:'T250_SCOPES_EQUIVALENCE_V1',passed:12,failed:0,rowCount:trace.rowCount,capacity:trace.capacity,incomplete:trace.incomplete,maxDepth:trace.maxDepth,childScopes:children,authority:'BIT_IDENTICAL',device:'NOT_RUN'},null,2));
