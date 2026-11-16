const fs=require('node:fs'),assert=require('node:assert/strict'),Module=require('node:module'),crypto=require('node:crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const [project,native,output]=process.argv.slice(2),cache={};
function load(name){if(cache[name])return cache[name].exports;const file=project+'/entry/src/main/ets/physics/'+name+'.ets',m=new Module(file);cache[name]=m;m.require=id=>id.startsWith('./')?load(id.slice(2)):require(id);m._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,file);return m.exports;}
const {GpuColdTraceV1}=load('GpuColdTraceV1'),{GpuTimelineSnapshotCold}=load('GpuTimelineSnapshotCold'),{utf8Bytes}=load('GpuUtf8Budget');
const files=fs.readdirSync(native).filter(n=>/^trace-1-\d+.json$/.test(n));assert(files.length>1);
const trace=files.map(n=>JSON.parse(fs.readFileSync(native+'/'+n))).sort((a,b)=>a.page-b.page),head=trace[0],results=[];
const scope=head.scope;
const capability=()=>JSON.stringify({revision:'T503_BOUNDED_COLD_V1',traceRevision:'T503_M2_COLD_TRACE_V1',payloadRevision:'T445_M2_TRACE_V3',traceSupported:true,diagnosticBytes:2097152,pageBytes:16384,pageWords:1024,traceCapacityBytes:55410688,scope});
async function test(name,fn){await fn();results.push({name,status:'PASS'});}
async function run(options={}){
 let released=0,reads=0;const saved=new Map(),collector=new GpuColdTraceV1();
 const port={coldCapabilities:()=>options.unknown?JSON.stringify({revision:'OLD'}):capability(),coldTraceBeginV1:()=>JSON.stringify({state:'REQUESTED',requestId:'1',traceId:head.traceId,scope}),coldTraceStateV1:()=>JSON.stringify({state:'TRACE_READY',requestId:'1',traceId:head.traceId,scope}),coldTracePageV1:(id,page)=>{reads++;const value=structuredClone(trace[page]);if(options.bad&&page===1)value.words[27]=99;return JSON.stringify(value);},coldTraceEndV1:id=>{assert.equal(id,head.traceId);released++;return 'ENDED';}};
 const store={save:async(name,text)=>{if(options.io&&name.endsWith('-1.json'))throw Error('IO_INJECTED');if(options.manifest&&name.endsWith('-manifest.json'))throw Error('MANIFEST_INJECTED');saved.set(name,text);return {name,bytes:Buffer.byteLength(text),sha256:crypto.createHash('sha256').update(text).digest('hex')};}};
 let out,error;try{out=await collector.collect(port,store,'0',async()=>{if(options.cancel)collector.cancel();});}catch(e){error=e;}
 assert.equal(collector.busy,false);assert.equal(released,options.unknown?0:1);return {out,error,reads,saved};
}
(async()=>{
 await test('actual native QUANTUM pages stream without aggregate state',async()=>{const r=await run();assert.equal(r.out.state,'COMPLETE');assert.equal(r.out.words,head.actualWords);assert.equal(r.reads,head.pages+1);});
 for(const option of ['bad','io','cancel','manifest','unknown'])await test(option+' releases owned trace even on rejection',async()=>{const r=await run({[option]:true});if(option==='manifest')assert.match(String(r.error),/MANIFEST_INJECTED/);else assert.equal(r.out.state,'INCOMPLETE');});
 await test('UTF8 budget matches real bytes including surrogate pairs',()=>{for(const s of ['水🙂','\ud800','x'.repeat(16384),'\u0001'])assert.equal(utf8Bytes(s),Buffer.byteLength(s));});
 const fixture='.codex/local-test/t492-delivery-controls-m2';
 const pages=fs.readdirSync(fixture).filter(n=>/^page-\d+.json$/.test(n)).map(n=>JSON.parse(fs.readFileSync(fixture+'/'+n))).sort((a,b)=>a.cursor-b.cursor);assert.equal(pages.length,32);const base=pages[0];
 async function snapshot(mode){let release=0;const collector=new GpuTimelineSnapshotCold(),saved=new Map();
  const port={coldCapabilities:capability,timeline:()=>{throw Error('LEGACY_LIVE_USED');},status:()=> '{}',timelineSnapshotBegin:()=>JSON.stringify(base),timelineSnapshotPage:(id,cursor)=>JSON.stringify(pages[cursor/4]),timelineSnapshotEnd:()=>{release++;return 'ENDED';},executionDiagnosticsV1:(expected,remaining)=>{assert(remaining<=2097152&&remaining>=16384);if(mode==='busy')return JSON.stringify({state:'DIAGNOSTIC_BUSY'});if(mode==='oversize')return '水'.repeat(remaining);if(mode==='throw')throw Error('NAPI_INJECTED');return JSON.stringify({revision:'T503_EXECUTION_DIAGNOSTICS_V1',state:mode==='budget'?'INCOMPLETE':'COMPLETE',dataComplete:mode!=='budget',scopeStale:mode==='stale',reason:'DIAGNOSTIC_BUDGET'});}};
  const store={save:async(name,text)=>{if(mode==='manifest'&&name.endsWith('-manifest.json'))throw Error('MANIFEST_INJECTED');saved.set(name,text);return {name,bytes:Buffer.byteLength(text),sha256:'0'.repeat(64)};}};
  let out,error;try{out=await collector.collectSnapshot(port,store,{module:'libgrsm1.so',route:'GpuResidentM1',packageSha256:'0'.repeat(64),pid:Number(base.scope.pid)},async()=>{if(mode==='cancel')collector.cancel();});}catch(e){error=e;}
  assert.equal(release,1);assert.equal(collector.snapshotCollecting,false);return {out,error};
 }
 for(const mode of ['ok','busy','budget','stale','oversize','throw','cancel','manifest'])await test('snapshot '+mode+' finally releases legacy immutable reader',async()=>{const r=await snapshot(mode);if(mode==='manifest')assert(r.error);else assert.equal(r.out.state,mode==='ok'?'COMPLETE':'INCOMPLETE');});
 for(const page of ['GpuResidentM1','GpuSandWaterExperience'])await test(page+' actual collector migration and hide cancellation',()=>{const s=fs.readFileSync(project+'/entry/src/main/ets/pages/'+page+'.ets','utf8');assert(s.includes('collectSnapshot'));if(page==='GpuSandWaterExperience'){assert(s.includes('coldTrace.collect'));assert(s.includes('this.coldTrace.cancel()'));assert(!s.includes('m1.requestM2Trace('));assert(!s.includes('m1.readM2Trace('));}});
 fs.mkdirSync(require('node:path').dirname(output),{recursive:true});fs.writeFileSync(output,JSON.stringify({revision:'T507_COLLECTOR_CHECKS_V1',passed:results.length,failed:0,results,device:'NOT_RUN'},null,2));console.log(JSON.stringify({passed:results.length,failed:0}));
})().catch(e=>{console.error(e);process.exitCode=1;});
