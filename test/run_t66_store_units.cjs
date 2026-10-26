const fs=require('fs'),vm=require('vm'),a=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
let workers=[],logs=[];
class Worker {constructor(){workers.push(this);this.sent=[]}postMessage(x){this.sent.push(x);if(x.type==='init')queueMicrotask(()=>this.emit({type:'ready'}))}emit(x){this.onmessage({data:x})}terminate(){this.terminated=true}}
const exportsStore={};const source=fs.readFileSync('entry/src/main/ets/physics/P011NaturalObservationStore.ets','utf8');
vm.runInNewContext(ts.transpileModule(source,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,{exports:exportsStore,require:n=>({default:n==='@ohos.worker'?{ThreadWorker:Worker}:{info:(...x)=>logs.push(x),error:(...x)=>logs.push(x)}}),Promise,Error,JSON,Number});
(async()=>{
 const make=async id=>exportsStore.createP011NaturalObservationStore({cacheDir:'/cache'},id);
 const s=await make('r'),w=workers.at(-1),id=s.inspect();
 s.append({context:{sequence:3,sourceEpoch:5,observationRouteId:'actual-route'}});s.appendAdmission({runId:'r'});
 a.equal(s.inspect().writtenRecords,0);a.equal(s.inspect().sentRecords,1);
 await a.rejects(s.finish('r','q',id.storeGeneration+1));a.equal(w.sent.filter(x=>x.type==='close').length,0);
 const p=s.finish('r','q',id.storeGeneration);a.equal(p,s.finish('r','q',id.storeGeneration));await a.rejects(s.finish('r','other',id.storeGeneration));
 w.emit({type:'written',sequence:1});a.equal(s.inspect().writtenAdmissions,0);w.emit({type:'written',admissions:1});w.emit({type:'closed',sequence:1,admissions:1});await p;
 a.equal(s.inspect().state,'CLOSED');a.equal(s.inspect().routeId,'actual-route');a.equal(s.inspect().lastSequence,3);a.equal(w.sent.filter(x=>x.type==='close').length,1);
 const bad=await make('bad'),bw=workers.at(-1);bad.append({context:{sequence:1}});const bp=bad.finish('bad','q',bad.inspect().storeGeneration);bw.emit({type:'closed',sequence:1,admissions:0});await a.rejects(bp);a.equal(bad.inspect().state,'FAILED');
 const skip=await make('skip'),sw=workers.at(-1);skip.append({context:{sequence:1}});skip.append({context:{sequence:2}});sw.emit({type:'written',sequence:2});a.equal(skip.inspect().state,'FAILED');await a.rejects(skip.close());
 console.log(JSON.stringify({suite:'T66_store',passed:5,failed:0,cases:['finish exact identity and request idempotency','separate written counters','strict dual stream closed fence','missing written acknowledgement rejects CLOSED','skipped written sequence fails closed']},null,2));
})().catch(e=>{console.error(e);process.exitCode=1});
