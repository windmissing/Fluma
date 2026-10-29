'use strict';
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.resolve(process.argv[2]||'.codex/local-test/t250-release/entry/src/main/ets/physics');
const {runTxGpuFixture:run}=require(base+'/tx/TxGpuFixture.ets'),{TxGpuTreeAdapter:Tree}=require(base+'/tx/TxGpuTreeAdapter.ets');
class NativeDouble {
 configure(){return{status:'READY',errorCode:'NONE',ownerGenerationLo:7,ownerGenerationHi:1,contextEpochLo:8,contextEpochHi:1,liveBuffers:12};}
 selectPair(p,r){let a=-1,b=-1;for(let i=0;i<p.nmax;i++)for(let j=0;j<p.nmax;j++){
  if(!p.pState[i]||!p.pState[j]||p.pId[i]>=p.pId[j]||p.pPhase[i]!==p.pPhase[j]||p.pSource[i]!==p.pSource[j]||p.pSourceId[i]!==p.pSourceId[j]||p.pState[i]!==p.pState[j]||Math.floor(p.pX[i])!==Math.floor(p.pX[j])||Math.abs(p.pY[i]-p.pY[j])>r)continue;
  if(a<0||p.pId[i]<p.pId[a]||p.pId[i]===p.pId[a]&&p.pId[j]<p.pId[b]){a=i;b=j;}}
 return{status:a<0?'NO_PAIR':'PAIR',slotA:a,slotB:b,errorCode:'HOST_CPU_DOUBLE_NOT_GPU',dispatchCount:11};
 }
 reset(){return{status:'RESET',liveBuffers:0,resourcesRetired:true};}
}
const normalize=s=>JSON.parse(s,(k,v)=>k==='runId'?'RUN':v);let cases=[];
for(const nmax of [192,512]){let reference=null;for(const arm of ['COMMON_FREEZE_OFF','COMMON_FREEZE_ON'])for(const backend of ['LEGACY_SCAN','TX_ADJ_AVL','GPU_PAIR_V1']){
 const config={nmax,shaderDirectory:'HOST_NOT_GPU',runId:'same-fixture',configId:'T250',sourceManifestSha256:'a'.repeat(64),buildManifestSha256:'b'.repeat(64),buildShaderSha256:'c'.repeat(64),queryShaderSha256:'d'.repeat(64),reduceShaderSha256:'e'.repeat(64),diagnosticOracle:true,fullDump:true,freezeArm:arm,backend,policy:'TX_EXACT_STRUCTURE_V1',traceCapacity:4096,variant:'STRICT_O2'};
 const result=run(config,'WET_'+nmax+'_64',1,backend==='GPU_PAIR_V1'?new NativeDouble():null,()=>0,backend==='TX_ADJ_AVL'?new Tree(nmax):null);
 assert.equal(result.physicalStatus,'COMMITTED',result.error);assert.equal(result.rootRows.length,3);const after=normalize(result.after);if(reference===null)reference=after;else assert.deepEqual(after,reference);
 const work=result.workloadRows.map(JSON.parse);assert(work.some(r=>r.kind==='B_BIRTH_COMPLETED'));assert(work.some(r=>r.kind==='SOURCE_LAMBDA_ATTEMPT'));
 const count=kind=>work.filter(r=>r.kind===kind).reduce((s,r)=>s+r.value,0);
 for(let i=0;i<result.count;i++)if(arm==='COMMON_FREEZE_ON')assert(result.ledger[i*64+61]>0);
 cases.push({nmax,arm,backend:backend==='GPU_PAIR_V1'?'GPU_INTERFACE_CPU_DOUBLE':backend,status:'PASS',roots:result.rootRows.length,B:count('B_BIRTH_COMPLETED'),M:count('M_MERGE_SUCCESS'),sourceLambda:count('SOURCE_LAMBDA_ATTEMPT'),selectorRequests:result.count});
}}
const beforeManifest=path.resolve(base,'../../../../../t250-before-manifest.json');
console.log(JSON.stringify({revision:'T250_OFFLINE_V2',cases,passed:cases.length,failed:0,nativeGpu:'NOT_RUN',sameBinaryDevicePid:'NOT_RUN',faultCuts:fs.existsSync(beforeManifest)?'BEFORE_MATRIX_SEPARATE_SEE_BEFORE_SUMMARY':'20_NOT_IMPLEMENTED'},null,2));
