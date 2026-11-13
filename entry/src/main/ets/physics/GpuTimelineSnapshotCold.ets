import { GpuBridgeTimelineCold, TimelineModule, TimelineStore, TimelineIdentity, TimelinePage, TimelineManifest, ColdFile } from './GpuBridgeTimelineCold';
export interface SnapshotPort extends TimelineModule {
 timelineSnapshotBegin():string;timelineSnapshotPage(id:string,cursor:number):string;timelineSnapshotEnd(id:string):string;
 executionDiagnostics():string;
}
interface SnapshotScope {epoch:string;run:string;context:string;surface:string;pid:string;packageSource:string;route:string;source:string;}
interface SnapshotPage extends TimelinePage {snapshotRevision:string;snapshotId:string;capturedNs:string;copyNs:string;scope:SnapshotScope;payloadBytes:number;}
interface SnapshotProbeRow {stage:number;probeStart?:string;probeEnd?:string;}
class SnapshotAdapter implements TimelineModule {
 private port:SnapshotPort;private id:string;
 constructor(port:SnapshotPort,id:string){this.port=port;this.id=id;}
 timeline(cursor:number):string{return this.port.timelineSnapshotPage(this.id,cursor);}
 status():string{return this.port.status();}
}
export class SnapshotManifest {
 revision:string='T492_SNAPSHOT_EXPORT_V1';snapshotId:string='';scope:SnapshotScope|null=null;
 beginRaw:string='';release:string='NOT_REQUESTED';trace:TimelineManifest|null=null;state:string='INCOMPLETE';reason:string='NOT_STARTED';
 files:ColdFile[]=[];rejectedPage:ColdFile|null=null;
}
// The original live reader and its fail-closed predicates remain available.
export class GpuTimelineSnapshotCold extends GpuBridgeTimelineCold {
 private identity:string='';private reference:SnapshotPage|null=null;private snapshotCollecting:boolean=false;
 private exact(p:SnapshotPage):string{return JSON.stringify({id:p.snapshotId,scope:p.scope,captured:p.capturedNs,copy:p.copyNs,count:p.count,serial:p.serial,dropped:p.dropped,dict:p.stageRevision});}
 protected page(raw:string,cursor:number):TimelinePage {
  const p=super.page(raw,cursor) as SnapshotPage;
  if(p.snapshotRevision!=='T492_IMMUTABLE_TIMELINE_V1'||p.snapshotId!==this.identity||p.payloadBytes!==753664||p.scope===undefined||this.reference===null||this.exact(p)!==this.exact(this.reference))throw new Error('SNAPSHOT_IDENTITY_CHANGED:scope');
  for(let i:number=0;i<p.rows.length;i++){
   const r=p.rows[i] as SnapshotProbeRow;
   if(r.stage>=43&&r.probeStart!==undefined&&r.probeStart!=='0'&&r.probeEnd==='0'&&!(p.stageRevision==='T477_STAGE67_V1'&&r.stage===66))throw new Error('INVALID_ROW:probeEnd@'+String(cursor+i));
  }
  return p;
 }
 async collectSnapshot(port:SnapshotPort,store:TimelineStore,id:TimelineIdentity,yieldUi:()=>Promise<void>):Promise<SnapshotManifest>{
  if(this.snapshotCollecting||this.busy)throw new Error('EXPORT_BUSY');this.snapshotCollecting=true;
  const out=new SnapshotManifest();const prefix='gpu-snapshot-'+id.pid+'-'+Date.now();let pending:string='';
  try{
   out.beginRaw=port.timelineSnapshotBegin();pending=out.beginRaw;
   if(out.beginRaw.length>16384)throw new Error('PAGE_BUDGET');
   const p=JSON.parse(out.beginRaw) as SnapshotPage;
   // Retain an acquired ID before validation, so malformed metadata still releases.
   if(typeof p.snapshotId==='string'&&/^[1-9][0-9]{0,19}$/.test(p.snapshotId)){this.identity=p.snapshotId;out.snapshotId=p.snapshotId;}
   if(p.snapshotRevision!=='T492_IMMUTABLE_TIMELINE_V1'||!out.snapshotId||p.scope===undefined||p.scope.pid!==String(id.pid))throw new Error('SNAPSHOT_BEGIN_SCOPE:pid');
   for(const value of [p.scope.epoch,p.scope.run,p.scope.context,p.scope.surface,p.scope.pid,p.capturedNs,p.copyNs])if(typeof value!=='string'||!/^(0|[1-9][0-9]{0,19})$/.test(value)||(value.length===20&&value>'18446744073709551615'))throw new Error('SNAPSHOT_BEGIN_SCOPE:u64');
   if(typeof p.scope.packageSource!=='string'||p.scope.packageSource.length>128||typeof p.scope.source!=='string'||p.scope.source.length>128||(p.scope.route!=='m1'&&p.scope.route!=='m2'))throw new Error('SNAPSHOT_BEGIN_SCOPE:source');
   this.reference=p;this.page(out.beginRaw,0);out.scope=p.scope;pending='';
   out.trace=await super.collect(new SnapshotAdapter(port,out.snapshotId),store,id,yieldUi);
   out.state=out.trace.state;out.reason=out.trace.reason;out.rejectedPage=out.trace.rejectedPage;
   const diagnostics=port.executionDiagnostics();if(diagnostics.length>2097152||diagnostics.length+out.trace.dataBytes>4194304)throw new Error('DIAGNOSTIC_BUDGET');
   out.files.push(await store.save(prefix+'-execution.json',diagnostics));
  }catch(error){
   // Persist original rejected bytes before reporting the validation reason.
   if(pending.length>0&&pending.length<=16384){try{out.rejectedPage=await store.save(prefix+'-rejected-page.json',pending);}catch(io){out.reason='REJECTED_PAGE_STORE:'+String(io);}}
   out.state='INCOMPLETE';out.reason=String(error)+';'+out.reason;
  }finally{
   if(out.snapshotId){try{out.release=port.timelineSnapshotEnd(out.snapshotId);}catch(error){out.release='RELEASE_FAILED:'+String(error);}}
   this.identity='';this.reference=null;
   try{out.files.push(await store.save(prefix+'-manifest.json',JSON.stringify(out)));}finally{this.snapshotCollecting=false;}
  }
  return out;
 }
}
