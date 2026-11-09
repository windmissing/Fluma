 private readonly roiInput:TextInputController=new TextInputController();
 private readonly planInput:TextInputController=new TextInputController();
 private readonly limitInput:TextInputController=new TextInputController();
 private readonly prepareGate:PrepareReadiness=new PrepareReadiness((reason:string)=>this.singleSand.capture.invalidate(reason));
 @State private prepareText:string='CONFIGURING';
 @State private fixedKeyboardConfirmed:boolean=false;
 private prepareWindow:window.Window|null=null;
 private avoidListener:((event:window.AvoidAreaOptions)=>void)|null=null;
 private windowListener:((event:window.WindowEventType)=>void)|null=null;
 private heightListener:((height:number)=>void)|null=null;
 private modeListener:((mode:window.WindowStatusType)=>void)|null=null;
 private windowShown:boolean=false;private keyboardHeight:number=-1;
 private prepareFrame:T426PrepareFrame|null=null;private prepareTimer:number=-1;
 private prepareFileState:string='';private windowSubscribed:boolean=false;
 private pendingPrepareId:number=0;
 private prepareUpdate():void {
  this.prepareText=this.prepareGate.state+' '+this.prepareGate.reason;
  const identity=this.prepareGate.pageGeneration+'-'+this.prepareGate.prepareId+'-'+this.prepareGate.state;
  if(identity!==this.prepareFileState&&(this.prepareGate.prepareId>0||this.prepareGate.state==='FAILED')){this.prepareFileState=identity;this.prepareCold();}
 }
 private async prepareCold():Promise<void> {
  const text=this.prepareGate.report(),id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration;
  try{
   const context=getContext(this) as common.UIAbilityContext;
   const record=await new DiagnosticColdFiles(context.filesDir).save('t426-prepare-'+this.prepareGate.session+'-'+generation+'-'+id+'-'+this.prepareGate.state+'-'+this.prepareGate.observationSerial+'.json',text);
   console.info('WS_EVENT t426_prepare '+JSON.stringify(record));
  }catch(error){if(this.prepareGate.currentId(id,generation)){this.prepareGate.fail('PREPARE_EXPORT_FAILED');this.prepareText=this.prepareGate.state+' '+this.prepareGate.reason;}}
 }
 private prepareConnect():void {
  this.prepareGate.show();this.configuring=true;this.windowShown=true;
  this.singleSand.bindPrepareGateCold(():string=>{
   try{
    const snapshot=this.readPrepareSnapshot();
    const reason=this.prepareGate.check(snapshot,p013NowMs());this.prepareUpdate();return reason;
   }catch(error){this.prepareGate.fail('PREPARE_OBSERVATION_FAILED');this.prepareUpdate();return 'PREPARE_OBSERVATION_FAILED';}
  });
  const generation=this.prepareGate.pageGeneration;
  try{
   const w=PrepareMainWindow.value;if(w===null)throw new Error('MAIN_WINDOW_UNAVAILABLE');
   this.prepareWindow=w;
   this.avoidListener=(event:window.AvoidAreaOptions):void=>{
    if(!this.active||this.prepareGate.pageGeneration!==generation||this.prepareWindow!==w)return;
    if(event.type===window.AvoidAreaType.TYPE_KEYBOARD){this.prepareGate.changed('KEYBOARD_AVOID_CHANGED');this.observePrepare();}
   };
   this.windowListener=(event:window.WindowEventType):void=>{
    if(!this.active||this.prepareGate.pageGeneration!==generation||this.prepareWindow!==w)return;
    if(event===window.WindowEventType.WINDOW_HIDDEN||event===window.WindowEventType.WINDOW_INACTIVE||event===window.WindowEventType.WINDOW_DESTROYED){
     this.windowShown=false;this.prepareGate.invalidate('PREPARE_WINDOW_CHANGED');
    }else if(event===window.WindowEventType.WINDOW_SHOWN||event===window.WindowEventType.WINDOW_ACTIVE)this.windowShown=true;
    this.observePrepare();
   };
   this.heightListener=(height:number):void=>{
    if(!this.active||this.prepareGate.pageGeneration!==generation||this.prepareWindow!==w)return;
    this.keyboardHeight=height;this.prepareGate.changed('KEYBOARD_HEIGHT_CHANGED');this.observePrepare();
   };
   this.modeListener=(_mode:window.WindowStatusType):void=>{
    if(!this.active||this.prepareGate.pageGeneration!==generation||this.prepareWindow!==w)return;
    this.prepareGate.invalidate('WINDOW_CONFIGURATION_CHANGED');this.observePrepare();
   };
   w.on('avoidAreaChange',this.avoidListener);w.on('windowEvent',this.windowListener);
   w.on('keyboardHeightChange',this.heightListener);w.on('windowStatusChange',this.modeListener);this.windowSubscribed=true;
   this.observePrepare();this.postPrepareFrame();
  }catch(error){this.prepareApiFailure('window.subscribe',error,0,p013NowMs(),generation);this.prepareGate.fail('PREPARE_WINDOW_OBSERVATION_FAILED');}
  this.prepareUpdate();
 }
 private prepareDisconnect():void {
  this.prepareGate.hide();this.pendingPrepareId=0;this.windowSubscribed=false;
  if(this.prepareTimer!==-1){clearTimeout(this.prepareTimer);this.prepareTimer=-1;}
  if(this.prepareFrame!==null){this.prepareFrame.owner=null;this.prepareFrame=null;}
  const w=this.prepareWindow;this.prepareWindow=null;
  if(w!==null){
   try{if(this.avoidListener!==null)w.off('avoidAreaChange',this.avoidListener);}catch(e){console.error('T426 off avoid '+String(e));}
   try{if(this.windowListener!==null)w.off('windowEvent',this.windowListener);}catch(e){console.error('T426 off window '+String(e));}
   try{if(this.heightListener!==null)w.off('keyboardHeightChange',this.heightListener);}catch(e){console.error('T426 off height '+String(e));}
   try{if(this.modeListener!==null)w.off('windowStatusChange',this.modeListener);}catch(e){console.error('T426 off mode '+String(e));}
  }
  this.avoidListener=null;this.windowListener=null;this.heightListener=null;this.modeListener=null;
  this.prepareUpdate();
 }
 private postPrepareFrame():void {
  if(!this.active||this.prepareFrame!==null)return;
  const callback=new T426PrepareFrame();callback.owner=this;callback.generation=this.prepareGate.pageGeneration;
  callback.prepareId=this.prepareGate.prepareId;this.prepareFrame=callback;
  this.getUIContext().postFrameCallback(callback);
 }
 prepareIdle(callback:T426PrepareFrame):void {
  if(this.prepareFrame!==callback)return;this.prepareFrame=null;
  if(!this.active||callback.generation!==this.prepareGate.pageGeneration)return;
  this.observePrepare();
  if(callback.prepareId===this.prepareGate.prepareId)this.prepareGate.frame(callback.frameNs,p013NowMs());
  this.prepareUpdate();this.postPrepareFrame();
 }
 private observePrepare():void {
  try{this.prepareGate.observe(this.readPrepareSnapshot());}
  catch(error){this.prepareGate.fail('PREPARE_OBSERVATION_FAILED');this.prepareApiFailure('window.snapshot',error,this.prepareGate.prepareId,p013NowMs(),this.prepareGate.pageGeneration);}
  this.prepareUpdate();
 }
 private rect(r:window.Rect):PrepareRect {const out=new PrepareRect();out.x=r.left;out.y=r.top;out.width=r.width;out.height=r.height;return out;}
 private readPrepareSnapshot():PrepareSnapshot {
  const s=new PrepareSnapshot(),w=this.prepareWindow,g=this.layout;
  if(w===null||w!==PrepareMainWindow.value)return s;
  const props=w.getWindowProperties(),area=w.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD);
  const component=this.getUIContext().getComponentUtils().getRectangleById('t426-surface');
  const native=this.singleSand.prepareReadinessCold();
  s.windowId=props.id;s.windowGeneration=PrepareMainWindow.generation;
  s.windowActive=this.active&&this.windowShown&&w.isFocused();s.windowRegistered=this.windowSubscribed;
  s.windowRect=this.rect(props.windowRect);s.windowMode=w.getWindowStatus();s.deviceType=deviceInfo.deviceType;
  s.keyboardConfiguration=this.fixedKeyboardConfirmed?'TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD':'UNKNOWN';
  s.avoid=[this.rect(area.topRect),this.rect(area.rightRect),this.rect(area.bottomRect),this.rect(area.leftRect)];
  s.keyboardHeight=this.keyboardHeight;s.geometry=g.geometry;s.configurationRevision=this.prepareGate.configurationRevision;
  s.globalX=g.globalX;s.globalY=g.globalY;s.widthVp=g.widthVp;s.heightVp=g.heightVp;s.vp2px=this.getUIContext().vp2px(1);
  s.fullyVisible=this.fullyVisible;s.componentPresent=component.size.width>0&&component.size.height>0;
  s.component.x=component.windowOffset.x;s.component.y=component.windowOffset.y;s.component.width=component.size.width;s.component.height=component.size.height;
  s.vpBoxWindowPx.x=this.getUIContext().vp2px(g.globalX);s.vpBoxWindowPx.y=this.getUIContext().vp2px(g.globalY);
  s.vpBoxWindowPx.width=this.getUIContext().vp2px(g.widthVp);s.vpBoxWindowPx.height=this.getUIContext().vp2px(g.heightVp);
  s.vpBoxScreenPx.x=s.windowRect.x+s.vpBoxWindowPx.x;s.vpBoxScreenPx.y=s.windowRect.y+s.vpBoxWindowPx.y;
  s.vpBoxScreenPx.width=s.vpBoxWindowPx.width;s.vpBoxScreenPx.height=s.vpBoxWindowPx.height;
  s.screen.x=component.screenOffset.x;s.screen.y=component.screenOffset.y;s.screen.width=component.size.width;s.screen.height=component.size.height;
  s.transformIdentity=component.scale.x===1&&component.scale.y===1&&component.scale.z===1&&
   component.translate.x===0&&component.translate.y===0&&component.translate.z===0&&component.rotate.angle===0;
  s.orientation=s.windowRect.width>s.windowRect.height?1:0;
  s.controller=native.controller;s.session=native.session;s.surface=native.surface;s.context=native.context;
  s.nativeGeometry=native.geometry;s.viewportWidth=native.width;s.viewportHeight=native.height;
  s.nativeReady=native.ready;s.run=native.run;s.opticsRevision=native.revision;
  return s;
 }
 private inputCreated(index:number):void {this.prepareGate.inputCreated(index);}
 private inputEdit(index:number,value:boolean):void {this.prepareGate.inputEvent(index,value,null);this.observePrepare();}
 private inputFocus(index:number,value:boolean):void {this.prepareGate.inputEvent(index,null,value);this.observePrepare();}
 private prepareChanged():void {this.prepareGate.inputChanged();this.observePrepare();}
 private prepareApiFailure(api:string,error:Object,id:number,start:number,generation:number):void {
  const raw=error as PrepareCallError,record=new PrepareApiRecord();
  record.api=api;record.prepareId=id;record.startMonoMs=start;record.endMonoMs=p013NowMs();
  record.code=raw.code??-1;record.name=raw.name??'';record.message=raw.message??String(error);record.runStatus=String(raw.runStatus??'');
  this.prepareGate.record(record,id,generation);
 }
 private prepare():void {
  if(!this.windowSubscribed){this.prepareGate.fail('PREPARE_WINDOW_OBSERVATION_FAILED');this.prepareUpdate();return;}
  if(!this.prepareGate.begin(p013NowMs(),this.allowed()&&this.active))return;
  const id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration;
  if(this.prepareTimer!==-1)clearTimeout(this.prepareTimer);
  this.pendingPrepareId=id;
  const controllers:TextInputController[]=[this.roiInput,this.planInput,this.limitInput];
  for(let i=0;i<controllers.length;i++){
   const start=p013NowMs();
   try{controllers[i].stopEditing();const record=new PrepareApiRecord();record.api='TextInputController.stopEditing.'+i;
    record.prepareId=id;record.startMonoMs=start;record.endMonoMs=p013NowMs();this.prepareGate.record(record,id,generation);
   }catch(error){this.prepareApiFailure('TextInputController.stopEditing.'+i,error,id,start,generation);}
  }
  const start=p013NowMs();
  try{this.getUIContext().getFocusController().clearFocus();const record=new PrepareApiRecord();record.api='FocusController.clearFocus';
   record.prepareId=id;record.startMonoMs=start;record.endMonoMs=p013NowMs();this.prepareGate.record(record,id,generation);
  }catch(error){this.prepareApiFailure('FocusController.clearFocus',error,id,start,generation);}
  if(!this.prepareGate.currentId(id,generation))return;
  this.configuring=false;this.prepareUpdate();this.schedule();this.postPrepareFrame();
  this.prepareTimer=setTimeout(()=>{
   if(!this.prepareGate.currentId(id,generation)||this.pendingPrepareId!==id)return;
   this.prepareGate.elapsed(p013NowMs());this.prepareUpdate();
  },3000);
 }
