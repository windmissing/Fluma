const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]),target=process.argv[3];
if(!['t410','t437','regular'].includes(target))throw Error('expected t410, t437 or regular');
const experiment=target!=='regular';
if(target==='t437'){
 cp.execFileSync(process.execPath,[require.resolve('../t434/generate.cjs'),out],{stdio:'inherit'});
 for(const file of ['full_transport.cpp','full_transport.h','quantum_candidate.cpp','quantum_candidate.h']){
  const rel='entry/src/main/cpp/gpu_resident/m1/'+file;
  fs.writeFileSync(path.join(out,rel),cp.execFileSync('git',['show','b54dc48:'+rel]));
 }
}else require('../t434/frozen.cjs').freeze(out);
const main=path.join(out,'entry/src/main');
function edit(rel,fn){const file=path.join(main,rel);fs.writeFileSync(file,fn(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T450_SOURCE_DRIFT '+a);return s.replace(a,b);}
if(target==='t410')fs.appendFileSync(path.join(main,'cpp/CMakeLists.txt'),'\ntarget_compile_definitions(grsm1 PRIVATE GRS_PROGRESS_V2_EXPERIMENT)\n');
fs.copyFileSync(path.join(__dirname,'GpuCaptureLaunch.ets'),path.join(main,'ets/physics/GpuCaptureLaunch.ets'));
fs.writeFileSync(path.join(main,'ets/physics/T450CaptureProfile.ets'),`export const T450_CAPTURE_LAUNCH_ENABLED: boolean = ${experiment};\n`);
edit('ets/entryability/EntryAbility.ets',s=>{
 s=one(s,"import grsm1 from 'libgrsm1.so';","import grsm1 from 'libgrsm1.so';\nimport { initializeCaptureLaunch } from '../physics/GpuCaptureLaunch';");
 const a="    AppStorage.setOrCreate('gpuResidentM1', `${want.parameters?.grsm1}` === 'true');";
 return one(s,a,a+'\n    initializeCaptureLaunch(want.parameters?.grsm1, want.parameters?.gpusha);');
});
edit('ets/pages/GpuResidentM1.ets',s=>{
 s=one(s,"import m1 from 'libgrsm1.so';",`import m1 from 'libgrsm1.so';
import { CaptureArmAttempt, CaptureLaunchDeclaration, CaptureScope, captureLaunch, prepareCaptureArm } from '../physics/GpuCaptureLaunch';`);
 s=one(s,'class M1DemoFrame extends FrameCallback', 'interface LaunchCaptureReply extends EvidenceCaptureReply { invalidField?: string; }\nclass M1DemoFrame extends FrameCallback');
 s=s.replace(', EvidenceArmConfig','');
 const start=s.indexOf(' private armCapture():void {'),end=s.indexOf(' private updateCapture(',start);
 if(start<0||end<0)throw Error('T450_ARM_SOURCE_DRIFT');
 s=s.slice(0,start)+fs.readFileSync(path.join(__dirname,'arm.inc'),'utf8')+s.slice(end);
 s=one(s,' private readonly captureCold:',` private readonly launchDeclaration:CaptureLaunchDeclaration=captureLaunch();
 private pendingLaunchAttempt:CaptureArmAttempt|null=null;
 private archivedLaunchAttempt:CaptureArmAttempt|null=null;
 private lastLaunchAttempt:CaptureArmAttempt|null=null;
 private readonly captureCold:`);
 s=one(s," private updateCapture(reply:EvidenceCaptureReply):void {",
  " private updateCapture(reply:EvidenceCaptureReply):void {\n  if(reply.state!=='REJECTED'&&reply.requestId!==this.captureRequestId)return;");
 s=one(s,"if((reply.state==='ARMED'||reply.state==='FROZEN')", "if(reply.requestId===this.captureRequestId&&this.captureRequestId!==''&&(reply.state==='ARMED'||reply.state==='FROZEN')");
 s=one(s,'this.archivedRequest=reply;', 'this.archivedRequest=reply;this.archivedLaunchAttempt=this.pendingLaunchAttempt;');
 s=one(s,"   const result=await this.captureCold.collect(","   if(this.archivedLaunchAttempt!==null)await files.save('gpu-capture-launch-'+this.archivedRequestId+'-'+this.captureId+'.json',JSON.stringify(this.archivedLaunchAttempt));\n   const result=await this.captureCold.collect(");
 s=one(s,"@State private packageSha:string='';",'');
 s=one(s,"  if(this.cold.busy)return;this.coldStatus='导出中';","  if(this.cold.busy)return;if(this.launchDeclaration.reason!=='SHA_DECLARED'){this.coldStatus=this.launchDeclaration.reason;return;}this.coldStatus='导出中';");
 s=one(s,'this.packageSha.trim()','this.launchDeclaration.sha');
 s=one(s,"TextInput({placeholder:'诊断签包 SHA256（外部签包清单）',text:this.packageSha}).onChange((v:string)=>{this.packageSha=v;});","Text(this.launchDeclaration.reason+' '+this.launchDeclaration.sha).fontSize(10);Button('导出预约入口回执').onClick(()=>this.exportLaunchAttempt());");
 return s;
});
const napi=fs.readFileSync(path.join(__dirname,'config_diagnostic.inc'),'utf8').trim();
edit('cpp/gpu_resident/evidence_capture_napi.inc',s=>{
 const start=s.indexOf('bool EvidenceConfigArg('),end=s.indexOf('#ifdef GRS_PROGRESS_V2_EXPERIMENT',start);
 if(start<0||end<0)throw Error('T450_NAPI_SOURCE_DRIFT');
 return s.slice(0,start)+napi+'\n'+s.slice(end);
});
fs.writeFileSync(path.join(out,'t450-manifest.json'),JSON.stringify({schema:'T450_CAPTURE_LAUNCH_V1',target,base:'0228a7d',prefixTransport:target==='t437'?'b54dc48':null,experiment,device:'NOT_RUN'},null,2));
