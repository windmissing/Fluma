// V3 retains the T165 page+loadContent rendezvous, with a separate cold protocol.
import common from '@ohos.app.ability.common';
import Want from '@ohos.app.ability.Want';
import worker from '@ohos.worker';
import fs from '@ohos.file.fs';
import hilog from '@ohos.hilog';
import systemDateTime from '@ohos.systemDateTime';
import { T159_SOURCE_SHA,T159_SHADER_SHA } from './T159BuildIdentity';
import { T171_FIXTURES } from './T171Fixtures';
import { T171Command,t171ConfigId } from './T171Protocol';
export class T165PageLauncher {
 private context:common.UIAbilityContext|null=null;private launch:Want|null=null;
 private shown:boolean=false;private started:boolean=false;private physics:worker.ThreadWorker|null=null;
 configure(context:common.UIAbilityContext,want:Want|null):void{this.context=context;this.launch=want;this.start();}
 pageReady():void{this.shown=true;this.start();}
 private start():void{if(!this.shown||this.context===null||this.started)return;this.started=true;this.run();}
 private async run():Promise<void>{
  try{
   if(this.context===null)throw new Error('V3_CONTEXT');const context=this.context,p=this.launch?.parameters;
   if(p===undefined)throw new Error('V3_COLD_CONFIG_REQUIRED');
   const run=p['t171runId'],build=p['t171buildManifestSha256'],fixtureId=p['t171fixtureId'],mode=p['t171mode'],steps=p['t171steps'];
   const fault=p['t171fault']===undefined?'NONE':p['t171fault'];
   if(typeof run!=='string'||!/^[a-zA-Z0-9_-]{1,64}$/.test(run)||typeof build!=='string'||!/^[0-9a-f]{64}$/.test(build)||typeof fixtureId!=='string'||typeof mode!=='string'||typeof steps!=='number'||typeof fault!=='string')throw new Error('V3_COLD_CONFIG_INVALID');
   const fixture=T171_FIXTURES.find((f):boolean=>f.fixtureId===fixtureId);if(fixture===undefined)throw new Error('V3_FIXTURE_NOT_WHITELISTED');
   const directory=context.cacheDir+'/t171-'+run;if(!fs.accessSync(directory))fs.mkdirSync(directory);
   const recoveryDirectory=directory+'/original';if(!fs.accessSync(recoveryDirectory))fs.mkdirSync(recoveryDirectory);
   const names=['pair_build_v1.comp','pair_query_v1.comp','pair_reduce_v1.comp'];
   for(let i=0;i<3;i++){
    const original=await context.resourceManager.getRawFileContent('t141/glsl/'+names[i]);
    const clean=fs.openSync(recoveryDirectory+'/'+names[i],fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
    try{let offset=0;while(offset<original.byteLength){const n=fs.writeSync(clean.fd,original.buffer.slice(original.byteOffset+offset,original.byteOffset+original.byteLength));if(n<=0)throw new Error('V3_SHADER_SHORT_WRITE');offset+=n;}fs.fsyncSync(clean.fd);}finally{fs.closeSync(clean);}
    // Only this run's private copy changes. Expected source SHA remains the build-bound original.
    const changed=new Uint8Array(original.length+(fault==='SHADER_SHA_MISMATCH'&&i===0?1:0));changed.set(original);
    if(changed.length>original.length)changed[changed.length-1]=10;
    const file=fs.openSync(directory+'/'+names[i],fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
    try{let offset=0;while(offset<changed.byteLength){const n=fs.writeSync(file.fd,changed.buffer.slice(offset));if(n<=0)throw new Error('V3_SHADER_SHORT_WRITE');offset+=n;}fs.fsyncSync(file.fd);}finally{fs.closeSync(file);}
   }
   const command:T171Command={revision:'T171_RUN_BLOCK_V3',blockId:run,fixtureId:fixture.fixtureId,fixtureSha256:fixture.fixtureSha256,route:'GPU',steps:steps,
    oracleMode:mode==='PERFORMANCE'?'OFF':'ON',timingMode:'COLD_AND_STEADY_SPLIT',repeatId:0,mode:mode,fault:fault,recoveryShaderDirectory:recoveryDirectory,queuedUptimeMs:systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,false),queuedWallMs:Date.now(),output:context.filesDir+'/t171-'+run+'.jsonl',
    config:{nmax:fixture.nmax,shaderDirectory:directory,runId:run,buildManifestSha256:build,sourceManifestSha256:T159_SOURCE_SHA,
     configId:t171ConfigId(fixture,steps,mode==='PERFORMANCE'?'OFF':'ON',fault),
     diagnosticOracle:mode!=='PERFORMANCE',fullDump:mode!=='PERFORMANCE',buildShaderSha256:T159_SHADER_SHA[0],queryShaderSha256:T159_SHADER_SHA[1],reduceShaderSha256:T159_SHADER_SHA[2]}};
   this.physics=new worker.ThreadWorker('entry/ets/workers/T171Worker.ets');
   this.physics.onmessage=(event):void=>{hilog.info(0,'T171','%{public}s',JSON.stringify({result:event.data,uiReceivedUptimeMs:systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,false)}));};
   this.physics.onerror=(event):void=>{hilog.error(0,'T171','WORKER_FAULT %{public}s',event.message);};
   this.physics.postMessage(command);
  }catch(error){hilog.error(0,'T171','FAULT %{public}s',(error as Error).message);}
 }
}
export const t165PageLauncher:T165PageLauncher=new T165PageLauncher();
