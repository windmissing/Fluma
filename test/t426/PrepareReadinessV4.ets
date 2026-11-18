import { PrepareReadiness, PrepareSnapshot } from './PrepareReadiness';
export class PrepareAttemptOwner {
 prepareId:number=0;pageGeneration:number=0;branch:number=0;windowId:number=0;windowGeneration:number=0;controller:number=0;session:number=0;surface:number=0;context:number=0;run:string='';
 same(b:PrepareAttemptOwner):boolean{return JSON.stringify(this)===JSON.stringify(b);}
}
export class PrepareCheckTicket {valid:boolean=false;prepareId:number=0;pageGeneration:number=0;observationSerial:number=0;inputSerial:number=0;owner:string='';reason:string='';}
// One object owns the original strict predicate and permit. No side gate READY.
export class PrepareReadinessV4 extends PrepareReadiness {
 ownerSerial:number=0;
 subphase:string='DISMISSING';firstCause:string='';private ownerV4:PrepareAttemptOwner|null=null;private consumedV4:boolean=false;
 begin(now:number,allowed:boolean):boolean{const ok=super.begin(now,allowed);if(ok){this.ownerV4=null;this.consumedV4=false;this.firstCause='';}return ok;}
 bindOwner(o:PrepareAttemptOwner):boolean{
  if(this.state!=='PREPARING'&&this.state!=='READY')return false;
  if(!o.controller||!o.session||!o.surface||!o.context||!o.run)return false;
  if(this.ownerV4!==null&&!this.ownerV4.same(o)){this.invalidate('OWNER_CHANGED');return false;}
  if(this.ownerV4===null)this.ownerSerial++;this.ownerV4=JSON.parse(JSON.stringify(o)) as PrepareAttemptOwner;return true;
 }
 noteChange(reason:string,before:PrepareAttemptOwner,after:PrepareAttemptOwner):void{if(!before.same(after))this.invalidate(reason);else super.changed(reason);}
 changed(reason:string):void{if(this.ownerV4!==null)this.noteChange(reason,this.ownerV4,this.ownerV4);else super.changed(reason);}
 sampleFrame(o:PrepareAttemptOwner):boolean{return this.bindOwner(o);}
 show():void{super.show();this.ownerV4=null;this.consumedV4=false;this.firstCause='';}
 invalidate(reason:string):void{if(!this.firstCause)this.firstCause=reason;super.invalidate(reason);}
 checkForReserve(snapshot:PrepareSnapshot,now:number):PrepareCheckTicket{
  const t=new PrepareCheckTicket();const reason=this.check(snapshot,now);if(reason||this.consumedV4||this.ownerV4===null){t.reason=reason||'OWNER_NOT_BOUND';return t;}
  t.valid=true;t.prepareId=this.prepareId;t.pageGeneration=this.pageGeneration;t.observationSerial=this.observationSerial;t.inputSerial=this.inputSerial;t.owner=JSON.stringify(this.ownerV4);return t;
 }
 consumeTicket(t:PrepareCheckTicket):boolean{
  if(!t.valid||this.state!=='READY'||this.consumedV4||t.prepareId!==this.prepareId||t.pageGeneration!==this.pageGeneration||t.observationSerial!==this.observationSerial||t.inputSerial!==this.inputSerial||t.owner!==JSON.stringify(this.ownerV4))return false;
  this.consumedV4=true;super.consume();return true;
 }
 report():string{return '{"revision":"T598_PREPARE_TRANSACTION_V4","subphase":'+JSON.stringify(this.subphase)+',"firstCause":'+JSON.stringify(this.firstCause)+',"owner":'+JSON.stringify(this.ownerV4)+',"authority":'+super.report()+'}';}
}
