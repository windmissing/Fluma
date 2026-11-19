import { resourceLab538 as lab } from '../physics/GpuResourceLabPort';
import { common } from '@kit.AbilityKit';
import { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';
@Entry
@Component
struct GpuResourceLab {
 @State private caseId:string='TRACE_MAX_SOURCE';@State private result:string='FIXTURE_SYNTHETIC · physicsValidated=false · currentGranted=false · deviceGLMeasured=false';private job:string='';private timer:number=-1;
 private start():void{const c=this.getUIContext().getHostContext() as common.UIAbilityContext;this.job=lab.resourceLabStartV1(this.caseId,c.filesDir);this.poll();}
 private poll():void{this.result='UI poll '+Date.now()+'\n'+lab.resourceLabPollV1(this.job).substring(0,4096);}
 private async save():Promise<void>{try{const c=this.getUIContext().getHostContext() as common.UIAbilityContext;const r=await new DiagnosticColdFiles(c.filesDir).save('resource-lab-'+this.job+'-'+Date.now()+'.json',lab.resourceLabExportV1(this.job));this.result=r.name+' '+r.sha256;console.info('WS_EVENT RESOURCE_LAB '+JSON.stringify(r));}catch(error){this.result='INCOMPLETE '+String(error);}}
 aboutToAppear():void{this.timer=setInterval(()=>{if(this.job)this.poll();},500);}
 aboutToDisappear():void{if(this.timer>=0)clearInterval(this.timer);if(this.job)lab.resourceLabCancelV1(this.job);}
 build(){Column({space:8}){
  Text('T531_RESOURCE_LAB_RECORD_V3 · 独立合成资源诊断').fontSize(18)
  Text('不创建XComponent、物理owner、SOURCE或Swap；不宣物理通过。').fontSize(12)
  Select([{value:'CHECKPOINT_BARRIER'},{value:'TRACE_MAX_SOURCE'},{value:'TRACE_MAX_Q'},{value:'TWO_SLOT_PIN_REPLACE'},{value:'TTL_WALL'},{value:'TTL_EDGE_INJECTED'},{value:'DIAGNOSTIC_CREDIT'},{value:'DIAGNOSTIC_BUSY_FAIL'},{value:'CACHE_FAULT'}]).selected(0).onSelect((_i:number,v:string)=>{this.caseId=v;})
  Row(){Button('启动').onClick(()=>this.start());Button('查询').onClick(()=>this.poll());Button('释放屏障').onClick(()=>{lab.resourceLabReleaseBarrierV1(this.job);this.poll();});Button('取消').onClick(()=>{lab.resourceLabCancelV1(this.job);this.poll();});Button('导出').onClick(()=>this.save());}
  Scroll(){Text(this.result).fontSize(11)}.layoutWeight(1).width('100%')
 }.width('100%').height('100%').padding(8)}
}
