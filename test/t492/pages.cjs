module.exports=(main,edit,one)=>{
for(const name of ['GpuResidentM1','GpuSandWaterExperience'])edit('ets/pages/'+name+'.ets',s=>{
 const diagnostic=name==='GpuResidentM1',frame=diagnostic?'M1DemoFrame':'ExperienceFrame';
 s="import { GpuTimelineSnapshotCold } from '../physics/GpuTimelineSnapshotCold';\n"+s;
 if(!diagnostic)s="import process from '@ohos.process';\n"+s;
 if(diagnostic){
  s=one(s,'private readonly cold:GpuBridgeTimelineCold=new GpuBridgeTimelineCold();','private readonly cold:GpuTimelineSnapshotCold=new GpuTimelineSnapshotCold();');
  s=one(s,'this.cold.collect(m1,','this.cold.collectSnapshot(m1,');
 }else{
  s=one(s,'  private readonly model:',`  private readonly cold:GpuTimelineSnapshotCold=new GpuTimelineSnapshotCold();
  @State private packageSha:string='';
  private async exportTimeline():Promise<void>{
   try{const context=getContext(this) as common.UIAbilityContext;const out=await this.cold.collectSnapshot(m1,new DiagnosticColdFiles(context.filesDir),{module:'libgrsm1.so',route:'GpuSandWaterExperience',packageSha256:this.packageSha.trim(),pid:process.pid},()=>new Promise<void>((resolve:()=>void)=>{setTimeout(resolve,0);}));this.operation=out.state+' '+out.reason;}catch(error){this.operation=String(error);}
  }
  private readonly model:`);
  s=one(s,'    Column({ space: 6 }) {',`    Column({ space: 6 }) {
      TextInput({placeholder:'诊断签包 SHA256（外部清单）',text:this.packageSha}).onChange((value:string)=>{this.packageSha=value;})
      Row(){Button('导出稳定快照').onClick(()=>this.exportTimeline());Button('取消导出').onClick(()=>this.cold.cancel());}`);
 }
 s=one(s,' private restart():void{',` private actionSerial:number=0;
 private action(stage:string,serial:number,detail:string=''):void{m1.recordDiagnosticAction(JSON.stringify({origin:'PAGE',actionSerial:serial,event:stage,clock:'UI_WALL_MS',stamp:Date.now(),detail:detail}));}
 private restart():void{`);
 const original='this.frame.pulse.stop();this.frame.owner=null;this.model.reset();this.frame=new '+frame+'();this.frame.owner=this;this.frame.pulse.bindRequested();';
 const replacement=`const serial=++this.actionSerial;this.action('RESTART_ENTER',serial);
 this.action('OLD_FRAME_CANCEL_ENTER',serial);this.frame.pulse.stop();this.frame.owner=null;this.action('OLD_FRAME_CANCEL_EXIT',serial);
 try{this.model.reset(serial);}catch(error){this.action('RESTART_THROW',serial,String(error));throw new Error(String(error));}
 this.frame=new ${frame}();this.frame.owner=this;this.frame.pulse.bindRequested();this.action('NEW_FRAME_BIND',serial,m1.recoveryState());`;
 s=one(s,original,replacement);
 // The button itself must be observable independently of restart execution.
 s=s.replaceAll("Button('新空场').onClick(() => this.restart())","Button('新空场').onClick(() => {this.action('RESET_HANDLER_ENTER',this.actionSerial+1);this.restart();})");
 if(diagnostic)s=s.replaceAll("Button('新空场').onClick(()=>this.restart())","Button('新空场').onClick(()=>{this.action('RESET_HANDLER_ENTER',this.actionSerial+1);this.restart();})");
 if(diagnostic)s=s.replace('onPageHide():void{','onPageHide():void{this.cold.cancel();');
 else s=one(s,'onPageHide(): void {','onPageHide(): void { this.cold.cancel();');
 return s;
});
};
