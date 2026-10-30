import { t276Hash } from './T276Export';
export interface T284Item {id:string;payload:string;part:number;parts:number;digest:number;}
export interface T284Index {revision:string;anchor:string;status:string;reason:string;rawBytes:number;parts:number;emitted:number;captureMs:number;exportMs:number;hashes:string[];}
// Cold diagnostic limits. UTF16LE payload and physical ASCII line budgets are
// accounted separately; normal frames never enqueue or format a large object.
export class T284Cold {
 readonly maxAnchorBytes:number=262144;readonly maxAnchorParts:number=512;
 readonly maxRunBytes:number=1048576;readonly maxRunParts:number=2048;
 readonly maxQueuedBytes:number=524288;readonly maxItems:number=16;
 readonly maxCaptureMs:number=4;readonly maxBatchMs:number=2;
 readonly maxExportMs:number=200;readonly maxBatchParts:number=4;
 private queue:T284Item[]=[];private queuedBytes:number=0;private runBytes:number=0;private runParts:number=0;
 private run:string='';private epoch:number=-1;private seen:string[]=[];private birthDue:number=-1;private pending:string[]=[];
 private anchor:string='';private anchorBytes:number=0;private anchorParts:number=0;private captureBegin:number=0;
 private indices:T284Index[]=[];private current:T284Index|null=null;private incomplete:boolean=false;
 now:()=>number=():number=>Date.now();emit:(line:string)=>void=(line:string):void=>{console.info(line);};
 enabled:boolean=false;manualFrame:number=-1;private manualUsed:boolean=false;
 bind(run:string,epoch:number):void {if(run===this.run&&epoch===this.epoch)return;if(this.queue.length>0){for(const i of this.indices)if(i.status==='PENDING'){i.status='INCOMPLETE';i.reason='RUN_CHANGED';}this.emitIndex();}this.queue=[];this.queuedBytes=0;this.runBytes=0;this.runParts=0;this.run=run;this.epoch=epoch;this.seen=[];this.pending=[];this.birthDue=-1;this.anchor='';this.current=null;this.indices=[];this.incomplete=false;this.manualUsed=false;}
 signal(reason:string,frame:number):void {if(!this.enabled||this.seen.indexOf(reason)>=0)return;this.seen.push(reason);if(reason==='FIRST_BIRTH')this.birthDue=frame+1;else this.pending.push(reason);}
 select(frame:number,commit:number):string {if(!this.enabled)return '';if(this.birthDue>=0&&frame>=this.birthDue){this.pending.push('FIRST_BIRTH');this.birthDue=-1;}if(!this.manualUsed&&frame===this.manualFrame){this.manualUsed=true;this.pending.push('MANUAL');}if(this.pending.length===0)return '';const reason=this.pending.shift()??'';if(this.indices.length>=4){this.reject('ANCHOR_LIMIT');return '';}
  this.anchor=this.run+'|'+this.epoch+'|'+frame+'|'+commit+'|'+reason;this.anchorBytes=this.anchorParts=0;this.captureBegin=this.now();this.current={revision:'T284_COLD_INDEX_V1',anchor:this.anchor,status:'PENDING',reason:'NONE',rawBytes:0,parts:0,emitted:0,captureMs:0,exportMs:0,hashes:[]};this.indices.push(this.current);return reason;}
 private reject(reason:string):void {this.incomplete=true;if(this.current!==null){this.current.status='INCOMPLETE';this.current.reason=reason;}this.emit('T284_BUDGET '+JSON.stringify({revision:'T284_COLD_BUDGET_V1',anchor:this.anchor,status:'INCOMPLETE',reason:reason}));}
 enqueue(kind:string,payload:string):void {if(this.current===null||this.anchor===''||this.current.status==='INCOMPLETE')return;const bytes=payload.length*2,parts=Math.max(1,Math.ceil(payload.length/256));
  if(this.now()-this.captureBegin>this.maxCaptureMs){this.reject('BUDGET_CAPTURE_MS');return;}
  if(bytes>this.maxAnchorBytes-this.anchorBytes||parts>this.maxAnchorParts-this.anchorParts){this.reject('BUDGET_ANCHOR');return;}
  if(bytes>this.maxRunBytes-this.runBytes||parts>this.maxRunParts-this.runParts){this.reject('BUDGET_RUN');return;}
  if(bytes>this.maxQueuedBytes-this.queuedBytes||this.queue.length===this.maxItems){this.reject('BUDGET_QUEUE');return;}
  const id=kind+'|'+this.anchor,digest=t276Hash(payload);this.queue.push({id:id,payload:payload,part:0,parts:parts,digest:digest});this.queuedBytes+=bytes;this.runBytes+=bytes;this.runParts+=parts;this.anchorBytes+=bytes;this.anchorParts+=parts;this.current.rawBytes=this.anchorBytes;this.current.parts=this.anchorParts;this.current.hashes.push(kind+':'+digest);
 }
 finish():void {if(this.current!==null){this.current.captureMs=this.now()-this.captureBegin;if(this.current.captureMs>this.maxCaptureMs)this.reject('BUDGET_CAPTURE_MS');if(this.current.parts===0&&this.current.status==='PENDING'){this.current.status='INCOMPLETE';this.current.reason='NOT_CAPTURED';}}this.anchor='';this.current=null;this.emitIndex();}
 // Deferred callback only; a frame callback never drains. A busy/source-held
 // view defers output until stop/pause. Strings retain original token/commit.
 drain(idle:boolean):number {if(!idle)return 0;const begin=this.now();let emitted=0;while(this.queue.length>0&&emitted<this.maxBatchParts&&this.now()-begin<this.maxBatchMs){const item=this.queue[0],index=this.indices.find((v:T284Index):boolean=>item.id.endsWith('|'+v.anchor));if(index===undefined){this.queue.shift();continue;}if(index.status==='INCOMPLETE'){this.queuedBytes-=item.payload.length*2;this.queue.shift();continue;}if(index.exportMs>=this.maxExportMs){index.status='INCOMPLETE';index.reason='BUDGET_EXPORT_MS';this.queuedBytes-=item.payload.length*2;this.queue.shift();this.incomplete=true;continue;}
   const before=this.now(),text=item.payload.slice(item.part*256,(item.part+1)*256);let hex='';for(let j=0;j<text.length;j++)hex+=text.charCodeAt(j).toString(16).padStart(4,'0');const raw=JSON.stringify({revision:'T276_COLD_UTF16LE_V1',exportId:item.id,partIndex:item.part,partCount:item.parts,byteCount:text.length*2,totalBytes:item.payload.length*2,digest:item.digest,partDigest:t276Hash(text),hex:hex});if(raw.length>1800){index.status='INCOMPLETE';index.reason='BUDGET_LINE';this.queue.shift();this.queuedBytes-=item.payload.length*2;continue;}this.emit('T276_COLD_V1 '+raw);const cost=this.now()-before;index.exportMs+=cost;index.emitted++;item.part++;emitted++;if(item.part===item.parts){this.queuedBytes-=item.payload.length*2;this.queue.shift();}if(cost>this.maxBatchMs){index.status='INCOMPLETE';index.reason='BUDGET_BATCH_MS';this.incomplete=true;}if(index.emitted===index.parts&&index.status==='PENDING')index.status='COMPLETE';}
  if(emitted>0)this.emitIndex();return emitted;}
 emitIndex():void {for(const v of this.indices)this.emit('T284_INDEX '+JSON.stringify(v));}
 summary():string {return JSON.stringify({revision:'T284_COLD_QUEUE_V1',queuedItems:this.queue.length,queuedBytes:this.queuedBytes,runBytes:this.runBytes,runParts:this.runParts,incomplete:this.incomplete,indices:this.indices});}
}
export const t284Cold:T284Cold=new T284Cold();
