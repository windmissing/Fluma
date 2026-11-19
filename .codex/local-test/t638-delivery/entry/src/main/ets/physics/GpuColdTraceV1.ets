import { ColdFile,TimelineStore } from './GpuBridgeTimelineCold';
import { BoundedColdPort,ColdScope,coldCapability } from './GpuBoundedCold';
import { utf8Bytes } from './GpuUtf8Budget';
interface TraceReceipt {revision:string;state:string;requestId:string;traceId:string;scope:ColdScope;rawGl?:number;rawEgl?:number;failedStage?:string;}
interface TracePage {revision:string;payloadRevision:string;state:string;traceId:string;scope:ColdScope;page:number;pages:number;actualWords:number;stateWords:number;eventWords:number;rootKind:number;stride:number;rows:number;rootToken:string;arena:number;generationLo:number;generationHi:number;words:number[];}
let traceExportSerial:number=0;
export class ColdTraceManifest {
 revision:string='T503_M2_COLD_ARCHIVE_V1';state:string='INCOMPLETE';reason:string='NOT_STARTED';requestId:string='';traceId:string='';scope:ColdScope|null=null;
 release:string='NOT_REQUESTED';files:ColdFile[]=[];bytes:number=0;words:number=0;header:TracePage|null=null;rejectedPage:ColdFile|null=null;nativeReceipt:string='';
}
export class GpuColdTraceV1 {
 busy:boolean=false;private cancelled:boolean=false;
 cancel():void{this.cancelled=true;}
 private u64(lo:number,hi:number):string {let low=lo,high=hi,text='';do{const rest=(high%10)*4294967296+low;const digit=rest%10;text=String(digit)+text;low=Math.floor(rest/10);high=Math.floor(high/10);}while(low!==0||high!==0);return text;}
 private page(raw:string,id:string,page:number,reference:TracePage|null):TracePage {
  if(utf8Bytes(raw)>16384)throw new Error('TRACE_PAGE_BUDGET');
  const p:TracePage=JSON.parse(raw) as TracePage;
  if(p.revision!=='T503_M2_COLD_TRACE_V1'||p.payloadRevision!=='T445_M2_TRACE_V3'||p.state!=='TRACE_READY'||p.traceId!==id||p.page!==page||!Number.isSafeInteger(p.actualWords)||p.actualWords<256||p.actualWords>27705344/4||!Number.isSafeInteger(p.stateWords)||p.stateWords<=0||!Number.isSafeInteger(p.eventWords)||p.eventWords<0||p.actualWords!==256+p.stateWords+p.eventWords||p.pages!==Math.ceil(p.actualWords/1024)||!Number.isSafeInteger(p.rows)||p.rows<0||(p.rootKind!==1&&p.rootKind!==2)||p.stride!==(p.rootKind===1?128:64)||p.eventWords!==p.rows*p.stride||!Number.isInteger(p.arena)||p.arena<0||p.arena>1||!Number.isSafeInteger(p.generationLo)||p.generationLo<0||p.generationLo>4294967295||!Number.isSafeInteger(p.generationHi)||p.generationHi<0||p.generationHi>4294967295||!Array.isArray(p.words)||p.words.length!==(page===0?0:Math.min(1024,p.actualWords-(page-1)*1024)))throw new Error('TRACE_FORMAT_REJECTED');
  if(reference!==null&&(JSON.stringify(p.scope)!==JSON.stringify(reference.scope)||p.rootToken!==reference.rootToken||p.actualWords!==reference.actualWords||p.stateWords!==reference.stateWords||p.eventWords!==reference.eventWords||p.rootKind!==reference.rootKind||p.rows!==reference.rows||p.arena!==reference.arena||p.generationLo!==reference.generationLo||p.generationHi!==reference.generationHi))throw new Error('TRACE_SCOPE_OR_DESCRIPTOR_CHANGED');
  for(const word of p.words)if(!Number.isSafeInteger(word)||word<0||word>4294967295)throw new Error('TRACE_INVALID_U32');
  if(page===1){
   const c:number[]=p.words;if(c[1]!==3||c[27]!==p.rootKind||c[p.rootKind===1?169:53]!==p.rows)throw new Error('TRACE_COMPLETION_IDENTITY');
   let token:string='GRS_GPU_ROOT_Q20_M2_V3';for(let i:number=4;i<28;i++)token+='|'+String(c[i]);for(let i:number=32;i<42;i++)token+='|'+String(c[i]);for(let i:number=114;i<138;i++)token+='|'+String(c[i]);for(let i:number=160;i<173;i++)token+='|'+String(c[i]);
   if(token!==p.rootToken)throw new Error('TRACE_ROOT_TOKEN');
   if(c[36]!==p.generationLo||c[37]!==p.generationHi||c[p.rootKind===1?172:51]!==p.arena||this.u64(c[6],c[7])!==p.scope.control.run||this.u64(c[10],c[11])!==p.scope.control.context||this.u64(c[12],c[13])!==p.scope.control.surface)throw new Error('TRACE_COMPLETION_IDENTITY');
   const k=c[88]+c[89],kw=c[90]+c[91];if((k!==192&&k!==512)||(kw!==64&&kw!==128)||p.stateWords!==16*(k+kw)+2304)throw new Error('TRACE_PROFILE');
  }return p;
 }
 async collect(port:BoundedColdPort,store:TimelineStore,replaceId:string='0',yieldUi:()=>Promise<void>=()=>new Promise<void>((resolve:()=>void)=>{setTimeout(resolve,10);})):Promise<ColdTraceManifest>{
  if(this.busy)throw new Error('EXPORT_BUSY');this.busy=true;this.cancelled=false;const out=new ColdTraceManifest();const prefix='gpu-cold-trace-'+Date.now()+'-'+String(++traceExportSerial);let raw:string='';let budget:number=0;const start:number=Date.now();
  try{
   const caps=coldCapability(port);if(!caps.traceSupported)throw new Error('UNSUPPORTED_ROUTE');out.scope=caps.scope;
   raw=port.coldTraceBeginV1(JSON.stringify(caps.scope),replaceId);const begin=JSON.parse(raw) as TraceReceipt;out.nativeReceipt=raw;
   if(/^[1-9][0-9]{0,19}$/.test(begin.traceId)){out.traceId=begin.traceId;out.requestId=begin.requestId;}
   if(begin.state!=='REQUESTED'||!out.traceId)throw new Error(begin.state);raw='';
   while(true){
    if(this.cancelled)throw new Error('CANCELLED');if(Date.now()-start>=60000)throw new Error('EXPIRED');
    const status:string=port.coldTraceStateV1(out.requestId);out.nativeReceipt=status;const receipt=JSON.parse(status) as TraceReceipt;
    if(receipt.traceId!==out.traceId||JSON.stringify(receipt.scope)!==JSON.stringify(out.scope))throw new Error('TRACE_SCOPE_CHANGED');
    if(receipt.state==='TRACE_READY')break;if(receipt.state!=='REQUESTED'&&receipt.state!=='BUILDING')throw new Error(receipt.state);await yieldUi();
   }
   raw=port.coldTracePageV1(out.traceId,0);const header=this.page(raw,out.traceId,0,null);out.header=header;
   if(JSON.stringify(header.scope)!==JSON.stringify(out.scope))throw new Error('TRACE_SCOPE_CHANGED');
   budget=(header.pages+1)*16384;
   const save=async(page:number,text:string):Promise<void>=>{const bytes=utf8Bytes(text);if(bytes>16384||out.bytes+bytes>budget)throw new Error('TRACE_ARCHIVE_BUDGET');const saved=await store.save(prefix+'-'+page+'.json',text);if(saved.bytes!==bytes)throw new Error('UTF8_FILE_BYTES');out.files.push(saved);out.bytes+=bytes;};
   await save(0,raw);raw='';
   for(let page:number=1;page<=header.pages;page++){
    if(this.cancelled)throw new Error('CANCELLED');if(Date.now()-start>=60000)throw new Error('EXPIRED');
    raw=port.coldTracePageV1(out.traceId,page);const parsed=this.page(raw,out.traceId,page,header);await save(page,raw);out.words+=parsed.words.length;raw='';await yieldUi();
   }
   if(out.words!==header.actualWords)throw new Error('MISSING_TRACE_WORDS');out.state='COMPLETE';out.reason='STREAMED_COMPLETE';
  }catch(error){
   if(raw&&utf8Bytes(raw)<=16384&&(budget===0||out.bytes+utf8Bytes(raw)<=budget)){try{out.rejectedPage=await store.save(prefix+'-rejected-page.json',raw);}catch(io){out.reason='REJECTED_STORE:'+String(io);}}
   out.state='INCOMPLETE';out.reason=String(error)+';'+out.reason;
  }finally{
   if(out.traceId){try{out.release=port.coldTraceEndV1(out.traceId);}catch(error){out.release='RELEASE_FAILED:'+String(error);}}
   try{out.files.push(await store.save(prefix+'-manifest.json',JSON.stringify(out)));}finally{this.busy=false;}
  }return out;
 }
}
