import { PrepareAuthorityReport, PreparePairOwner, PairRaster, PairKeyboard, PairRect, buildPreparePairingEvidence, compactPreparePairingEvidence, minimumPreparePairingEvidence } from './PreparePairingEvidence';
import { PrepareReadiness, PrepareSnapshot } from './PrepareReadiness';
// T658/T659: method-free DTO. JSON/native values never supply authorization methods.
export class PrepareAttemptOwner implements PreparePairOwner {
 prepareId:number=0;pageGeneration:number=0;branch:number=0;windowId:number=0;windowGeneration:number=0;controller:number=0;session:number=0;surface:number=0;context:number=0;run:string='';
}
export function copyPrepareOwner(o:PrepareAttemptOwner):PrepareAttemptOwner|null {
 if(o===null||o===undefined||![o.prepareId,o.pageGeneration,o.branch,o.windowId,o.windowGeneration,o.controller,o.session,o.surface,o.context].every((v:number)=>typeof v==='number'&&Number.isSafeInteger(v)&&v>=0)||o.controller===0||o.session===0||o.surface===0||o.context===0||typeof o.run!=='string'||o.run.length===0)return null;
 const c=new PrepareAttemptOwner();c.prepareId=o.prepareId;c.pageGeneration=o.pageGeneration;c.branch=o.branch;c.windowId=o.windowId;c.windowGeneration=o.windowGeneration;c.controller=o.controller;c.session=o.session;c.surface=o.surface;c.context=o.context;c.run=o.run;return c;
}
export function samePrepareOwner(a:PrepareAttemptOwner,b:PrepareAttemptOwner):boolean {
 return a.prepareId===b.prepareId&&a.pageGeneration===b.pageGeneration&&a.branch===b.branch&&a.windowId===b.windowId&&a.windowGeneration===b.windowGeneration&&a.controller===b.controller&&a.session===b.session&&a.surface===b.surface&&a.context===b.context&&a.run===b.run;
}
export function prepareOwnerKey(o:PrepareAttemptOwner):string {return JSON.stringify([o.prepareId,o.pageGeneration,o.branch,o.windowId,o.windowGeneration,o.controller,o.session,o.surface,o.context,o.run]);}
export function prepareUtf8Bytes(s:string):number {let bytes=0;for(let i=0;i<s.length;i++){const c=s.charCodeAt(i);if(c<128)bytes++;else if(c<2048)bytes+=2;else if(c>=55296&&c<=56319&&i+1<s.length&&s.charCodeAt(i+1)>=56320&&s.charCodeAt(i+1)<=57343){bytes+=4;i++;}else bytes+=3;}return bytes;}
export function prepareBoundedText(s:string,limit:number):string {let end=0,bytes=0;while(end<s.length){const c=s.charCodeAt(end);let n=1,b=c<128?1:c<2048?2:3;if(c>=55296&&c<=56319&&end+1<s.length&&s.charCodeAt(end+1)>=56320&&s.charCodeAt(end+1)<=57343){n=2;b=4;}if(bytes+b>limit)break;bytes+=b;end+=n;}return s.substring(0,end);}
// Escaped JSON also consumes the 16KiB envelope; retain raw prefixes with explicit flags.
function prepareErrorText(s:string,rawLimit:number,jsonLimit:number):string {let value=prepareBoundedText(s,rawLimit);while(prepareUtf8Bytes(JSON.stringify(value))>jsonLimit)value=prepareBoundedText(value,Math.floor(prepareUtf8Bytes(value)/2));return value;}
export class PrepareCheckTicket {valid:boolean=false;prepareId:number=0;pageGeneration:number=0;observationSerial:number=0;inputSerial:number=0;ownerSerial:number=0;owner:string='';reason:string='';}
class PrepareOwnerError {
 stage:string='';code:number=0;name:string='';message:string='';messageTruncated:boolean=false;nameTruncated:boolean=false;before:string='';after:string='';ownerTruncated:boolean=false;prepareId:number=0;pageGeneration:number=0;branch:number=0;
}
// One gate owns the original predicate, immutable private owner and issued ticket.
export class PrepareReadinessV4 extends PrepareReadiness {
 private serialV4:number=0;
 get ownerSerial():number{return this.serialV4;}
 subphase:string='DISMISSING';firstCause:string='';private ownerV4:PrepareAttemptOwner|null=null;private consumedV4:boolean=false;private issuedV4:PrepareCheckTicket|null=null;
 private firstErrorV4:PrepareOwnerError|null=null;private lastErrorV4:PrepareOwnerError|null=null;private discardedV4:number=0;
 private resetOwner():void {this.ownerV4=null;this.consumedV4=false;this.issuedV4=null;this.firstCause='';this.firstErrorV4=null;this.lastErrorV4=null;this.discardedV4=0;}
 begin(now:number,allowed:boolean):boolean{const ok=super.begin(now,allowed);if(ok)this.resetOwner();return ok;}
 show():void {super.show();this.resetOwner();}
 hide():void {super.hide();this.ownerV4=null;this.issuedV4=null;}
 ownerSnapshot():PrepareAttemptOwner|null{return this.ownerV4===null?null:copyPrepareOwner(this.ownerV4);}
 discardOwnerCallback():void{if(this.discardedV4<Number.MAX_SAFE_INTEGER)this.discardedV4++;}
 ownerFailure(stage:string,code:number,name:string,message:string,before:PrepareAttemptOwner|null,after:PrepareAttemptOwner|null,id:number,generation:number,branch:number):void {
  if(!this.currentId(id,generation)||(this.ownerV4!==null&&this.ownerV4.branch!==branch)){this.discardOwnerCallback();return;}
  const r=new PrepareOwnerError();r.stage=prepareErrorText(stage,128,256);r.code=code;r.name=prepareErrorText(name,256,512);r.message=prepareErrorText(message,1024,2048);r.messageTruncated=r.message!==message;r.nameTruncated=r.name!==name;r.prepareId=id;r.pageGeneration=generation;r.branch=branch;
  const b=before===null?null:copyPrepareOwner(before),a=after===null?null:copyPrepareOwner(after);r.before=b===null?'UNKNOWN':prepareOwnerKey(b);r.after=a===null?'UNKNOWN':prepareOwnerKey(a);const beforeKey=r.before,afterKey=r.after;r.before=prepareErrorText(beforeKey,2048,1024);r.after=prepareErrorText(afterKey,2048,1024);r.ownerTruncated=r.before!==beforeKey||r.after!==afterKey;
  if(this.firstErrorV4===null)this.firstErrorV4=r;this.lastErrorV4=r;this.fail(stage);
 }
 bindOwner(o:PrepareAttemptOwner):boolean{
  if(this.state!=='PREPARING'&&this.state!=='READY')return false;
  const copy=copyPrepareOwner(o);
  if(copy!==null&&(copy.prepareId!==this.prepareId||copy.pageGeneration!==this.pageGeneration)){this.discardOwnerCallback();return false;}
  if(copy===null){this.ownerFailure('OWNER_IDENTITY_INVALID',0,'','Invalid or foreign owner DTO',this.ownerV4,copy,this.prepareId,this.pageGeneration,this.ownerV4?.branch??0);return false;}
  if(this.ownerV4!==null){if(!samePrepareOwner(this.ownerV4,copy)){this.invalidate('OWNER_CHANGED');return false;}return true;}
  if(this.serialV4>=Number.MAX_SAFE_INTEGER){this.fail('OWNER_SERIAL_EXHAUSTED');return false;}
  this.serialV4++;this.ownerV4=copy;return true;
 }
 noteChange(reason:string,before:PrepareAttemptOwner,after:PrepareAttemptOwner):void {
  const b=copyPrepareOwner(before),a=copyPrepareOwner(after);
  if(b===null||a===null){this.fail('OWNER_IDENTITY_INVALID');return;}
  if(!samePrepareOwner(b,a)){this.invalidate(reason);return;}super.changed(reason);
 }
 changed(reason:string):void{this.issuedV4=null;super.changed(reason);}
 sampleFrame(o:PrepareAttemptOwner):boolean{return this.bindOwner(o);}
 invalidate(reason:string):void{if(!this.firstCause)this.firstCause=reason;this.issuedV4=null;super.invalidate(reason);}
 fail(reason:string):void{if(!this.firstCause)this.firstCause=reason;this.issuedV4=null;super.fail(reason);}
 frame(frame:number,now:number):void {if(this.ownerV4===null){if(this.state==='PREPARING'&&this.elapsed(now))this.reason='OWNER_NOT_BOUND';return;}super.frame(frame,now);}
 check(snapshot:PrepareSnapshot,now:number):string {
  const reason=super.check(snapshot,now);if(reason)return reason;const o=this.ownerV4;
  if(o===null){this.invalidate('OWNER_NOT_BOUND');return 'OWNER_NOT_BOUND';}
  if(o.prepareId!==this.prepareId||o.pageGeneration!==this.pageGeneration||o.windowId!==snapshot.windowId||o.windowGeneration!==snapshot.windowGeneration||o.controller!==snapshot.controller||o.session!==snapshot.session||o.surface!==snapshot.surface||o.context!==snapshot.context||o.run!==snapshot.run){this.invalidate('OWNER_CHANGED');return 'OWNER_CHANGED';}
  return '';
 }
 checkForReserve(snapshot:PrepareSnapshot,now:number):PrepareCheckTicket{
  const t=new PrepareCheckTicket(),reason=this.check(snapshot,now),o=this.ownerV4;
  if(reason||this.consumedV4||o===null){t.reason=reason||'OWNER_NOT_BOUND';return t;}
  t.valid=true;t.prepareId=this.prepareId;t.pageGeneration=this.pageGeneration;t.observationSerial=this.observationSerial;t.inputSerial=this.inputSerial;t.ownerSerial=this.serialV4;t.owner=prepareOwnerKey(o);
  const saved=new PrepareCheckTicket();saved.valid=true;saved.prepareId=t.prepareId;saved.pageGeneration=t.pageGeneration;saved.observationSerial=t.observationSerial;saved.inputSerial=t.inputSerial;saved.ownerSerial=t.ownerSerial;saved.owner=t.owner;this.issuedV4=saved;return t;
 }
 consumeTicket(t:PrepareCheckTicket):boolean{
  if(t.prepareId!==this.prepareId||t.pageGeneration!==this.pageGeneration){this.discardOwnerCallback();return false;}
  const issued=this.issuedV4,o=this.ownerV4;
  if(!t.valid||this.state!=='READY'||this.consumedV4||issued===null||o===null||t.prepareId!==this.prepareId||t.pageGeneration!==this.pageGeneration||t.observationSerial!==this.observationSerial||t.inputSerial!==this.inputSerial||t.ownerSerial!==this.serialV4||t.owner!==prepareOwnerKey(o)||t.prepareId!==issued.prepareId||t.pageGeneration!==issued.pageGeneration||t.observationSerial!==issued.observationSerial||t.inputSerial!==issued.inputSerial||t.ownerSerial!==issued.ownerSerial||t.owner!==issued.owner){this.invalidate('PREPARE_TICKET_MISMATCH');return false;}
  this.consumedV4=true;this.issuedV4=null;super.consume();return true;
 }
 report():string{
  // One authority serialization and one same-call owner copy; no new observations or authorization.
  const authority=super.report(),owner=this.ownerSnapshot(),ownerSerial=this.serialV4;
  const parsed=JSON.parse(authority) as PrepareAuthorityReport;
  const evidence=buildPreparePairingEvidence(parsed,owner,ownerSerial);
  const first=this.firstErrorV4,last=this.lastErrorV4;
  let raw='{"revision":"T659_PREPARE_OWNER_VALUE_V1","subphase":'+JSON.stringify(this.subphase)+',"firstCause":'+JSON.stringify(this.firstCause)+',"owner":'+JSON.stringify(owner)+',"ownerKey":'+JSON.stringify(owner===null?'':prepareOwnerKey(owner))+',"ownerSerial":'+ownerSerial+',"discardedCallbacks":'+this.discardedV4+',"firstRawError":'+JSON.stringify(first)+',"lastRawError":'+JSON.stringify(last)+',"pairingEvidence":'+JSON.stringify(evidence)+',"authority":'+authority+'}';
  if(prepareUtf8Bytes(raw)<=14336)return raw;
  const reason=prepareErrorText(parsed.readinessReason??'',256,512),cause=prepareErrorText(this.firstCause,256,512);
  const stringsTruncated=reason!==(parsed.readinessReason??'')||cause!==this.firstCause;
  raw=JSON.stringify({revision:'T659_PREPARE_OWNER_VALUE_V1',detailTruncated:true,state:parsed.state??'UNKNOWN',reason:reason,prepareId:parsed.prepareId??null,pageGeneration:parsed.pageGeneration??null,ownerSerial:ownerSerial,firstCause:cause,summaryTextTruncated:stringsTruncated,firstRawError:first,lastRawError:last,pairingEvidence:evidence});
  if(prepareUtf8Bytes(raw)<=14336)return raw;
  compactPreparePairingEvidence(evidence);
  // Extreme simultaneous error/summary pressure is explicit INCOMPLETE, never an evidence PASS.
  raw=JSON.stringify({revision:'T659_PREPARE_OWNER_VALUE_V1',detailTruncated:true,state:'INCOMPLETE',reason:'PREPARE_COLD_UTF8_LIMIT',firstRawError:first,lastRawError:last,pairingEvidence:evidence});
  if(prepareUtf8Bytes(raw)<=14336)return raw;
  evidence.raster=new PairRaster();evidence.keyboard=new PairKeyboard();for(let i=0;i<4;i++)evidence.keyboard.avoid.push(new PairRect());
  // Identity loss stays individually named even under exceptional envelope pressure.
  evidence.missingFields=evidence.missingFields.filter((p:string)=>p.startsWith('gate.')||p.startsWith('owner')||p.startsWith('snapshot.')||p.startsWith('raster.native.'));evidence.missingFields.push('raster','keyboard','FIELD_LISTS_TRUNCATED_SEE_NULL_FIELDS');
  evidence.truncatedFields=evidence.truncatedFields.filter((p:string)=>p.startsWith('gate.')||p.startsWith('owner')||p.startsWith('snapshot.')||p.startsWith('raster.native.'));evidence.truncatedFields.push('raster','keyboard','missingFields','truncatedFields');
  raw=JSON.stringify({revision:'T659_PREPARE_OWNER_VALUE_V1',detailTruncated:true,state:'INCOMPLETE',reason:'PREPARE_COLD_UTF8_LIMIT',firstRawError:first,lastRawError:last,pairingEvidence:evidence});
  if(prepareUtf8Bytes(raw)<=14336)return raw;
  // Irreducible diagnostics still retain both raw errors and the same-call gate/window/owner identity.
  raw=JSON.stringify({revision:'T659_PREPARE_OWNER_VALUE_V1',detailTruncated:true,state:'INCOMPLETE',reason:'PREPARE_COLD_UTF8_LIMIT',firstRawError:first,lastRawError:last,pairingEvidence:minimumPreparePairingEvidence(evidence)});
  if(prepareUtf8Bytes(raw)>14336)throw new Error('PREPARE_COLD_UTF8_LIMIT');
  return raw;
 }
}
