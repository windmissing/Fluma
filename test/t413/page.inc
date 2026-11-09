 @State private captureText:string='IDLE';
 @State private configuring:boolean=true;
 @State private maxHoldText:string='300000';
 @State private planText:string='';
 private t394Arm:string='';private t394Optics:string='TEXTURE';private t394GeometryEpoch:number=1;
 private layout:CaptureGeometry=new CaptureGeometry();
 private archivedReceipt:ColdFile|null=null;private archivedAttempt:string='';private fullyVisible:boolean=false;private lastArmName:string='';private t394WaitB:boolean=false;private statusSerial:number=0;
 private allowed():boolean {return this.singleSand.capture.configurable()&&!this.exporting&&this.evidence===null;}
 private t394Phase(name:string,locked:boolean):void {
  if(!this.allowed()){this.probeStatus='HOLD_ACTIVE';return;}
  this.fixtureLocked=locked;this.probeFixtureKey='';this.model.stop();this.singleSand.beginT394(name,locked,false,true);this.schedule();
 }
 private t394Start(arm:string):void {
  if(!this.allowed()){this.probeStatus='HOLD_ACTIVE';return;}
  this.singleSand.setT406Calibration(false);this.t394Arm=arm;this.t394WaitB=false;
  this.model.startT394Fixture(arm==='STATIC',p013NowMs());this.publication=this.model.createPublicationCold();
  this.t394Phase(arm==='STATIC'?'STATIC_'+this.t394Optics:arm+'_A',arm!=='CONTROL');
 }
 private t394Next():void {
  if(!this.allowed()||this.singleSand.holdStatus().state!=='RELEASED'||this.lastArmName!==this.t394Arm+'_A'){
   this.probeStatus='先封存归档并释放 A';return;}
  this.t394WaitB=true;this.t394Phase(this.t394Arm+'_B',false);
 }
 private t394SetOptics(neutral:boolean):void {
  if(!this.allowed()){this.probeStatus='HOLD_ACTIVE';return;}
  if(this.t394Arm==='CONTROL'||this.t394Arm==='LOCK'){this.probeStatus='动态光学已固定';return;}
  this.t394Optics=neutral?'NEUTRAL':'TEXTURE';this.singleSand.setProbeNeutral(neutral);
  this.t394Phase(this.t394Arm==='STATIC'?'STATIC_'+this.t394Optics:'LOCKED_'+this.t394Optics,this.fixtureLocked);
 }
 private async statusCold():Promise<void> {
  const s=this.singleSand.holdStatus();this.captureText=s.state+' '+s.holdId+' '+s.archiveState+' '+s.reason;
  const context=getContext(this) as common.UIAbilityContext;
  const f=await new DiagnosticColdFiles(context.filesDir).save('t413-status.json',JSON.stringify({readSerial:++this.statusSerial,status:s}));
  console.info('WS_EVENT t413_status '+JSON.stringify(f));
 }
 private async prepare():Promise<void> {
  if(!this.allowed())return;this.exporting=true;
  try{this.getUIContext().getFocusController().clearFocus();await inputMethod.getController().hideTextInput();
   this.configuring=false;this.probeStatus='布局稳定后预约';this.schedule();
  }catch(error){this.probeStatus='KEYBOARD_HIDE_FAILED';}
  finally{this.exporting=false;}
 }
 private async reserve():Promise<void> {
  // A separate configure/close-keyboard action must have completed before reserve.
  if(this.configuring||this.exporting||this.evidence!==null){this.probeStatus='CONFIGURATION_OR_ARCHIVE_BUSY';return;}
  const roi=parseWindowRoi(this.probeRoi);
  if(roi===null){this.probeStatus='ROI_INVALID';return;}
  const r=this.singleSand.reserveCapture(roi,this.probeFixtureKey,Number(this.maxHoldText));
  this.probeStatus=JSON.stringify(r);
  if(r.state==='RESERVED'){
   if(this.t394WaitB){this.t394WaitB=false;this.model.t394InjectB(p013NowMs());}
   this.schedule();
  }
  const context=getContext(this) as common.UIAbilityContext;
  await new DiagnosticColdFiles(context.filesDir).save('t413-reserve.json',JSON.stringify({readSerial:++this.statusSerial,reply:r}));
  await this.statusCold();
 }
 private async exportWindow():Promise<void> {
  if(this.exporting)return;
  const scope=this.singleSand.holdStatus();
  if(this.evidence===null)this.evidence=this.singleSand.takeWindowProbe();
  if(this.evidence===null){
   if(this.archivedReceipt!==null&&this.archivedAttempt===scope.attemptId){
    const context=getContext(this) as common.UIAbilityContext;
    await new DiagnosticColdFiles(context.filesDir).save('t413-native.json',JSON.stringify(this.archivedReceipt));
   }else this.probeStatus='NATIVE_ARCHIVE_PENDING';
   return;
  }
  this.exporting=true;const e=this.evidence;e.capture=scope;
  try{
   const context=getContext(this) as common.UIAbilityContext;
   const receipt=JSON.parse(await saveWindowProbe(context.filesDir,e)) as ColdFile;
   const ack=this.singleSand.ackArchive(scope.attemptId,scope.requestId,receipt.sha256);
   if(ack.state==='REJECTED')throw new Error(ack.reason);
   this.archivedReceipt=receipt;this.archivedAttempt=scope.attemptId;
   this.evidence=null;this.probeStatus='NATIVE_ARCHIVE_SAVED '+scope.attemptId;
   await new DiagnosticColdFiles(context.filesDir).save('t413-native.json',JSON.stringify(receipt));
   console.info('WS_EVENT t413_native '+JSON.stringify(receipt));
  }catch(error){this.probeStatus='NATIVE_EXPORT_FAILED_RETRY';}
  finally{this.exporting=false;await this.statusCold();}
 }
 private async clockCold():Promise<void> {
  const s=this.singleSand.holdStatus(),sample=this.singleSand.sampleHoldClock(s.holdId);
  if(sample===null){this.probeStatus='CLOCK_HOLD_NOT_OPEN';return;}
  const context=getContext(this) as common.UIAbilityContext;
  const f=await new DiagnosticColdFiles(context.filesDir).save('t413-clock.json',JSON.stringify(sample));
  console.info('WS_EVENT t413_clock '+JSON.stringify(f));
 }
 private async writeTerminal(directory:string,name:string,text:string):Promise<ColdFile> {
  const path=directory+'/'+name;
  const temp=name+'.partial';
  if(fs.accessSync(path)&&fs.readTextSync(path)!==text)throw new Error('TERMINAL_FILE_CONFLICT');
  const written=await new DiagnosticColdFiles(directory).save(temp,text);
  if(!fs.accessSync(path))fs.renameSync(directory+'/'+temp,path);
  else fs.unlinkSync(directory+'/'+temp);
  const receipt:ColdFile={name:name,bytes:written.bytes,sha256:written.sha256};return receipt;
 }
 private async terminalCold(seal:boolean):Promise<void> {
  if(this.exporting)return;this.exporting=true;
  const s=this.singleSand.holdStatus();
  try{
   if(seal){const reply=this.singleSand.sealCapture(s.holdId);if(reply.state==='REJECTED'){this.probeStatus=reply.reason;return;}}
   const text=this.singleSand.capture.terminal();if(!text){this.probeStatus='TERMINAL_NOT_READY';return;}
   const context=getContext(this) as common.UIAbilityContext;
   const terminal=this.singleSand.holdStatus();
   const name='t413-'+s.attemptId+'-'+(s.holdId||'nohold')+'-'+terminal.state+'.json';
   const f=await this.writeTerminal(context.filesDir,name,text);
   const reply=this.singleSand.capture.savedTerminal(s.attemptId,s.holdId,f.sha256);
   if(reply.state==='REJECTED')throw new Error(reply.reason);
   await new DiagnosticColdFiles(context.filesDir).save('t413-terminal.json',JSON.stringify(f));
   console.info('WS_EVENT t413_terminal '+JSON.stringify(f));
  }catch(error){this.probeStatus='TERMINAL_EXPORT_FAILED_RETRY';}
  finally{this.exporting=false;await this.statusCold();}
 }
 private async cancel():Promise<void> {
  const s=this.singleSand.holdStatus();this.singleSand.cancelCapture(s.attemptId,'EXPLICIT_CANCEL');
  if(this.singleSand.takeWindowProbe()!==null)await this.exportWindow();
  await this.terminalCold(false);
 }
 private release():void {
  const s=this.singleSand.holdStatus(),r=this.singleSand.releaseCapture(s.attemptId,s.holdId);
  this.probeStatus=JSON.stringify(r);if(r.state==='RELEASED'){this.lastArmName=s.state==='SEALED'?this.singleSand.t394.name:this.lastArmName;this.previous=0;this.schedule();}
  this.statusCold();
 }
 private captureVisibility(visible:boolean,ratio:number):void {
  const full=visible&&ratio===1;
  if(full===this.fullyVisible)return;
  this.fullyVisible=full;
  const g=this.singleSand.capture.geometry();g.visible=full&&this.active;
  g.geometry=++this.t394GeometryEpoch;this.layout=g;this.singleSand.captureGeometry(g);this.schedule();
 }
 private area(area:Area):void {
  const x=Number(area.globalPosition.x),y=Number(area.globalPosition.y),w=Number(area.width),h=Number(area.height),scale=this.getUIContext().vp2px(1);
  if(x!==this.layout.globalX||y!==this.layout.globalY||w!==this.layout.widthVp||h!==this.layout.heightVp||scale!==this.layout.vp2px){
   this.t394GeometryEpoch++;
   const g=new CaptureGeometry();g.globalX=x;g.globalY=y;g.widthVp=w;g.heightVp=h;g.vp2px=scale;
   g.geometry=this.t394GeometryEpoch;g.visible=this.active&&this.fullyVisible;g.configuration='FIXED_PANEL_XCOMPONENT_V2';
   this.layout=g;this.singleSand.captureGeometry(g);this.surfaceWidth=w;this.surfaceHeight=h;
   this.probeFixtureKey='';this.schedule();
  }
 }
