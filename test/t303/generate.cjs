'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]);
// Fresh generation retains formal BEFORE cuts and all original fault/receipt domains.
if(!fs.existsSync(out))cp.execFileSync(process.execPath,['test/t250/generate_before.cjs',out],{cwd:root,stdio:'pipe'});
const ets=path.join(out,'entry/src/main/ets'),base='physics/tx/';const sha=x=>crypto.createHash('sha256').update(x).digest('hex'),edits=[];
function patch(file,a,b){const p=path.join(ets,base+file),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T303_PATCH:'+file+':'+a.slice(0,60));const t=s.replace(a,b);fs.writeFileSync(p,t);edits.push({file,sha:sha(t)});}
fs.copyFileSync(path.join(__dirname,'TxRunnerStages.ets'),path.join(ets,base,'TxRunnerStages.ets'));
patch('TxGpuFixture.ets',"import { WaterSandField", "import { TxRunnerStages } from './TxRunnerStages';\nimport { WaterSandField");
patch('TxGpuFixture.ets','beforeEndpointInputs:boolean=false):TxGpuFixtureResult {','beforeEndpointInputs:boolean=false,stages:TxRunnerStages|null=null):TxGpuFixtureResult {');
patch('TxGpuFixture.ets','backend.beginRoot();const rootBefore=field.t171ArchiveState();const h=', 'backend.beginRoot();stages?.add(8,backend.metadata.rootId);const rootBefore=field.t171ArchiveState();const h=');
patch('TxGpuFixture.ets',"      rootRows.push(JSON.stringify({kind:'SOURCE'", "      stages?.add(9,backend.metadata.rootId,r?.rejectedReason===null?0:1);if(originalException!==null)stages?.error(originalException);\n      rootRows.push(JSON.stringify({kind:'SOURCE'");
patch('TxGpuFixture.ets','for(let n=0;n<steps;n++){backend.beginRoot();','for(let n=0;n<steps;n++){backend.beginRoot();stages?.add(8,backend.metadata.rootId);');
patch('TxGpuFixture.ets',"rootRows.push(JSON.stringify({kind:'STEP'", "stages?.add(9,backend.metadata.rootId,r?.errorCode??-1);if(originalStepException!==null)stages?.error(originalStepException);rootRows.push(JSON.stringify({kind:'STEP'");
patch('TxGpuWorker.ets',"import process from '@ohos.process';", "import process from '@ohos.process';\nimport { TxRunnerStages } from './TxRunnerStages';");
patch('TxGpuWorker.ets','export interface TxGpuCommand {','export interface TxGpuCommand {stageEnabled?:boolean;');
patch('TxGpuWorker.ets','class NativeTransport implements TxNativeTransport {','let observation:TxRunnerStages|null=null;\nclass NativeTransport implements TxNativeTransport {');
patch('TxGpuWorker.ets','configure(c:TxGpuConfig):TxNativeReply{return gpu.configure(c) as TxNativeReply;}','configure(c:TxGpuConfig):TxNativeReply{observation?.add(5);try{const r=gpu.configure(c) as TxNativeReply;observation?.add(6);return r;}catch(e){observation?.error(String(e));throw e as Error;}}');
patch('TxGpuWorker.ets',"  try{\n    if(c.revision",`  let stageFile:fs.File|null=null;
  observation=c.stageEnabled===false?null:new TxRunnerStages(now);
  if(observation!==null){try{stageFile=fs.openSync(c.output+'.worker-stages.ndjson',fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);observation.sink=(line:string):void=>{const bytes=new util.TextEncoder().encodeInto(line);let off=0;while(off<bytes.length){const written=fs.writeSync(stageFile!.fd,bytes.buffer.slice(off));if(written<=0)throw new Error('STAGE_SHORT_WRITE');off+=written;}fs.fsyncSync(stageFile!.fd);};}catch(e){observation.observerError=String(e);}
   observation.identity({runId:c.config.runId,packageSha:c.config.buildManifestSha256,pid:process.pid,tid:process.tid,backend:c.config.backend,arm:c.config.freezeArm??'COMMON_FREEZE_OFF',fixture:c.fixtureId,suite:c.suite===true,faultPlan:JSON.stringify(c.faultPlan??null)});observation.add(4);}
  try{
    if(c.revision`);
patch('TxGpuWorker.ets','    const start=now(),queueMs=', '    observation?.add(7);\n    const start=now(),queueMs=');
patch('TxGpuWorker.ets','c.faultPlan??null,process.pid),encodeStart=now();','c.faultPlan??null,process.pid,false,false,observation),encodeStart=now();');
patch('TxGpuWorker.ets',"new TxGpuTreeAdapter(config.nmax):null);", "new TxGpuTreeAdapter(config.nmax):null,null,process.pid,false,false,observation);");
patch('TxGpuWorker.ets','      const suiteStart=now(),row=',"      observation?.identity({runId:config.runId,packageSha:config.buildManifestSha256,pid:process.pid,tid:process.tid,backend:backend,arm:arm,fixture:c.fixtureId,suite:true,faultPlan:'null'});observation?.add(7);\n      const suiteStart=now(),row=");
patch('TxGpuWorker.ets','    physicalStatus=result.physicalStatus;const bytes=', '    physicalStatus=result.physicalStatus;observation?.add(10);const bytes=');
patch('TxGpuWorker.ets',"    const f=fs.openSync(c.output", "    observation?.add(11);observation?.add(12);\n    const f=fs.openSync(c.output");
patch('TxGpuWorker.ets',"    port.postMessage({revision:'TX_GPU_ARCHIVE_V1',status:'SEALED'", "    observation?.add(13);observation?.add(14);\n    port.postMessage({stages:observation?.archive()??'DISABLED',revision:'TX_GPU_ARCHIVE_V1',status:'SEALED'");
patch('TxGpuWorker.ets',"  }catch(error){port.postMessage({revision:","  }catch(error){observation?.error((error as Error).message);observation?.add(14,0,-1);port.postMessage({stages:observation?.archive()??'DISABLED',revision:");
patch('TxGpuWorker.ets','  finally{active=false;}','  finally{if(stageFile!==null){try{fs.fsyncSync(stageFile.fd);fs.closeSync(stageFile);}catch(e){observation?.error(String(e));}}active=false;}');
patch('TxGpuPageLauncher.ets',"import common from", "import process from '@ohos.process';import time from '@ohos.systemDateTime';import util from '@ohos.util';import { TxRunnerStages } from './TxRunnerStages';\nimport common from");
patch('TxGpuPageLauncher.ets','export class T165PageLauncher {','export class T165PageLauncher {private readonly stages:TxRunnerStages=new TxRunnerStages(()=>time.getUptime(time.TimeType.ACTIVE,false));');
patch('TxGpuPageLauncher.ets',"    const directory=context.cacheDir",`    const stages=this.stages,stageFile=fs.openSync(context.filesDir+'/txgpu-'+run+'.launcher-stages.ndjson',fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
    stages.sink=(line:string):void=>{const b=new util.TextEncoder().encodeInto(line);let offset=0;while(offset<b.length){const n=fs.writeSync(stageFile.fd,b.buffer.slice(offset));if(n<=0)throw new Error('LAUNCHER_STAGE_SHORT_WRITE');offset+=n;}fs.fsyncSync(stageFile.fd);};
    stages.identity({runId:run,packageSha:build,pid:process.pid,tid:process.tid,backend:backend,arm:String(p['txFreezeArm']??'COMMON_FREEZE_OFF'),fixture:fixtureId,suite:p['txSuite']===true||p['txSuite']==='true',faultPlan:JSON.stringify({root:p['txFaultRoot'],cut:p['txFaultCut'],ordinal:p['txFaultOrdinal']})});stages.add(1);
    const directory=context.cacheDir`);
patch('TxGpuPageLauncher.ets',"this.work.onmessage=(e):void=>{hilog.info", "this.stages.add(2);this.work.onerror=(e):void=>{this.stages.error(JSON.stringify(e));this.stages.add(15,0,-1);};this.work.onexit=(code:number):void=>{this.stages.add(16,0,code);};this.work.onmessage=(e):void=>{this.stages.add(14);hilog.info");
patch('TxGpuPageLauncher.ets','this.work.postMessage(c);','this.stages.add(3);this.work.postMessage(c);');
patch('TxGpuPageLauncher.ets',"}catch(error){hilog.error", "}catch(error){this.stages.error((error as Error).message);hilog.error");
const parent=JSON.parse(fs.readFileSync(path.join(out,'t250-before-manifest.json')));const payload={revision:'T303_STAGE_GENERATOR_V1',parent:parent.sourceManifestSha256,helperSha:sha(fs.readFileSync(path.join(__dirname,'TxRunnerStages.ets'))),edits};const source=sha(JSON.stringify(payload));
for(const rel of ['entry/src/main/ets/physics/T159BuildIdentity.ets','entry/src/main/cpp/p013_compute/CMakeLists.txt']){const p=path.join(out,rel);fs.writeFileSync(p,fs.readFileSync(p,'utf8').replaceAll(parent.sourceManifestSha256,source));}
fs.writeFileSync(path.join(out,'t303-manifest.json'),JSON.stringify({...payload,sourceManifestSha256:source},null,2));cp.execFileSync(process.execPath,[path.join(__dirname,'refine.cjs'),out],{cwd:root,stdio:'pipe'});console.log(out);
