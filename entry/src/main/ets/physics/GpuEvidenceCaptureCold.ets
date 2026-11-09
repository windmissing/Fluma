import util from '@ohos.util';
import { ColdFile, TimelineStore } from './GpuBridgeTimelineCold';
export interface EvidenceCaptureReply {state:string;reason:string;captureId?:string;requestId?:string;lastBlocker?:string;}
export interface EvidenceArmConfig {packageSha256:string;expectedRevision:string;epoch:string;runLo:number;runHi:number;pageSession:string;durationNs:string;maxRows:number;maxBytes:number;}
export interface EvidenceCapturePort {
 beginEvidenceCapture(configJson:string):string;endEvidenceCapture(id:string):string;
 evidenceCapturePage(id:string,cursor:number):string;
}
export interface EvidenceLedgerView {
 generatedQuanta:string;acceptedQuantumCount:string;pendingQuanta:string;activePlanRemaining:string;
 overloadDroppedQuanta:string;revokedDroppedQuanta:string;quotaCarry:string;clearedCarry:string;
}
export interface EvidenceSummaryView {
 initialLedger:EvidenceLedgerView;terminalLedger:EvidenceLedgerView;maxRows:number;maxBytes:number;
 dropped:string;suppressedDetailedRows:string;readySeen:boolean;sourceAttemptSeen:boolean;
 acceptedSeen:boolean;sourceOfferSeen:boolean;faultSeen:boolean;revokeSeen:boolean;
}
export interface EvidenceEventView {serial:string;captureId:string;kind:string;ledger:EvidenceLedgerView;}
export interface EvidenceTokenView {root:string;rootToken:string;requestedToken:string;renderedToken:string;}
export interface EvidencePageView {
 evidenceSource:string;clock:string;uiClock:string;probeClock:string;
 revision:string;state:string;incomplete:boolean;reason:string;captureId:string;activeRevision:string;
 packageSha256:string;epoch:string;runLo:number;runHi:number;pageSession:string;startNs:string;endNs:string;
 cursor:number;next:number;complete:boolean;pageCount:number;rowCount:number;tokenCount:number;type:string;
 summary?:EvidenceSummaryView;rows?:EvidenceEventView[];token?:EvidenceTokenView;
}
export class EvidenceManifest {
 evidenceSource:string='';nativeClock:string='';uiClock:string='';probeClock:string='';
 revision:string='T409_EVIDENCE_ARCHIVE_V1';state:string='INCOMPLETE';reason:string='NOT_STARTED';
 captureId:string='';activeRevision:string='';packageSha256:string='';epoch:string='';runLo:number=0;runHi:number=0;
 pageSession:string='';startNs:string='';endNs:string='';rowCount:number=0;tokenCount:number=0;pageCount:number=0;
 files:ColdFile[]=[];summary:EvidenceSummaryView|null=null;requiredEvents:string[]=['BEGIN','END'];
 dataBytes:number=0;totalBytes:number=0;maxBytes:number=1048576;
 operationClock:string='WALL_MS';exportStartMs:number=0;exportEndMs:number=0;
 systemPresentVerified:boolean=false;
}
export class GpuEvidenceCaptureCold {
 busy:boolean=false;private cancelled:boolean=false;
 cancel():void{this.cancelled=true;}
 private byteLength(s:string):number{return new util.TextEncoder().encodeInto(s).length;}
 private u64(s:string):boolean{return typeof s==='string'&&/^(0|[1-9][0-9]*)$/.test(s)&&(s.length<20||(s.length===20&&s<='18446744073709551615'));}
 private ledger(l:EvidenceLedgerView):boolean{
  return l!==undefined&&[l.generatedQuanta,l.acceptedQuantumCount,l.pendingQuanta,l.activePlanRemaining,
   l.overloadDroppedQuanta,l.revokedDroppedQuanta,l.quotaCarry,l.clearedCarry].every((v:string)=>this.u64(v));
 }
 private same(p:EvidencePageView,f:EvidencePageView):boolean {
  return p.evidenceSource===f.evidenceSource&&p.clock===f.clock&&p.uiClock===f.uiClock&&p.probeClock===f.probeClock&&p.captureId===f.captureId&&p.activeRevision===f.activeRevision&&p.packageSha256===f.packageSha256&&
   p.epoch===f.epoch&&p.runLo===f.runLo&&p.runHi===f.runHi&&p.pageSession===f.pageSession&&
   p.startNs===f.startNs&&p.endNs===f.endNs&&p.incomplete===f.incomplete&&p.reason===f.reason&&p.pageCount===f.pageCount&&p.rowCount===f.rowCount&&p.tokenCount===f.tokenCount;
 }
 async collect(port:EvidenceCapturePort,store:TimelineStore,id:string,sha:string,pid:number,yieldUi:()=>Promise<void>):Promise<EvidenceManifest>{
  if(this.busy)throw new Error('EXPORT_BUSY');this.busy=true;this.cancelled=false;
  const out=new EvidenceManifest();out.exportStartMs=Date.now();
  const prefix='gpu-evidence-'+pid+'-'+id+'-'+Date.now();let eventCount=0,tokenCount=0;const events:string[]=[];const tokenRoots:string[]=[];
  try{
   if(!this.u64(id)||id==='0'||!/^[a-fA-F0-9]{64}$/.test(sha))throw new Error('CAPTURE_ID_OR_SHA_REQUIRED');
   let first:EvidencePageView|null=null;let terminal:EvidenceLedgerView|null=null;
   for(let cursor=0;cursor<1033;cursor++){
    if(this.cancelled)throw new Error('CANCELLED');
    const raw=port.evidenceCapturePage(id,cursor);
    if(raw.length>16384)throw new Error('PAGE_BUDGET');
    const p:EvidencePageView=JSON.parse(raw) as EvidencePageView;
    if(p.revision!=='T409_EVIDENCE_CAPTURE_PAGE_V1'||p.state!=='FROZEN'||p.captureId!==id||p.packageSha256!==sha||typeof p.incomplete!=='boolean'||(p.activeRevision!=='GRS_M1_PROGRESS_CLOCK_V1'&&p.activeRevision!=='GRS_M1_PROGRESS_CLOCK_V2')||!Number.isInteger(p.runLo)||p.runLo<0||p.runLo>4294967295||!Number.isInteger(p.runHi)||p.runHi<0||p.runHi>4294967295||p.cursor!==cursor||p.next!==cursor+1||
     !Number.isInteger(p.pageCount)||p.pageCount<1||p.pageCount>1033||p.complete!==(p.next===p.pageCount)||
     !this.u64(p.epoch)||!this.u64(p.pageSession)||!this.u64(p.startNs)||!this.u64(p.endNs)||
     !Number.isInteger(p.rowCount)||p.rowCount<2||p.rowCount>4096||!Number.isInteger(p.tokenCount)||p.tokenCount<0||p.tokenCount>8)throw new Error('INVALID_CAPTURE_PAGE');
    if(first===null){
     if(p.type!=='SUMMARY'||p.summary===undefined||!this.ledger(p.summary.initialLedger)||!this.ledger(p.summary.terminalLedger)||
      !Number.isInteger(p.summary.maxBytes)||p.summary.maxBytes>1048576||p.summary.maxBytes<98304||!this.u64(p.summary.dropped))throw new Error('MISSING_SUMMARY');
     first=p;out.evidenceSource=p.evidenceSource;out.nativeClock=p.clock;out.uiClock=p.uiClock;out.probeClock=p.probeClock;out.captureId=p.captureId;out.activeRevision=p.activeRevision;out.packageSha256=p.packageSha256;
     out.epoch=p.epoch;out.runLo=p.runLo;out.runHi=p.runHi;out.pageSession=p.pageSession;out.startNs=p.startNs;out.endNs=p.endNs;
     out.rowCount=p.rowCount;out.tokenCount=p.tokenCount;out.pageCount=p.pageCount;out.summary=p.summary;out.maxBytes=p.summary.maxBytes;
     if(p.summary.readySeen)out.requiredEvents.push('READY');
     if(p.summary.sourceAttemptSeen)out.requiredEvents.push('SOURCE_ATTEMPT');
     if(p.summary.sourceOfferSeen)out.requiredEvents.push('FIRST_OFFER');
     if(p.summary.acceptedSeen)out.requiredEvents.push('FIRST_ACCEPT');
     if(p.summary.faultSeen)out.requiredEvents.push('FIRST_FAILURE');
     if(p.summary.revokeSeen)out.requiredEvents.push('REVOKE');
    }else if(!this.same(p,first))throw new Error('CAPTURE_IDENTITY_CHANGED');
    if(cursor>0&&p.type!==(cursor<=Math.ceil(p.rowCount/4)?'EVENTS':'TOKEN'))throw new Error('PAGE_TYPE_ORDER');
    if(p.pageCount!==1+Math.ceil(p.rowCount/4)+p.tokenCount)throw new Error('PAGE_COUNT');
    if(p.type==='EVENTS'){
     if(p.rows===undefined||p.rows.length<1||p.rows.length>4)throw new Error('MISSING_ROWS');
     for(const r of p.rows){if(r.captureId!==id||r.serial!==String(++eventCount)||!this.ledger(r.ledger))throw new Error('SERIAL_OR_LEDGER');events.push(r.kind);if(r.kind==='END')terminal=r.ledger;}
    }else if(p.type==='TOKEN'){
     if(p.token===undefined||!this.u64(p.token.root)||!p.token.rootToken||tokenRoots.indexOf(p.token.root)>=0)throw new Error('MISSING_TOKEN');tokenRoots.push(p.token.root);tokenCount++;
    }else if(cursor!==0)throw new Error('PAGE_TYPE_ORDER');
    // Keep room for the complete manifest (all referenced SHA receipts), never
    // continue writing raw fragments past the configured total budget.
    if(out.dataBytes+raw.length+49152>out.maxBytes)throw new Error('EXPORT_TOTAL_BUDGET');
    const file=await store.save(prefix+'-page'+cursor+'.json',raw);
    if(file.bytes!==raw.length||!/^[a-f0-9]{64}$/i.test(file.sha256))throw new Error('FILE_RECEIPT');
    out.files.push(file);out.dataBytes+=file.bytes;
    if(p.complete){
     if(eventCount!==out.rowCount||tokenCount!==out.tokenCount||out.files.length!==out.pageCount)throw new Error('MISSING_FRAGMENTS');
     if(out.requiredEvents.some((v:string)=>events.indexOf(v)<0)||events[0]!=='BEGIN'||events[events.length-1]!=='END')throw new Error('MISSING_REQUIRED_EVENTS');
     if(terminal===null||JSON.stringify(terminal)!==JSON.stringify(out.summary?.terminalLedger))throw new Error('TERMINAL_MISMATCH');
     if(p.incomplete||out.summary?.dropped!=='0')throw new Error('NATIVE_INCOMPLETE');
     out.state='COMPLETE';out.reason='FROZEN_WINDOW_ALL_FILES';break;
    }
    await yieldUi();
   }
   if(out.state!=='COMPLETE')throw new Error('MISSING_FINAL_PAGE');
  }catch(e){out.state='INCOMPLETE';out.reason=String(e).slice(0,128);}
  out.exportEndMs=Date.now();let text='';
  // All protocol bytes are ASCII; actual UTF8 byte lengths are checked above.
  for(let i=0;i<8;i++){text=JSON.stringify(out);const total=out.dataBytes+this.byteLength(text);if(total===out.totalBytes)break;out.totalBytes=total;}
  text=JSON.stringify(out);
  if(out.totalBytes>out.maxBytes){out.state='INCOMPLETE';out.reason='MANIFEST_TOTAL_BUDGET';text=JSON.stringify(out);}
  try{const manifest=await store.save(prefix+'-manifest.json',text);console.info('GRS_EVIDENCE_CAPTURE '+JSON.stringify(manifest));}
  finally{this.busy=false;}
  return out;
 }
}
