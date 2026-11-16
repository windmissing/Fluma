const fs=require('node:fs'),path=require('node:path');
module.exports=(main,edit,one,route)=>{
for(const name of ['GpuBoundedCold','GpuColdTraceV1','GpuUtf8Budget'])fs.copyFileSync('entry/src/main/ets/physics/'+name+'.ets',path.join(main,'ets/physics',name+'.ets'));
edit('ets/physics/GpuBridgeTimelineCold.ets',s=>{
 s="import { utf8Bytes } from './GpuUtf8Budget';\n"+s;
 for(const expression of ['raw.length','chunk.length','pendingRaw.length'])s=s.replaceAll(expression,'utf8Bytes('+expression.split('.')[0]+')');
 s=one(s,"    chunk+=raw+'\\n';", "    if(out.dataBytes+utf8Bytes(chunk)+utf8Bytes(raw)+1>4194304)throw new Error('EXPORT_BUDGET');\n    chunk+=raw+'\\n';");
 s=one(s,'if(utf8Bytes(pendingRaw)>0&&utf8Bytes(pendingRaw)<=16384){','if(utf8Bytes(pendingRaw)>0&&utf8Bytes(pendingRaw)<=16384&&out.dataBytes+utf8Bytes(chunk)+utf8Bytes(pendingRaw)<=4194304){');
 return s;
});
edit('ets/physics/GpuTimelineSnapshotCold.ets',s=>{
 s="import { BoundedColdPort,coldCapability,DiagnosticEnvelope } from './GpuBoundedCold';\nimport { utf8Bytes } from './GpuUtf8Budget';\n"+s;
 s=one(s,'export interface SnapshotPort extends TimelineModule {','export interface SnapshotPort extends TimelineModule,BoundedColdPort {');
 s=one(s,' executionDiagnostics():string;','');
 s=one(s,'   out.beginRaw=port.timelineSnapshotBegin();','   const capability=coldCapability(port);\n   out.beginRaw=port.timelineSnapshotBegin();');
 s=one(s,'   const diagnostics=port.executionDiagnostics();if(diagnostics.length>2097152||diagnostics.length+out.trace.dataBytes>4194304)throw new Error(\'DIAGNOSTIC_BUDGET\');',`   const remaining=Math.min(2097152,4194304-out.trace.dataBytes);if(remaining<16384)throw new Error('DIAGNOSTIC_BUDGET');
   const diagnostics=port.executionDiagnosticsV1(JSON.stringify(capability.scope),remaining);if(utf8Bytes(diagnostics)>remaining)throw new Error('DIAGNOSTIC_BUDGET');
   const diagnostic=JSON.parse(diagnostics) as DiagnosticEnvelope;`);
 s=one(s,"   out.files.push(await store.save(prefix+'-execution.json',diagnostics));", "   const savedDiagnostic=await store.save(prefix+'-execution.json',diagnostics);if(savedDiagnostic.bytes!==utf8Bytes(diagnostics))throw new Error('UTF8_FILE_BYTES');out.files.push(savedDiagnostic);\n   if(diagnostic.revision!=='T503_EXECUTION_DIAGNOSTICS_V1'||diagnostic.state!=='COMPLETE'||!diagnostic.dataComplete||diagnostic.scopeStale)throw new Error(diagnostic.scopeStale?'INCOMPLETE_SCOPE_CHANGED':diagnostic.reason??diagnostic.state);");
 for(const expression of ['raw.length','out.beginRaw.length','pending.length'])s=s.replaceAll(expression,'utf8Bytes('+expression.substring(0,expression.length-7)+')');
 s=one(s,'if(utf8Bytes(pending)>0&&utf8Bytes(pending)<=16384){','if(utf8Bytes(pending)>0&&utf8Bytes(pending)<=16384&&(out.trace===null||out.trace.dataBytes+utf8Bytes(pending)<=4194304)){');
 return s;
});
if(route==='m2')edit('ets/pages/GpuSandWaterExperience.ets',s=>{
 s="import { GpuColdTraceV1 } from '../physics/GpuColdTraceV1';\n"+s;
 const begin=s.indexOf(' private exportingTrace:boolean=false;'),end=s.indexOf(' private createExportBusy:',begin);if(begin<0||end<0)throw Error('M2_COLLECTOR');
 s=s.slice(0,begin)+` private readonly coldTrace:GpuColdTraceV1=new GpuColdTraceV1();
 private async exportM2Trace():Promise<void>{try{const context=getContext(this) as common.UIAbilityContext;const result=await this.coldTrace.collect(m1,new DiagnosticColdFiles(context.filesDir));this.operation=result.state+' '+result.reason;}catch(error){this.operation='TRACE_INCOMPLETE '+String(error);}}
`+s.slice(end);
 s=s.replace("Button('锁存 V3 冷档').onClick(()=>{if(!this.exportingTrace)this.operation=m1.requestM2Trace();})","Button('取消冷trace').onClick(()=>this.coldTrace.cancel())");
 s=s.replace("Button('导出 V3 冷档')","Button('导出有界冷trace')");
 return one(s,'onPageHide(): void { this.cold.cancel();','onPageHide(): void { this.cold.cancel();this.coldTrace.cancel();');
});
};
