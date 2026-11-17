import { TimelineStore,ColdFile } from './GpuBridgeTimelineCold';
import { ColdScope } from './GpuBoundedCold';
import { utf8Bytes } from './GpuUtf8Budget';
export interface ResourcePort538 { resourceState():string;cacheStorageState():string;coldCapabilities():string;resourceSampleBeginV1(label:string,scope:string):string;resourceSampleEndV1(id:string):string; }
interface Capability538 { scope:ColdScope; }
interface SampleBegin538 { state:string;sampleId:string; }
export class ResourceSample538 {
 revision:string='T531_RESOURCE_SAMPLE_V1';sampleId:string='';label:string='';state:string='INCOMPLETE';reason:string='NOT_STARTED';
 evidenceOrigin:string='PRODUCTION_READ_ONLY';physicsValidated:boolean=false;driverAccounting:string='DRIVER_NOT_MEASURED';
 packageReceipt:string='NOT_MEASURED';packageInputClaim:string='';elfReceipt:string='NOT_MEASURED';
 nativeBeginRaw:string='';nativeEndRaw:string='';scopeBeforeRaw:string='';scopeAfterRaw:string='';scopeStable:boolean=false;files:ColdFile[]=[];bytes:number=0;
}
export class GpuResourceSnapshotCold {
 private busy:boolean=false;private serial:number=0;
 async collect(port:ResourcePort538,store:TimelineStore,label:string,claim:string):Promise<ResourceSample538>{
  if(this.busy)throw new Error('RESOURCE_EXPORT_BUSY');this.busy=true;const out=new ResourceSample538();
  out.sampleId=String(++this.serial)+'-'+Date.now();out.label=label.substring(0,80);out.packageInputClaim=claim.substring(0,128);
  const prefix='resource-'+out.sampleId;let nativeId:string='';
  try{
   out.scopeBeforeRaw=port.coldCapabilities();const capability=JSON.parse(out.scopeBeforeRaw) as Capability538;out.nativeBeginRaw=port.resourceSampleBeginV1(out.label,JSON.stringify(capability.scope));const begin=JSON.parse(out.nativeBeginRaw) as SampleBegin538;if(begin.state!=='REQUESTED')throw new Error(begin.state);nativeId=begin.sampleId;
   for(const kind of ['resource','cache']){
    const raw=kind==='resource'?port.resourceState():port.cacheStorageState();const n=utf8Bytes(raw);
    if(n>1048576||out.bytes+n>2080768)throw new Error('RESOURCE_UTF8_BUDGET');
    JSON.parse(raw);const file=await store.save(prefix+'-'+kind+'.json',raw);
    if(file.bytes!==n)throw new Error('UTF8_FILE_BYTES');out.files.push(file);out.bytes+=n;
    console.info('WS_EVENT RESOURCE_FILE '+file.name+' bytes='+file.bytes+' sha256='+file.sha256);
    if(raw.indexOf('INCOMPLETE_UTF8_LIMIT')>=0)throw new Error('NATIVE_RESOURCE_UTF8_LIMIT');
   }
   out.scopeAfterRaw=port.coldCapabilities();
   const before=JSON.parse(out.scopeBeforeRaw) as Capability538;const after=JSON.parse(out.scopeAfterRaw) as Capability538;
   out.scopeStable=JSON.stringify(before.scope)===JSON.stringify(after.scope);
   if(!out.scopeStable)throw new Error('INCOMPLETE_SCOPE_CHANGED');out.state='COMPLETE';out.reason='READ_ONLY_NO_DRIVER_CLAIM';
  }catch(error){out.reason=String(error);}
  finally{
   if(nativeId){try{out.nativeEndRaw=port.resourceSampleEndV1(nativeId);const end=JSON.parse(out.nativeEndRaw) as Capability538;const before=JSON.parse(out.scopeBeforeRaw) as Capability538;if(JSON.stringify(end.scope)!==JSON.stringify(before.scope)){out.scopeStable=false;out.state='INCOMPLETE';out.reason='INCOMPLETE_SCOPE_CHANGED';}}catch(error){out.state='INCOMPLETE';out.reason+=';RELEASE_FAILED:'+String(error);}}
   try{const raw=JSON.stringify(out);if(out.bytes+utf8Bytes(raw)>2097152)throw new Error('MANIFEST_BUDGET');const file=await store.save(prefix+'-manifest.json',raw);console.info('WS_EVENT RESOURCE_MANIFEST '+file.name+' bytes='+file.bytes+' sha256='+file.sha256);}
   finally{this.busy=false;}
  }
  return out;
 }
}
