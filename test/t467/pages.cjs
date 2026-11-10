const fs=require('node:fs'),path=require('node:path');
module.exports=(main,edit,one,target)=>{
for(const name of ['GpuResidentM1','GpuSandWaterExperience'])edit('ets/pages/'+name+'.ets',s=>{
 s="import { GpuRecoveryPulse } from '../physics/GpuRecoveryPulse';\n"+s;
 const diagnostic=name==='GpuResidentM1';
 if(diagnostic){
  s=one(s," owner:GpuResidentM1|null=null;session:string='0';sequence:number=0;",` owner:GpuResidentM1|null=null;session:string='0';sequence:number=0;
 pulse:GpuRecoveryPulse=new GpuRecoveryPulse(m1,String(++recoveryPageSerial));`);
  s=one(s,"this.session==='0'||!Number.isSafeInteger(ns)","!Number.isSafeInteger(ns)");
  s=one(s,'const code:string=m1.pagePulse(this.session,String(++this.sequence),String(ns));','const code:string=this.pulse.pulse(ns);this.session=this.pulse.session;');
  s=s.replaceAll('this.frame.owner=null;','this.frame.pulse.stop();this.frame.owner=null;');
  s=one(s,'this.frame.session=m1.beginPageSession();','');
  s=one(s,"@State private progressV2:boolean=false;",`@State private progressV2:boolean=${target!=='regular'};`);
  s=one(s,'aboutToAppear():void{this.frame.owner=this;}','aboutToAppear():void{this.frame.owner=this;'+(target!=='regular'?'m1.setProgressV2(true);':'')+'}');
  s=one(s,' private schedule():void',` private restart():void{this.frame.pulse.stop();this.frame.owner=null;this.model.reset();this.frame=new M1DemoFrame();this.frame.owner=this;this.frame.pulse.bindRequested();this.queued=false;this.schedule();}
 private schedule():void`);
  s=s.replaceAll('()=>this.model.reset()', '()=>this.restart()');
  s=one(s,' private schedule():void',` private bindColdFrame():void{this.frame.pulse.stop();this.frame.owner=null;this.frame=new M1DemoFrame();this.frame.owner=this;this.frame.pulse.bindRequested();this.queued=false;this.schedule();}
 private schedule():void`);
  s=s.replace('()=>this.model.applyOptics(this.sigma,this.gain,this.widthPx)', '()=>{this.model.applyOptics(this.sigma,this.gain,this.widthPx);this.bindColdFrame();}');
  for(const k of [192,512])s=s.replace('()=>this.model.profile('+k+',128)', '()=>{this.model.profile('+k+',128);this.bindColdFrame();}');
 }else{
  s=one(s,"  session: string = '0';",`  session: string = '0';
  pulse:GpuRecoveryPulse=new GpuRecoveryPulse(m1,String(++recoveryPageSerial));`);
  s=one(s,"this.session === '0' || ", '');
  s=one(s,'this.owner.tick(ns, m1.pagePulse(this.session, String(++this.sequence), String(ns)));','const code:string=this.pulse.pulse(ns);this.session=this.pulse.session;this.owner.tick(ns,code);');
  s=s.replaceAll('this.frame.owner = null;', 'this.frame.pulse.stop();this.frame.owner = null;');
  s=one(s,'      this.frame.session = m1.beginPageSession();','');
  s=one(s,'if (this.supportsV2) m1.setProgressV2(false);',`if (this.supportsV2) m1.setProgressV2(${target!=='regular'});`);
  s=one(s,'      this.model.reset();','');
  s=one(s,'  private selectProgress(v2: boolean): void {',`  private restart():void{this.frame.pulse.stop();this.frame.owner=null;this.model.reset();this.frame=new ExperienceFrame();this.frame.owner=this;this.frame.pulse.bindRequested();if(this.active)this.getUIContext().postFrameCallback(this.frame);}
  private selectProgress(v2: boolean): void {`);
  s=s.replaceAll('this.model.reset();', 'this.restart();');
  // Keep restart's one native request, without recursively invoking itself.
  s=s.replace('this.frame.owner=null;this.restart();this.frame=new ExperienceFrame()', 'this.frame.owner=null;this.model.reset();this.frame=new ExperienceFrame()');
  s=s.replaceAll('() => this.model.reset()', '() => this.restart()');
 }
 const at=diagnostic?'class M1DemoFrame':'class ExperienceFrame';
 s=one(s,at,'let recoveryPageSerial:number=0;\n'+at);
 return s;
});
};
