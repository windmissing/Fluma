// T165 isolated page/loadContent rendezvous; creation timing is a candidate, not a proven root cause.
import common from '@ohos.app.ability.common';
import Want from '@ohos.app.ability.Want';
import worker from '@ohos.worker';
import fs from '@ohos.file.fs';
import hilog from '@ohos.hilog';
import { T159_SOURCE_SHA,T159_SHADER_SHA } from './T159BuildIdentity';
export class T165PageLauncher {
  private physics: worker.ThreadWorker | null = null;
  private launch: Want | null = null;
  private context: common.UIAbilityContext | null = null;
  private pageShown: boolean = false;
  private started: boolean = false;
  // Either lifecycle notification may arrive first. Both are mandatory, and launch is one-shot.
  configure(context: common.UIAbilityContext, want: Want | null): void {
    this.context=context; this.launch=want; this.tryStart();
  }
  pageReady(): void { this.pageShown=true; this.tryStart(); }
  private tryStart(): void {
    if(!this.pageShown||this.context===null||this.started)return;
    this.started=true;
    this.run();
  }
  private async run(): Promise<void> {
    try {
      const p=this.launch?.parameters;
      if(p===undefined)throw new Error('COLD_CONFIG_REQUIRED');
      const build=p['t159buildManifestSha256'],route=p['t159route'],run=p['t159runId'];
      const nmax=p['t159nmax'],steps=p['t159steps'];
      if(typeof build!=='string'||!/^[0-9a-f]{64}$/.test(build)||typeof run!=='string'||!/^[a-zA-Z0-9_-]{1,64}$/.test(run)||
        typeof route!=='string'||(route!=='GPU'&&route!=='T123'&&route!=='LEGACY_SCAN')||typeof nmax!=='number'||typeof steps!=='number')throw new Error('COLD_CONFIG_INVALID');
      const directory=this.context!.cacheDir+'/t159-shaders';
      if(!fs.accessSync(directory))fs.mkdirSync(directory);
      const names=['pair_build_v1.comp','pair_query_v1.comp','pair_reduce_v1.comp'];
      for(let i=0;i<names.length;i++) {
        const bytes=await this.context!.resourceManager.getRawFileContent('t141/glsl/'+names[i]);
        const file=fs.openSync(directory+'/'+names[i],fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
        try {let offset=0;while(offset<bytes.byteLength){const n=fs.writeSync(file.fd,bytes.buffer.slice(bytes.byteOffset+offset,bytes.byteOffset+bytes.byteLength));if(n<=0)throw new Error('SHADER_SHORT_WRITE');offset+=n;}fs.fsyncSync(file.fd);}
        finally {fs.closeSync(file);}
      }
      this.physics=new worker.ThreadWorker('entry/ets/workers/T159PhysicsWorker.ets');
      this.physics.onmessage=(event): void => { hilog.info(0,'T159','%{public}s',JSON.stringify(event.data)); };
      this.physics.onerror=(event): void => { hilog.error(0,'T159','WORKER_FAULT %{public}s',event.message); };
      this.physics.postMessage({type:'run',route:route,steps:steps,queuedAt:Date.now(),output:this.context!.filesDir+'/t159-'+run+'-'+route+'.jsonl',
        config:{nmax:nmax,shaderDirectory:directory,buildManifestSha256:build,sourceManifestSha256:T159_SOURCE_SHA,runId:run,
          configId:'T159_WET_FULL_POOL_V2|nmax:'+nmax.toString()+'|steps:'+steps.toString(),diagnosticOracle:p['t159diagnosticOracle']!==false,
          buildShaderSha256:T159_SHADER_SHA[0],queryShaderSha256:T159_SHADER_SHA[1],reduceShaderSha256:T159_SHADER_SHA[2]}});
    } catch(error) { hilog.error(0,'T159','FAULT %{public}s',(error as Error).message); }
  }
}
export const t165PageLauncher: T165PageLauncher = new T165PageLauncher();
