// One fixture owner and one native NAPI env/EGL context per Worker. No shared
// XComponent context. Concurrent owners must use separate Workers/envs.
import worker,{MessageEvents} from '@ohos.worker';
import fs from '@ohos.file.fs';import util from '@ohos.util';import time from '@ohos.systemDateTime';
import gpu from 'libp011_gpu_pair_prototype.so';
import { runTxGpuFixture } from './TxGpuFixture';
import { TxGpuConfig,TxNativeReply,TxNativeTransport } from './TxPairBackends';
import { P011ParticleSnapshot } from '../P011ParticleField';
import { TxGpuContextV2,TxGpuCompatWork } from './TxGpuCompatWork';
import { T159_SOURCE_SHA } from '../T159BuildIdentity';
export interface TxGpuCommand {revision:string;fixtureId:string;steps:number;config:TxGpuConfig;output:string;}
class NativeTransport implements TxNativeTransport {
  configure(c:TxGpuConfig):TxNativeReply{return gpu.configure(c) as TxNativeReply;}
  selectPair(p:P011ParticleSnapshot,radius:number,c:TxGpuContextV2,w:TxGpuCompatWork):TxNativeReply{return gpu.selectPair(p,radius,c,w) as TxNativeReply;}
  reset():TxNativeReply{return gpu.reset() as TxNativeReply;}
}
function now():number{return time.getUptime(time.TimeType.ACTIVE,false);}
const port=worker.workerPort;let active=false;let used=false;
port.onmessage=(event:MessageEvents):void=>{
  if(active||used){port.postMessage({status:'FAULT',error:'TX_GPU_ONE_OWNER_PER_WORKER_CREATE_NEW_WORKER'});return;}active=true;used=true;const c=event.data as TxGpuCommand;let physicalStatus='NOT_STARTED';
  try{
    if(c.revision!=='TX_GPU_COMMAND_V1'||c.config.sourceManifestSha256!==T159_SOURCE_SHA)throw new Error('TX_GPU_SOURCE_IDENTITY');
    const start=now(),result=runTxGpuFixture(c.config,c.fixtureId,c.steps,c.config.backend==='GPU_PAIR_V1'?new NativeTransport():null,now),encodeStart=now();
    physicalStatus=result.physicalStatus;const bytes=new util.TextEncoder().encodeInto(JSON.stringify({command:c,result:result})+'\n'),encodeMs=now()-encodeStart,writeStart=now();
    const f=fs.openSync(c.output,fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);try{let offset=0;while(offset<bytes.byteLength){const n=fs.writeSync(f.fd,bytes.buffer.slice(offset));if(n<=0)throw new Error('TX_GPU_SHORT_WRITE');offset+=n;}fs.fsyncSync(f.fd);}finally{fs.closeSync(f);}
    port.postMessage({revision:'TX_GPU_ARCHIVE_V1',status:'SEALED',physicalStatus:physicalStatus,retirement:result.retirement,source:T159_SOURCE_SHA,variant:c.config.variant,backend:c.config.backend,output:c.output,physicalMs:result.physicalMs,archiveEncodeMs:encodeMs,archiveWriteFsyncMs:now()-writeStart,bytes:bytes.byteLength,serviceMs:now()-start,gpuTimer:'NOT_MEASURED'});
  }catch(error){port.postMessage({revision:'TX_GPU_ARCHIVE_V1',status:'FAULT',physicalStatus:physicalStatus,archiveStatus:'FAILED',error:(error as Error).message});}
  finally{active=false;}
};
