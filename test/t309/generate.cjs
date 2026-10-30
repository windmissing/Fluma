'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]);cp.execFileSync(process.execPath,['test/t270/generate.cjs',out],{cwd:root,stdio:'pipe'});const ets=path.join(out,'entry/src/main/ets'),d=path.join(ets,'physics/clearwater/diagnostics');
const sha=x=>crypto.createHash('sha256').update(x).digest('hex');let s=fs.readFileSync(path.join(d,'ClearWaterDiagnostic.ets'),'utf8');fs.writeFileSync(path.join(d,'ClearWaterDiagnosticV1.ets'),s);
s="import { ClearWaterDiagnostic as LegacyDiagnostic } from './ClearWaterDiagnosticV1';\n"+s;s=s.replace("'CW_C_DIAG_V1'","'CW_C_DIAG_V2'").replace('constructor(capacity:number=256)','constructor(capacity:number=16384)').replace('capacity>4096','capacity>32768');
s=s.replace('export class ClearWaterDiagnostic {',`export class ClearWaterDiagnostic {
 readonly low:LegacyDiagnostic=new LegacyDiagnostic(256);lowSkipped:number=0;criticalDropped:number=0;globalSequence:number=0;private lowAttempts:number=0;
 private lastStop:string='';private lastRun:string='';private activity:boolean=true;
 readonly firstLostKey:Float64Array=new Float64Array(24);firstLostRunId:string='';firstLostReason:string='';
 readonly stringBudgetBytes:number=8388608;stringBytes:number=0;
 private lost(e:P011ClearWaterEngine,r:ClearWaterSourceRequest|null,reason:string):void {this.criticalDropped++;this.dropped++;if(this.criticalDropped===1){this.firstLostRunId=r?.runId??e.runId;this.firstLostReason=reason;const v=this.firstLostKey;v[0]=e.ownerId;v[1]=e.runGeneration;v[2]=e.currentInputGeneration;v[3]=e.geometryEpoch;if(r!==null){v[4]=r.runGeneration;v[5]=r.configRev;v[6]=r.geometryEpoch;v[7]=r.inputGeneration;v[8]=r.gestureId;v[9]=r.sourceId;v[10]=r.requestSeq;v[11]=r.kind;v[12]=r.x;v[13]=r.y;v[14]=r.requestedVolume;v[15]=r.timestamp;v[16]=r.clockWindowId;}}}
 event(kind:number,e:P011ClearWaterEngine,r:ClearWaterSourceRequest|null=null,t:ClearWaterSourceReceipt|null=null,a:number=0,b:number=0,reason:string=''):void {
  this.globalSequence++;let low=kind===4;
  if(kind===2){low=!this.activity&&this.lastStop===reason&&this.lastRun===e.runId;this.lastStop=reason;this.lastRun=e.runId;this.activity=false;}else if(kind!==4)this.activity=true;
  if(low){if(this.lowAttempts++%60!==0){this.lowSkipped++;return;}this.low.event(kind,e,r,t,a,b,reason);return;}
  const strings=[e.runId,r?.runId??'',r?.timestampUnit??'',reason,t?.disposition??''];let bytes=0;for(const value of strings)bytes+=value.length*2;
  if(this.count===this.capacity||this.stringBytes+bytes>this.stringBudgetBytes){this.lost(e,r,this.count===this.capacity?'CRITICAL_CAPACITY':'STRING_BUDGET');return;}
  this.stringBytes+=bytes;this.record(kind,e,r,t,a,b,reason);this.rows[(this.count-1)*64+40]=this.globalSequence;
 }
`);
// Rename only original recording method, not new routing event wrapper.
const at=s.indexOf('  event(kind:number',s.indexOf('readonly revision:'));if(at<0)throw Error('T309_RECORD');s=s.slice(0,at)+s.slice(at).replace('  event(kind:number','  private record(kind:number');
const a=s.indexOf('  clockEvent('),b=s.indexOf('  capture(',a);s=s.slice(0,a)+`  clockEvent(e:P011ClearWaterEngine,raw:number,seconds:number,window:number,unit:string,domain:string,reason:string,valid:boolean):void {this.globalSequence++;if(this.lowAttempts++%60!==0){this.lowSkipped++;return;}this.low.clockEvent(e,raw,seconds,window,unit,domain,reason,valid);}
`+s.slice(b);
// Existing explicit full export remains V2, cold only; independent frame slots untouched.
s=s.replace('count:this.count,dropped:this.dropped,incomplete:',"lowLane:JSON.parse(this.low.exportCold(packageSha)),criticalDropped:this.criticalDropped,firstLostKey:this.firstLostKey,firstLostRunId:this.firstLostRunId,firstLostReason:this.firstLostReason,lowSkipped:this.lowSkipped,stringBytes:this.stringBytes,stringBudgetBytes:this.stringBudgetBytes,count:this.count,dropped:this.dropped,incomplete:");
fs.writeFileSync(path.join(d,'ClearWaterDiagnostic.ets'),s);fs.copyFileSync(path.join(__dirname,'CriticalCold.ets'),path.join(d,'CriticalCold.ets'));
const c=path.join(ets,'render/P013ClearWaterController.ets');s=fs.readFileSync(c,'utf8').replace('new ClearWaterDiagnostic(capacity)','new ClearWaterDiagnostic(capacity)');fs.writeFileSync(c,s);
const page=path.join(ets,'pages/ClearWaterDemo.ets');s=fs.readFileSync(page,'utf8');s="import { common } from '@kit.AbilityKit';\nimport { exportCriticalCold } from '../physics/clearwater/diagnostics/CriticalCold';\nimport { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';\n"+s;s=s.replace('this.model.configureDiagnostic(256,','this.model.configureDiagnostic(16384,');
const start=s.indexOf("    Button('导出诊断').onClick("),end=s.indexOf('\n',start);if(start<0)throw Error('T309_PAGE');s=s.slice(0,start)+`    Button('冷分段导出诊断').onClick(async()=>{if(this.model.diagnostic===null)return;try{const context=getContext(this) as common.UIAbilityContext;const manifest=await exportCriticalCold(this.model.diagnostic,new DiagnosticColdFiles(context.filesDir),AppStorage.get<string>('cwdiagSha')??'',cwNative.cwDiagExport(),()=>new Promise<void>((resolve:()=>void)=>setTimeout(resolve,0)));this.status=manifest;}catch(e){this.status='INCOMPLETE '+String(e);}})`+s.slice(end);fs.writeFileSync(page,s);
const manifest={revision:'T309_CRITICAL_DIAG_V2',parent:JSON.parse(fs.readFileSync(path.join(out,'t270-manifest.json'))),criticalCapacity:16384,lowCapacity:256,rowBytes:512,typedRowsBytes:16384*512+256*512,stringUtf16Budget:8388608,files:['ClearWaterDiagnostic.ets','ClearWaterDiagnosticV1.ets','CriticalCold.ets'].map(file=>({file,sha:sha(fs.readFileSync(path.join(d,file)))})),productionTouched:false};fs.writeFileSync(path.join(out,'t309-manifest.json'),JSON.stringify(manifest,null,2));cp.execFileSync(process.execPath,[path.join(__dirname,'refine.cjs'),out],{cwd:root,stdio:'pipe'});console.log(out);
