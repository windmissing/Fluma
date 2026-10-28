import common from '@ohos.app.ability.common';import Want from '@ohos.app.ability.Want';import worker from '@ohos.worker';import fs from '@ohos.file.fs';import hilog from '@ohos.hilog';
import { T159_SOURCE_SHA,T159_SHADER_SHA,T191_BUILD_VARIANT } from '../T159BuildIdentity';
import { T171_FIXTURES } from '../T171Fixtures';import { TxGpuCommand } from './TxGpuWorker';
export class T165PageLauncher {
  private context:common.UIAbilityContext|null=null;private want:Want|null=null;private shown:boolean=false;private started:boolean=false;private work:worker.ThreadWorker|null=null;
  configure(context:common.UIAbilityContext,want:Want|null):void{this.context=context;this.want=want;this.start();}
  pageReady():void{this.shown=true;this.start();}
  private start():void{if(this.started||!this.shown||this.context===null)return;this.started=true;this.run();}
  private async run():Promise<void>{try{
    const context=this.context!,p=this.want?.parameters;if(p===undefined)throw new Error('TX_GPU_WANT');
    const backend=p['txBackend'],run=p['txRunId'],build=p['txUnsignedSha'],fixtureId=p['txFixtureId'],steps=p['txSteps'];
    if(typeof run!=='string'||!/^[a-zA-Z0-9_-]{1,64}$/.test(run)||typeof backend!=='string'||['LEGACY_SCAN','TX_ADJ_AVL','GPU_PAIR_V1'].indexOf(backend)<0||typeof build!=='string'||!/^[0-9a-f]{64}$/.test(build)||typeof fixtureId!=='string'||typeof steps!=='number')throw new Error('TX_GPU_WANT_FIELDS');
    const f=T171_FIXTURES.find((x):boolean=>x.fixtureId===fixtureId);if(f===undefined)throw new Error('TX_GPU_FIXTURE');
    const directory=context.cacheDir+'/txgpu-'+run;if(!fs.accessSync(directory))fs.mkdirSync(directory);
    const names=['pair_build_v1.comp','pair_query_v1.comp','pair_reduce_v1.comp'];for(let j=0;j<3;j++){
      const bytes=await context.resourceManager.getRawFileContent('t141/glsl/'+names[j]),file=fs.openSync(directory+'/'+names[j],fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
      try{let offset=0;while(offset<bytes.byteLength){const n=fs.writeSync(file.fd,bytes.buffer.slice(bytes.byteOffset+offset,bytes.byteOffset+bytes.byteLength));if(n<=0)throw new Error('TX_GPU_SHADER_SHORT_WRITE');offset+=n;}fs.fsyncSync(file.fd);}finally{fs.closeSync(file);}
    }
    const configId='TX_GPU_PAIR_EXPERIMENT_V1|backend:'+backend+'|variant:'+T191_BUILD_VARIANT+'|fixture:'+f.fixtureSha256+'|roles:1seed,2collect,3preview,4authoritative,5explicit|policy:TX_EXACT_STRUCTURE_V1';
    const c:TxGpuCommand={revision:'TX_GPU_COMMAND_V1',fixtureId:fixtureId,steps:steps,output:context.filesDir+'/txgpu-'+run+'.json',config:{nmax:f.nmax,shaderDirectory:directory,runId:run,configId:configId,sourceManifestSha256:T159_SOURCE_SHA,buildManifestSha256:build,buildShaderSha256:T159_SHADER_SHA[0],queryShaderSha256:T159_SHADER_SHA[1],reduceShaderSha256:T159_SHADER_SHA[2],diagnosticOracle:true,fullDump:true,backend:backend,policy:'TX_EXACT_STRUCTURE_V1',traceCapacity:16384,variant:T191_BUILD_VARIANT}};
    this.work=new worker.ThreadWorker('entry/ets/physics/tx/TxGpuWorker.ets');this.work.onmessage=(e):void=>{hilog.info(0,'TXGPU','%{public}s',JSON.stringify(e.data));};this.work.postMessage(c);
  }catch(error){hilog.error(0,'TXGPU','FAULT %{public}s',(error as Error).message);}}
}
export const t165PageLauncher:T165PageLauncher=new T165PageLauncher();
