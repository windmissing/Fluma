'use strict';
// T339 validator anti-drift tests (T337 recommendation, semantic-positioned):
// the card text says "V1 1024-page sample VALID + V2 4096-page sample VALID",
// but after the fix the validator MUST reject capacity 1024 (that rejection IS
// the fix). The V1/V2 in the card therefore maps to the page DICTIONARY
// versions (stageRevision T295_STAGE23_V1 / T317_STAGE39_V2 / T324_STAGE41_V1),
// not ring capacities: each dictionary version constructs a VALID 4096 page;
// out-of-capacity samples (stale 1024, over-limit 4097) must stay INVALID
// (fail-closed). Noted in the task receipt as semantic positioning.
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText,f);
const {GpuBridgeTimelineCold}=require(path.resolve('entry/src/main/ets/physics/GpuBridgeTimelineCold.ets'));
const dir=fs.mkdtempSync(path.join(require('os').tmpdir(),'t339-'));
const store={save:async(name,data)=>{const b=Buffer.from(data,'utf8');fs.writeFileSync(path.join(dir,name),b);return {name,bytes:b.length,sha256:require('crypto').createHash('sha256').update(b).digest('hex')};}};
const id={packageSha256:'a'.repeat(64),module:'libgrsm1.so',pid:'123'};
function page(cursor,count,capacity,stageRevision){return {revision:'T295_STEADY_TRACE_V1',clock:'STEADY_NS',stageRevision,capacity,rowBytes:120,count,dropped:'0',serial:String(count),cursor,rows:Array.from({length:Math.min(4,count-cursor)},(_,j)=>({serial:String(cursor+j+1),ns:String(9007199254740993n+BigInt(cursor+j)),now:String(9007199254740993n+BigInt(cursor+j)),tid:'18446744073709551615',epoch:'1',run:'1',surface:'1',input:'1',last:'0',delta:'0',root:'0',stage:6})),next:Math.min(cursor+4,count),completePage:true};}
async function collect(count,capacity,stageRevision){const c=new GpuBridgeTimelineCold();const module={status:()=>'{"state":"PAUSED_CLOCK"}',timeline:cursor=>JSON.stringify(page(cursor,count,capacity,stageRevision))};return c.collect(module,store,id,async()=>{});}
(async()=>{
 let passed=0;
 // 1) dictionary T295_STAGE23_V1, capacity 4096 -> VALID
 {const out=await collect(5,4096,'T295_STAGE23_V1');assert.equal(out.state,'COMPLETE');passed++;}
 // 2) dictionary T324_STAGE41_V1, capacity 4096 -> VALID (T317_STAGE39_V2 covered by t324 suite)
 {const out=await collect(5,4096,'T324_STAGE41_V1');assert.equal(out.state,'COMPLETE');passed++;}
 // 3) stale capacity 1024 (the pre-T336 ring) -> INVALID (fail-closed; this is the T337 blocker root)
 {const out=await collect(5,1024,'T324_STAGE41_V1');assert.equal(out.state,'INCOMPLETE');assert.match(out.reason,/INVALID_PAGE/);passed++;}
 // 4) over-limit capacity 4097 / count 4097 -> INVALID (fail-closed upper bound)
 {const out=await collect(4097,4097,'T324_STAGE41_V1');assert.equal(out.state,'INCOMPLETE');assert.match(out.reason,/INVALID_PAGE/);passed++;}
 const report={revision:'T339_VALIDATOR_ANTI_DRIFT_V1',passed,failed:0,device:'NOT_RUN'};
 fs.mkdirSync('test/reports/t339',{recursive:true});
 fs.writeFileSync('test/reports/t339/anti-drift.json',JSON.stringify(report,null,2));
 console.log(JSON.stringify(report));
})().catch(e=>{console.error(String(e));process.exit(1);});
