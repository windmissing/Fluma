 private t394Arm:string='';
 private t394Optics='TEXTURE';
 private t394GeometryEpoch=1;
 private t394Phase(name:string,locked:boolean,auto:boolean=false,reuse:boolean=false):void{
  this.fixtureLocked=locked;this.probeFixtureKey='';this.model.stop();
  this.singleSand.beginT394(name,locked,auto,reuse);
  this.probeStatus='WAITING_LOCKED_FRAME';
  this.schedule();
 }
 private t394Start(arm:string):void{
  if(this.singleSand.probeBusy()||this.evidence!==null||this.exporting){this.probeStatus='请先导出上一笔';return;}
  this.t394Arm=arm;
  this.model.startT394Fixture(arm==='STATIC',p013NowMs());
  this.publication=this.model.createPublicationCold();
  this.t394Phase(arm==='STATIC'?'STATIC_'+this.t394Optics:arm+'_A',arm!=='CONTROL',arm!=='STATIC');
 }
 private t394Next():void{
  const hold=this.singleSand.t394Hold(),arm=this.t394Arm;
  if(hold===null||hold.name!==arm+'_A'||this.singleSand.probeBusy()||this.evidence!==null||this.exporting){
   this.probeStatus='先完成并导出 A';return;
  }
  this.t394Phase(arm+'_B',false,true,true);
  this.model.t394InjectB(p013NowMs());
 }
 private t394SetOptics(neutral:boolean):void{
  if(this.singleSand.probeBusy()||this.evidence!==null||this.exporting){this.probeStatus='请先导出上一笔';return;}
  if(this.t394Arm==='CONTROL'||this.t394Arm==='LOCK'){this.probeStatus='动态 A/B 的光学已固定';return;}
  this.t394Optics=neutral?'NEUTRAL':'TEXTURE';
  this.singleSand.setProbeNeutral(neutral);
  this.t394Phase(this.t394Arm==='STATIC'?'STATIC_'+this.t394Optics:'LOCKED_'+this.t394Optics,this.fixtureLocked);
 }
 private async t394SealHold():Promise<void>{
  const hold=this.singleSand.t394VerifyHold();
  if(hold===null){this.probeStatus='DISPLAY_NOT_JUDGABLE';this.schedule();return;}
  const endWallMs=Date.now(),endMonoMs=p013NowMs();
  const text=JSON.stringify({revision:'T394_DISPLAY_HOLD_V1',hold:hold,endWallMs:endWallMs,endMonoMs:endMonoMs,
   endFrame:this.serial,endToken:hold.token,displayStatus:'DISPLAY_NOT_JUDGABLE'});
  try{
   const context=getContext(this) as common.UIAbilityContext;
   const file=await new DiagnosticColdFiles(context.filesDir).save('t394-hold-'+hold.requestId+'-'+hold.frame+'.json',text);
   this.probeStatus=file.name+' '+file.sha256;
   console.info('WS_EVENT t394_hold '+JSON.stringify(file));
  }catch(error){this.probeStatus='HOLD_EXPORT_FAILED';}
 }
 private async t394ExportPhase():Promise<void>{
  const snapshot=this.singleSand.exportT394Phase();
  if(!snapshot){this.probeStatus=this.singleSand.t394PhaseStatus();return;}
  try{
   const context=getContext(this) as common.UIAbilityContext;
   const file=await new DiagnosticColdFiles(context.filesDir).save('t394-phase-'+Date.now()+'.json',snapshot);
   this.probeStatus=file.name+' '+file.sha256;
   console.info('WS_EVENT t394_phase '+JSON.stringify(file));
  }catch(error){this.probeStatus='PHASE_EXPORT_FAILED';}
 }
