// T306. Explicit cold-only reader. No calls to active/reset/offer or worker wake.
export interface TimelineModule {timeline(cursor:number):string;status():string;}
export interface ColdFile {name:string;bytes:number;sha256:string;}
export interface TimelineStore {save(name:string,text:string):Promise<ColdFile>;}
export interface TimelineIdentity {module:string;route:string;packageSha256:string;pid:number;}
export interface TimelineRow {serial:string;ns:string;tid:string;epoch:string;run:string;surface:string;input:string;now:string;last:string;delta:string;root:string;stage:number;}
const TRANSPORT_STAGES:string[]=['SubmitEnter','SubmitExit','ShaderEnter','ShaderExit','ProgramEnter','ProgramExit','ResourceEnter','ResourceExit','PrepareEnter','PrepareExit','DispatchEnter','DispatchExit','FenceEnter','FenceExit','PollEnter','PollExit'];
interface TimelinePage {revision:string;clock:string;capacity:number;rowBytes:number;count:number;dropped:string;serial:string;cursor:number;rows:TimelineRow[];next:number;completePage:boolean;stageRevision:string;}
export class TimelineManifest {
 stageRevision:string='T295_STAGE23_V1';stages:string[]=['INVALID','Invalidate','InitBegin','Ready','ReadyRejected','WaitEnter','Wake','FirstTick','AdvanceBegin','AdvanceEnd','AnchorSet','PumpBegin','PumpEnd','DrawBegin','DrawEnd','SwapBegin','SwapEnd','PauseBegin','PauseEnd','Offer','Consume','Stale','RootSubmit','RootPoll'];
 packageIdentitySource:string='OPERATOR_SIGNED_ARTIFACT_MANIFEST_EXTERNAL_VERIFY_REQUIRED';
 revision:string='T306_TIMELINE_COLD_V1';state:string='INCOMPLETE';reason:string='NOT_STARTED';identity:TimelineIdentity;
 before:string='';after:string='';statusBefore:string='';statusAfter:string='';pages:number=0;rows:number=0;missing:number=0;duplicates:number=0;
 firstSerial:string='';lastSerial:string='';groups:string[]=[];files:ColdFile[]=[];dataBytes:number=0;cancelled:boolean=false;end:string='END';
 constructor(id:TimelineIdentity){this.identity={module:id.module,route:id.route,packageSha256:id.packageSha256,pid:id.pid};}
}
export class GpuBridgeTimelineCold {
 busy:boolean=false;private cancelled:boolean=false;private sequence:number=0;
 cancel():void{this.cancelled=true;}
 private u64(s:string):boolean {return typeof s==='string'&&/^(0|[1-9][0-9]*)$/.test(s)&&(s.length<20||(s.length===20&&s<='18446744073709551615'));}
 private greater(a:string,b:string):boolean{return a.length>b.length||(a.length===b.length&&a>b);}
 private page(raw:string,cursor:number):TimelinePage {
  if(raw.length>16384)throw new Error('PAGE_BUDGET');const p:TimelinePage=JSON.parse(raw) as TimelinePage;
  if(p.revision!=='T295_STEADY_TRACE_V1'||p.clock!=='STEADY_NS'||(p.stageRevision!=='T295_STAGE23_V1'&&p.stageRevision!=='T317_STAGE39_V2')||p.capacity!==1024||!Number.isSafeInteger(p.rowBytes)||p.rowBytes<=0||!Number.isSafeInteger(p.count)||p.count<0||p.count>1024||!this.u64(p.dropped)||!this.u64(p.serial)||p.cursor!==cursor||!p.completePage||!Array.isArray(p.rows)||p.rows.length>4||p.next!==cursor+p.rows.length||p.next>p.count)throw new Error('INVALID_PAGE');
  for(const r of p.rows){if(!this.u64(r.serial)||!this.u64(r.ns)||!this.u64(r.tid)||!this.u64(r.epoch)||!this.u64(r.run)||!this.u64(r.surface)||!this.u64(r.input)||!this.u64(r.now)||!this.u64(r.last)||!this.u64(r.delta)||!this.u64(r.root)||!Number.isInteger(r.stage)||r.stage<1||r.stage>(p.stageRevision==='T317_STAGE39_V2'?39:23)||r.now!==r.ns)throw new Error('INVALID_ROW');}
  return p;
 }
 async collect(module:TimelineModule,store:TimelineStore,id:TimelineIdentity,yieldUi:()=>Promise<void>):Promise<TimelineManifest> {
  if(this.busy)throw new Error('EXPORT_BUSY');this.busy=true;this.cancelled=false;const out=new TimelineManifest(id);const prefix='gpu-timeline-'+id.module+'-'+id.pid+'-'+(++this.sequence)+'-'+Date.now();let previousNs='0',previousSerial='0';let target=0;let chunk='';let chunkIndex=0;
  const flush=async ():Promise<void>=>{if(!chunk)return;const file=await store.save(prefix+'-part'+chunkIndex+'.ndjson',chunk);out.files.push(file);out.dataBytes+=file.bytes;chunk='';chunkIndex++;};
  try {
   if(!/^[a-fA-F0-9]{64}$/.test(id.packageSha256))throw new Error('PACKAGE_SHA_REQUIRED');
   out.statusBefore=module.status();const initial=module.timeline(0);const first=this.page(initial,0);out.stageRevision=first.stageRevision;if(first.stageRevision==='T317_STAGE39_V2')out.stages=out.stages.concat(TRANSPORT_STAGES);out.before=initial;target=first.count;let cursor=0;const start=Date.now();
   do {
    if(this.cancelled)throw new Error('CANCELLED');if(Date.now()-start>30000)throw new Error('EXPORT_TIMEOUT');
    const raw=cursor===0?initial:module.timeline(cursor);const p=this.page(raw,cursor);
    if(p.count<target||p.rowBytes!==first.rowBytes||p.stageRevision!==first.stageRevision)throw new Error('PREFIX_CHANGED');
    if(cursor<target&&p.next===cursor)throw new Error('MISSING_PAGE');
    const take=Math.min(p.rows.length,target-cursor);
    for(let i=0;i<take;i++) {const row=p.rows[i];if(!this.greater(row.serial,previousSerial)){out.duplicates++;throw new Error('SERIAL_DUPLICATE_OR_BACKWARD');}if(row.serial!==String(cursor+i+1)){out.missing++;throw new Error('SERIAL_GAP');}if(this.greater(previousNs,row.ns))throw new Error('NS_BACKWARD');previousNs=row.ns;previousSerial=row.serial;if(!out.firstSerial)out.firstSerial=row.serial;out.lastSerial=row.serial;const group=row.epoch+'/'+row.run+'/'+row.surface+'/'+row.tid;if(out.groups.indexOf(group)<0)out.groups.push(group);}
    // Preserve original page bytes and all decimal U64 strings, even if new
    // rows appeared in the last page beyond the initial stable prefix.
    chunk+=raw+'\n';out.pages++;out.rows+=take;cursor+=take;
    if(out.dataBytes+chunk.length>1048576)throw new Error('EXPORT_BUDGET');if(chunk.length>=49152)await flush();
    if(cursor>=target)break;await yieldUi();
   }while(out.pages<=256);
   if(out.rows!==target)throw new Error('MISSING_ROWS');await flush();
   out.after=module.timeline(0);const last=this.page(out.after,0);out.statusAfter=module.status();
   if(last.stageRevision!==first.stageRevision||last.count!==first.count||last.serial!==first.serial||last.dropped!==first.dropped)throw new Error('GREW_DURING_EXPORT');
   if(id.packageSha256!==out.identity.packageSha256||id.module!==out.identity.module||id.pid!==out.identity.pid)throw new Error('PACKAGE_IDENTITY_CHANGED');
   if(first.dropped!=='0')throw new Error('NATIVE_DROPPED');out.state='COMPLETE';out.reason='STABLE_PREFIX_FULL_TRACE';
  }catch(e){out.state='INCOMPLETE';out.reason=String(e);out.cancelled=this.cancelled;try{await flush();}catch(io){out.reason+=';STORE:'+String(io);}}
  try{const manifest=await store.save(prefix+'-manifest.json',JSON.stringify(out));console.info('GRS_TIMELINE_COLD '+JSON.stringify({file:manifest.name,bytes:manifest.bytes,sha256:manifest.sha256,packageSha256:id.packageSha256,module:id.module,state:out.state}));}finally{this.busy=false;}
  return out;
 }
}
