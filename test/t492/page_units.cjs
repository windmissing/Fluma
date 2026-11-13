const fs=require('node:fs'),assert=require('node:assert/strict'),crypto=require('node:crypto'),Module=require('node:module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const project=process.argv[2],native=process.argv[3],report=process.argv[4];
const cache={};function load(name){if(cache[name])return cache[name].exports;const file=project+'/entry/src/main/ets/physics/'+name+'.ets',m=new Module(file);cache[name]=m;m.require=id=>id.startsWith('./')?load(id.slice(2)):require(id);m._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,file);return m.exports;}
const {GpuTimelineSnapshotCold}=load('GpuTimelineSnapshotCold'),{GpuM1DemoController}=load('GpuM1DemoController');
const pages=fs.readdirSync(native).filter(n=>/^page-\d+.json$/.test(n)).map(n=>JSON.parse(fs.readFileSync(native+'/'+n,'utf8'))).sort((a,b)=>a.cursor-b.cursor);
assert.equal(pages.length,32);
const base=pages[0],identity={module:'libgrsm1.so',route:'GpuResidentM1',packageSha256:'a'.repeat(64),pid:Number(base.scope.pid)};
const results=[];async function test(name,fn){await fn();results.push({name,status:'PASS'});}
async function collect(change=()=>{},yielding=()=>{},beginChange=()=>{},saveFault=false){
 const files=new Map(),events=[];const reader=new GpuTimelineSnapshotCold();let release=0,live=0;
 const evidence=native+'/reader-evidence/case-'+String(++collect.sequence);fs.mkdirSync(evidence,{recursive:true});
 const raw=(cursor,begin=false)=>{const p=structuredClone(pages[cursor/4]);(begin?beginChange:change)(p,cursor);return JSON.stringify(p);};
 const port={timeline:()=>{throw Error('LIVE_ENDPOINT_USED');},status:()=>JSON.stringify({live:++live}),timelineSnapshotBegin:()=>raw(0,true),timelineSnapshotPage:(id,cursor)=>{assert.equal(id,base.snapshotId);return raw(cursor);},timelineSnapshotEnd:id=>{assert.equal(id,base.snapshotId);release++;return 'SNAPSHOT_RELEASED';},executionDiagnostics:()=>fs.readFileSync(native+'/execution.json','utf8')};
 const store={save:async(name,text)=>{events.push(name);if(saveFault&&name.includes('-part'))throw Error('FSYNC_INJECTED');files.set(name,text);const fd=fs.openSync(evidence+'/'+name,'w');try{fs.writeFileSync(fd,text);fs.fsyncSync(fd);}finally{fs.closeSync(fd);}return {name,bytes:Buffer.byteLength(text),sha256:crypto.createHash('sha256').update(text).digest('hex')};}};
 const out=await reader.collectSnapshot(port,store,identity,async()=>{await yielding(reader);});
 assert.equal(release,1);assert.equal(out.release,'SNAPSHOT_RELEASED');return {out,files,events};
}
collect.sequence=0;
(async()=>{
 await test('actual native 125 rows / 66+67 / tokens; live growth ignored by snapshot identity',async()=>{const {out,files}=await collect();assert.equal(out.state,'COMPLETE');assert.equal(out.trace.rows,125);assert.equal(out.trace.pages,32);const text=[...files.values()].join('\n');assert(text.includes('requested-token')&&text.includes('"probeEnd":"0"'));});
 for(const [name,mutate,pattern] of [
  ['unknown68',(p,c)=>{if(c===120)p.rows[0].stage=68;},/INVALID_ROW:stage@120/],
  ['old dictionary141',(p,c)=>{if(c===120){p.stageRevision='T404_STAGE65_V1';p.rows[0].stage=141;}},/INVALID_ROW/],
  ['scope identity changes',(p,c)=>{if(c===4)p.scope.run='999';},/SNAPSHOT_IDENTITY_CHANGED/],
  ['wrong snapshot ID',(p,c)=>{if(c===4)p.snapshotId='999';},/SNAPSHOT_IDENTITY_CHANGED/],
  ['duplicate cursor',(p,c)=>{if(c===4)p.cursor=0;},/INVALID_PAGE/],
  ['serial duplicate',(p,c)=>{if(c===4)p.rows[0].serial='4';},/SERIAL_DUPLICATE/],
  ['native dropped',p=>p.dropped='1',/SNAPSHOT_IDENTITY_CHANGED|NATIVE_DROPPED/],
  ['malformed u64',(p,c)=>{if(c===4)p.rows[0].ns='18446744073709551616';},/INVALID_ROW:ns@4/],
  ['unterminated EXIT',(p,c)=>{if(c===120){p.rows[1].probeEnd='0';}},/probeEnd@121/]
 ])await test(name+' saves original rejected bytes before manifest and releases',async()=>{const {out,files,events}=await collect(mutate);assert.equal(out.state,'INCOMPLETE');assert.match(out.reason,pattern);assert(out.rejectedPage);const rejected=files.get(out.rejectedPage.name);assert.equal(crypto.createHash('sha256').update(rejected).digest('hex'),out.rejectedPage.sha256);assert(events.indexOf(out.rejectedPage.name)<events.findIndex(n=>n.endsWith('-manifest.json')));});
 await test('native dropped captured from Begin stays INCOMPLETE',async()=>{const mutate=p=>p.dropped='2';const {out}=await collect(mutate,()=>{},mutate);assert.match(out.reason,/NATIVE_DROPPED/);});
 await test('cancel releases snapshot preserving partial files',async()=>{const {out}=await collect(()=>{},r=>r.cancel());assert.equal(out.state,'INCOMPLETE');assert.match(out.reason,/CANCELLED/);assert.equal(out.trace.rows,4);});
 await test('30s export timeout releases',async()=>{const real=Date.now;let current=real();Date.now=()=>current;try{const {out}=await collect(()=>{},()=>{current+=30001;});assert.match(out.reason,/EXPORT_TIMEOUT/);}finally{Date.now=real;}});
 await test('fsync failure preserves INCOMPLETE and releases',async()=>{const {out}=await collect(()=>{},()=>{},()=>{},true);assert.equal(out.state,'INCOMPLETE');assert.match(out.reason,/FSYNC_INJECTED/);});
 await test('malformed Begin original bytes saved and ID still released',async()=>{const {out}=await collect(()=>{},()=>{},p=>p.scope.pid='999');assert.equal(out.state,'INCOMPLETE');assert(out.rejectedPage);});
 // Actual controller: do not substitute touches for the product entry. Exercise its normal down/tick API.
 const events=[],commands=[];let ready=false,request=0;const state=()=>({sourceUiRevision:'GRS_M1_DEMO_UI_V4',progressClockRevision:'GRS_M1_PROGRESS_CLOCK_V2',ready,permissionState:ready?'READY':'WAIT_NEW_SESSION',epoch:3,readyEpoch:ready?3:0,runLo:5,runHi:0,geometryLo:3,geometryHi:0,inputLo:3,inputHi:0,configLo:3,configHi:0,sourceControlLo:1,sourceControlHi:0,sourceOpen:true,sourceKind:0,pageSession:'7',sampleSerial:'1',sourceRequestSeq:request,sourceReceiptStatus:request?1:0,sourceAdmittedQ:12,sourceRejectedQ:0,sourceCommitLo:request,sourceCommitHi:0});
 const port={status:()=>JSON.stringify(state()),recordDiagnosticAction:raw=>{events.push(JSON.parse(raw));return 'ACTION_RECORDED';},recordSourceAttempt:()=> 'SOURCE_ATTEMPT_RECORDED',offer:b=>{commands.push(Buffer.from(b.slice(0)));request++;return 'ENQUEUED_NOT_ACK';},setSource:()=>'',setActive:()=>'',setGravityMode:()=>'',newRun:()=> 'NEW_RUN_REQUESTED'};
 const controller=new GpuM1DemoController(port);controller.setActive(true);controller.geometry(256,256);
 await test('not READY and outside coordinates reject without Offer',async()=>{assert.equal(controller.down(1,16,64),false);ready=true;assert.equal(controller.down(1,-1,64),false);assert.equal(commands.length,0);});
 await test('legitimate down preserves route wire revision; 32U32 hash matches actual Offer / receipt',async()=>{assert(controller.down(1,16,64));controller.tick(1000000000);controller.tick(1008000000);assert.equal(commands.length,1);assert.equal(commands[0].readUInt32LE(4),base.scope.route==='m2'?3:2);let hash=2166136261;for(const b of commands[0])hash=Math.imul(hash^b,16777619)>>>0;const upload=events.find(e=>e.event==='UPLOAD_32U32');assert.equal(JSON.parse(upload.detail).hash,hash.toString(16).padStart(8,'0'));controller.refresh();assert(events.some(e=>e.event==='SOURCE_RECEIPT'));assert(events.some(e=>e.event==='OFFER_REPLY'));fs.writeFileSync(native+'/controller-command.bin',commands[0]);});
 await test('two explicit resets retain action serial + return/exception',async()=>{controller.reset(51);controller.reset(52);assert.deepEqual(events.filter(e=>e.event==='NEW_RUN_RETURN').map(e=>e.actionSerial),[51,52]);port.newRun=()=>{throw Error('BRIDGE_THROW');};assert.throws(()=>controller.reset(53),/BRIDGE_THROW/);assert(events.some(e=>e.event==='NEW_RUN_THROW'&&e.actionSerial===53));});
 // Compile and invoke the real page restart/handler body, with only ArkUI scheduling stubbed.
 for(const page of ['GpuResidentM1','GpuSandWaterExperience'])await test(page+' handler → cancel → reset → bind ordered and hide cancels',async()=>{
  const source=fs.readFileSync(project+'/entry/src/main/ets/pages/'+page+'.ets','utf8');assert(source.includes('RESET_HANDLER_ENTER'));assert.match(source,/onPageHide\(\)[^{]*\{[^}]*this.cold.cancel\(\)/);
  const name=page==='GpuResidentM1'?'M1DemoFrame':'ExperienceFrame',at=source.indexOf('private actionSerial:'),end=source.indexOf(' private ',source.indexOf('private restart()',at)+10);
  const body=source.slice(at,end);assert(body.includes('NEW_FRAME_BIND'));const ordered=[];
  const mock={recordDiagnosticAction:raw=>ordered.push(JSON.parse(raw)),recoveryState:()=> '{}'};
  class Frame{constructor(){this.pulse={stop:()=>ordered.push('stop'),bindRequested:()=>ordered.push('bind')};this.owner=null;}}
  const code=ts.transpileModule('class Page{'+body+'}\nexports.Page=Page;',{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText;const m={exports:{}};new Function('exports','m1',name,code)(m.exports,mock,Frame);
  const p=new m.exports.Page();p.frame=new Frame();p.model={reset:serial=>ordered.push('reset:'+serial)};p.active=true;p.schedule=()=>{};p.getUIContext=()=>({postFrameCallback:()=>ordered.push('post')});p.restart();p.restart();assert.deepEqual(ordered.filter(x=>typeof x==='string'&&x.startsWith('reset:')),['reset:1','reset:2']);assert(ordered.findIndex(x=>x==='stop')<ordered.findIndex(x=>x==='reset:1'));assert(ordered.findIndex(x=>x==='reset:1')<ordered.findIndex(x=>x==='bind'));
 });
 const result={revision:'T492_PAGE_READER_CONTROLS_V1',passed:results.length,failed:0,device:'NOT_RUN',cases:results};fs.writeFileSync(report,JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result));
})().catch(error=>{console.error(error);process.exitCode=1;});
