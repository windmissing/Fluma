'use strict';
// Actual generated Worker/field code, SDK/NAPI stubs. No GL, device or performance evidence.
const fs=require('node:fs'),path=require('node:path'),Module=require('node:module'),assert=require('node:assert/strict'),{performance}=require('node:perf_hooks');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const root=path.resolve('.codex/local-test/t171-project/entry/src/main/ets'),port={onmessage:null,postMessage:value=>messages.push(value)},messages=[];
let config=null,generation=0,serial=0,ordinals=[],configures=0,resets=0,requests=0;
const gpu={configure(c){config=c;serial=0;ordinals=[0,0,0,0,0,0];generation++;configures++;return {status:'READY',errorCode:'NONE',ownerGenerationLo:generation,ownerGenerationHi:0,contextEpochLo:generation,contextEpochHi:0};},reset(){resets++;return {status:'RESET',resourcesRetired:true,liveBuffers:0};},selectPair(p,r,c,w){
 assert.equal(c.runId,config.runId+'|runToken:'+w.runToken);assert.equal(c.ownerGenerationLo,generation);assert.equal(c.contextEpochLo,generation);assert.ok(c.requestSerialLo>serial);serial=c.requestSerialLo;assert.ok(c.candidateOrdinal>=ordinals[c.candidateRole]);ordinals[c.candidateRole]=c.candidateOrdinal;requests++;
 const active=[];for(let i=0;i<p.nmax;i++)if(p.pState[i])active.push(i);active.sort((a,b)=>p.pId[a]-p.pId[b]);let a=-1,b=-1;
 for(const x of active){for(const y of active)if(p.pId[y]>p.pId[x]&&p.pPhase[x]===p.pPhase[y]&&p.pSource[x]===p.pSource[y]&&p.pSourceId[x]===p.pSourceId[y]&&p.pState[x]===p.pState[y]&&Math.floor(p.pX[x])===Math.floor(p.pX[y])&&Math.abs(p.pY[x]-p.pY[y])<=r){a=x;b=y;break;}if(a>=0)break;}
 return {status:a>=0?'PAIR':'NO_PAIR',slotA:a,slotB:b,errorCode:'NONE',execStatus:'NOT_RUN_HOST_STAND_IN',consumeStatus:'NOT_RUN',rawGlError:0};
}};
const fsStub={OpenMode:{CREATE:1,READ_WRITE:2,TRUNC:4},openSync:p=>({fd:fs.openSync(p,'w')}),writeSync:(fd,b)=>fs.writeSync(fd,Buffer.from(b)),fsyncSync:fs.fsyncSync,closeSync:fs.closeSync};
const original=Module._load;Module._load=function(id,parent,main){
 if(id==='@ohos.worker')return {default:{workerPort:port}};
 if(id==='@ohos.file.fs')return {default:fsStub};
 if(id==='@ohos.util')return {default:{TextEncoder:class{encodeInto(text){return new TextEncoder().encode(text);}}}};
 if(id==='@ohos.systemDateTime')return {default:{TimeType:{ACTIVE:1},getUptime:()=>performance.now()}};
 if(id==='libp011_gpu_pair_prototype.so')return {default:gpu};
 return original.call(this,id,parent,main);
};
require(path.join(root,'workers/T171Worker.ets'));
const {T171_FIXTURES}=require(path.join(root,'physics/T171Fixtures.ets')),{T159_SOURCE_SHA,T159_SHADER_SHA}=require(path.join(root,'physics/T159BuildIdentity.ets'));
const {t171ConfigId}=require(path.join(root,'physics/T171Protocol.ets'));
const fixture=T171_FIXTURES[0],results=[];
function command(mode){return {revision:'T171_RUN_BLOCK_V3',blockId:'PIPELINE_'+mode,fixtureId:fixture.fixtureId,fixtureSha256:fixture.fixtureSha256,route:'GPU',steps:1,oracleMode:mode==='PERFORMANCE'?'OFF':'ON',timingMode:'COLD_AND_STEADY_SPLIT',repeatId:0,mode,fault:'NONE',queuedUptimeMs:performance.now(),queuedWallMs:Date.now(),output:path.resolve('.codex/local-test/t171/pipeline-'+mode+'.jsonl'),recoveryShaderDirectory:'UNUSED_STUB',config:{nmax:fixture.nmax,shaderDirectory:'SDK_STUB',buildManifestSha256:'a'.repeat(64),sourceManifestSha256:T159_SOURCE_SHA,runId:'PIPELINE_'+mode,configId:t171ConfigId(fixture,1,mode==='PERFORMANCE'?'OFF':'ON','NONE'),diagnosticOracle:mode!=='PERFORMANCE',fullDump:mode!=='PERFORMANCE',buildShaderSha256:T159_SHADER_SHA[0],queryShaderSha256:T159_SHADER_SHA[1],reduceShaderSha256:T159_SHADER_SHA[2]}};}
for(const mode of ['PERFORMANCE','MICRO']){
 const c=command(mode),beforeConfig=configures,beforeReset=resets,beforeRequest=requests;port.onmessage({data:c});
 assert.equal(messages.at(-1).status,'COMPLETE',messages.at(-1).error);const rows=fs.readFileSync(c.output,'utf8').trim().split('\n').map(JSON.parse);
 if(mode==='PERFORMANCE'){
  const attempts=rows.filter(r=>r.phase==='ATTEMPT');assert.equal(attempts.length,99);assert.equal(attempts.filter(r=>r.run.warmup).length,9);assert.ok(attempts.every(r=>r.status==='TRANSACTION_RETURNED'));
  assert.equal(configures-beforeConfig,1,'one retained native context across GPU attempts');assert.equal(resets-beforeReset,1);
  for(const r of attempts){const e=JSON.parse(r.common);assert.equal(e.mergeCommitted,64);assert.equal(e.overflow,false);assert.equal(e.pairRows,null);assert.equal(JSON.parse(r.after).poolRawBytes.length,13);}
 }else{
  const queries=rows.filter(r=>r.phase==='MICRO_REQUEST');assert.equal(queries.length,162);assert.equal(queries.filter(r=>r.warmup).length,18);
  for(const nmax of [192,256,512])for(const family of ['PAIR','NO_PAIR']){const cell=queries.filter(r=>r.cell.nmax===nmax&&r.cell.family===family&&!r.warmup);assert.equal(cell.length,24);assert.ok(cell.every(r=>r.result.status===family));}
 }
 results.push({mode,status:'PASS',rows:rows.length,configures:configures-beforeConfig,resets:resets-beforeReset,requests:requests-beforeRequest,backend:'SDK_NAPI_CPU_STAND_IN_ONLY'});
}
Module._load=original;fs.writeFileSync('test/reports/t171/worker-pipeline.json',JSON.stringify({passed:results.length,failed:0,results,gpuExecution:'NOT_RUN',timing:'HOST_STUB_NOT_DEVICE_MEASUREMENT'},null,2)+'\n');console.log(JSON.stringify(results));
