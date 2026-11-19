import { PrepareReadinessV4, PrepareAttemptOwner, prepareUtf8Bytes } from '../render/PrepareReadinessV4';
import componentUtils from '@ohos.arkui.componentUtils';
import { RasterGeometryController, RasterAllocation, RasterRect, RasterNative, RasterAncestor540, rasterNodeReason } from '../render/RasterGeometry';
import window from '@ohos.window';
import deviceInfo from '@ohos.deviceInfo';
import { PrepareReadiness, PrepareSnapshot, PrepareRect, PrepareApiRecord } from '../render/PrepareReadiness';
import { PrepareMainWindow } from '../render/PrepareMainWindow';
import fs from '@ohos.file.fs';
import { CaptureGeometry, HoldStatus } from '../render/T413Capture';
import { ColdFile } from '../physics/GpuBridgeTimelineCold';
import { T394Input } from '../render/T394Phase';
import { WindowProbeEvidence,parseWindowRoi,saveWindowProbe } from '../render/SingleSandWindowCold';
import { P013SingleSandController } from '../render/P013SingleSandController';
import { common } from '@kit.AbilityKit';
import { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';
import sensor from '@ohos.sensor';
import hilog from '@ohos.hilog';
import { P051GravityConfig,P051GravityInputAdapter } from '../physics/P051GravityInputAdapter';
import { p013NowMs } from '../render/P013RenderController';
import { FrameCallback, UIContext, UIObserver } from '@ohos.arkui.UIContext';
import { PrepareFrameObservation } from '../render/PrepareFrameObservation';
import { P051DerivedDemoV2 } from '../physics/derived/P051DerivedDemoV2';
import { DerivedPublishedSlotV2 } from '../physics/derived/P051DerivedFacadeV2';
import { P013SceneController } from '../render/P013SceneController';
import { P012FrameContext,P012AppearanceTier,P012Ablation,P012FillBox } from '../render/P012RenderTypes';
class DerivedFrameV2 extends FrameCallback {owner:P051DerivedV2Demo|null=null;onFrame(ns:number):void{this.owner?.tick(ns);}}
interface PrepareCallError {code?:number;name?:string;message?:string;runStatus?:string;}
class T426PrepareFrame extends FrameCallback {
 owner:P051DerivedV2Demo|null=null;observation:PrepareFrameObservation|null=null;
 onFrame(ns:number):void {this.owner?.prepareFrameEnter(this,ns);}
}
@Entry
@Component
struct P051DerivedV2Demo {
 private readonly singleSand=new P013SingleSandController();
 private readonly useSingleSand:boolean=true;
 @State private probeRoi:string='0,0,16,16';
 @State private probeStatus:string='窗口读回未预约';
 @State private fixtureLocked:boolean=false;
 private probeFixtureKey:string='';
 private evidence:WindowProbeEvidence|null=null;
 private exporting:boolean=false;
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
  this.refreshRaster();
  const s=this.singleSand.holdStatus();this.captureText=s.state+' '+s.holdId+' '+s.archiveState+' '+s.reason;
  const context=getContext(this) as common.UIAbilityContext;
  const f=await new DiagnosticColdFiles(context.filesDir).save('t413-status.json',JSON.stringify({readSerial:++this.statusSerial,status:s}));
  console.info('WS_EVENT t413_status '+JSON.stringify(f));
 }
 private readonly roiInput:TextInputController=new TextInputController();
 private readonly planInput:TextInputController=new TextInputController();
 private readonly limitInput:TextInputController=new TextInputController();
 private readonly prepareGate:PrepareReadinessV4=new PrepareReadinessV4((reason:string)=>this.singleSand.capture.invalidate(reason));
 @State private reserveEnabled:boolean=false;
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
  this.reserveEnabled=this.prepareGate.state==='READY'&&!this.configuring&&!this.exporting&&this.evidence===null&&this.singleSand.capture.configurable();
  if(this.prepareGate.state!=='PREPARING')this.stopPrepareObservation();
  this.prepareText=this.prepareGate.state+' '+this.prepareGate.reason;
  const identity=this.prepareGate.pageGeneration+'-'+this.prepareGate.prepareId+'-'+this.prepareGate.state;
  if(identity!==this.prepareFileState&&(this.prepareGate.prepareId>0||this.prepareGate.state==='FAILED')){this.prepareFileState=identity;this.prepareCold();}
 }
 private prepareColdPending659:boolean=false;
 private async prepareCold():Promise<void> {
  this.prepareColdPending659=true;
  const readiness=this.prepareGate.report(),full='{"revision":"T659_PREPARE_OWNER_VALUE_V1","observation":'+JSON.stringify(this.prepareObservation)+',"readiness":'+readiness+'}';
  const text=prepareUtf8Bytes(full)<=16384?full:'{"revision":"T659_PREPARE_OWNER_VALUE_V1","observationTruncated":true,"readiness":'+readiness+'}',id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration;
  try{if(prepareUtf8Bytes(text)>16384)throw new Error('PREPARE_COLD_UTF8_LIMIT');

   const context=getContext(this) as common.UIAbilityContext;
   const record=await new DiagnosticColdFiles(context.filesDir).save('t540-prepare-'+this.prepareGate.session+'-'+generation+'-'+id+'-'+this.prepareGate.state+'-'+this.prepareGate.observationSerial+'.json',text);
   console.info('WS_EVENT t426_prepare '+JSON.stringify(record));
  }catch(error){if(this.prepareGate.currentId(id,generation)){this.prepareRawFailure659('prepare.cold.write',error,id,generation,this.prepareBranch);this.prepareText=this.prepareGate.state+' '+this.prepareGate.reason;}}finally{this.prepareColdPending659=false;}
 }
 private prepareConnect():void {
  this.stopPrepareObservation();this.prepareObservation=null;this.prepareGate.show();this.prepareLastOwner659=null;this.configuring=true;this.windowShown=true;
  this.singleSand.bindPrepareGateCold(():string=>{
   const coldId=this.prepareGate.prepareId,coldGeneration=this.prepareGate.pageGeneration,coldBranch=this.prepareBranch;
   try{
    this.refreshRaster();
    const o=this.prepareObservation;
    if(this.prepareGate.state==='READY'&&(o===null||this.configuring||this.prepareBranch!==o.branch||
       !o.matchesLayout(this.prepareAnchor(this.getUIContext(),o.branch)))){
     this.prepareGate.invalidate('PREPARE_STALE_LAYOUT');this.prepareUpdate();return 'PREPARE_STALE_LAYOUT';
    }
    const readId=this.prepareGate.prepareId,readGeneration=this.prepareGate.pageGeneration,readBranch=this.prepareBranch,readWindow=this.prepareWindow,readWg=PrepareMainWindow.generation;
    const snapshot=this.readPrepareSnapshot();
    if(!this.prepareScope659(readId,readGeneration,readBranch,readWindow,readWg)){this.prepareGate.discardOwnerCallback();return 'PREPARE_STALE';}
    const reason=this.prepareGate.check(snapshot,p013NowMs());this.prepareUpdate();return reason;
   }catch(error){this.prepareRawFailure659('reserve.fresh',error,coldId,coldGeneration,coldBranch);this.prepareUpdate();return 'PREPARE_OBSERVATION_FAILED';}
  });
  const generation=this.prepareGate.pageGeneration;
  try{
   const w=PrepareMainWindow.value;if(w===null)throw new Error('MAIN_WINDOW_UNAVAILABLE');
   this.prepareWindow=w;this.refreshRaster();
   this.subscribePrepareWindow659();
   this.observePrepare();
  }catch(error){this.prepareApiFailure('window.subscribe',error,0,p013NowMs(),generation);this.prepareGate.fail('PREPARE_WINDOW_OBSERVATION_FAILED');}
  this.prepareUpdate();
 }
 private prepareDisconnect():void {
  this.prepareGate.hide();this.stopPrepareObservation();this.pendingPrepareId=0;this.windowSubscribed=false;
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
 private prepareObservedWindow659:window.Window|null=null;
 private prepareContext:UIContext|null=null;
 private prepareObserver:UIObserver|null=null;
 private prepareLayoutListener:(()=>void)|null=null;
 private prepareDrawListener:(()=>void)|null=null;
 private prepareObservation:PrepareFrameObservation|null=null;
 @State private prepareBranch:number=0;
 @State private preparePaint:boolean=false;

 private stopPrepareObservation():void {
  if(this.prepareTimer!==-1){clearTimeout(this.prepareTimer);this.prepareTimer=-1;}
  if(this.prepareFrame!==null){this.prepareFrame.owner=null;this.prepareFrame=null;}
  const observer=this.prepareObserver,layout=this.prepareLayoutListener,draw=this.prepareDrawListener;
  this.prepareObserver=null;this.prepareContext=null;this.prepareLayoutListener=null;this.prepareDrawListener=null;
  if(observer!==null){
   try{if(layout!==null)observer.off('didLayout',layout);}catch(e){console.error('T455 didLayout off '+String(e));}
   try{if(draw!==null)observer.off('willDraw',draw);}catch(e){console.error('T455 willDraw off '+String(e));}
  }
 }

 private prepareOwner(o:PrepareFrameObservation):boolean {
  return this.active&&this.prepareGate.state==='PREPARING'&&this.prepareObservation===o&&
   this.prepareGate.currentId(o.prepareId,o.pageGeneration)&&this.prepareBranch===o.branch&&
   this.prepareWindow!==null&&this.prepareWindow===this.prepareObservedWindow659&&this.prepareWindow===PrepareMainWindow.value&&
   PrepareMainWindow.generation===o.windowGeneration;
 }

 private prepareAnchor(context:UIContext,branch:number):string {
  try {
   const rect=context.getComponentUtils().getRectangleById('t449-action-'+branch);
   if(rect.size.width<=0||rect.size.height<=0)return '';
   const actual=context.getComponentUtils().getRectangleById('t463-xcomponent');
   return JSON.stringify([rect.size.width,rect.size.height,rect.windowOffset.x,rect.windowOffset.y,
     this.rasterRevision,'t463-xcomponent',this.rasterRect(actual),this.rasterRect(actual,true)]);
  }catch(e){return '';}
 }

 private prepareLayoutChanged():void {
  if(this.prepareGate.state==='PREPARING')this.prepareObservation?.layoutChanged();
  if(this.prepareGate.state==='READY')this.prepareGate.changed('PREPARE_LAYOUT_CHANGED');
 }

 private startPrepareObservation(id:number,generation:number):boolean {
  this.stopPrepareObservation();
  const w=this.prepareWindow;
  if(w===null)return false;
  const branch=this.prepareBranch+1;
  const context=this.getUIContext();
  const o=new PrepareFrameObservation(id,generation,w.getWindowProperties().id,PrepareMainWindow.generation,branch);
  this.prepareObservation=o;this.prepareObservedWindow659=w;
  this.prepareContext=context;
  try{
   const observer=context.getUIObserver();
   this.prepareObserver=observer;
   this.prepareLayoutListener=():void=>{this.prepareCallback659('frame.layout',o,():void=>{
    if(!this.prepareOwner(o)||this.configuring){o.drop(p013NowMs(),0,'STALE_LAYOUT_OWNER');return;}
    if(this.refreshRaster()||this.raster.plan.didLayoutSerial<=this.raster.plan.requestedAtLayout){o.drop(p013NowMs(),0,'RASTER_PLAN_WAIT_LAYOUT');return;}
    o.didLayout(p013NowMs(),this.prepareAnchor(context,branch));
    if(o.matchesLayout(this.prepareAnchor(context,branch)))this.raster.acknowledge(this.rasterRevision,o.layoutGeneration);});
   };
   this.prepareDrawListener=():void=>{this.prepareCallback659('frame.draw',o,():void=>{
    if(!this.prepareOwner(o)||this.configuring){o.drop(p013NowMs(),0,'STALE_DRAW_OWNER');return;}
    const now=p013NowMs();
    this.rasterDrawSerial540++;
    if(!this.prepareFrameDeadline(o,now))return;
    const ns=o.draw(now,this.prepareAnchor(context,branch));
    if(ns===0)return;if(this.raster.noteDraw(this.rasterDrawSerial540,ns)){this.layout.raster=this.raster.snapshot(this.layout.geometry);this.singleSand.captureGeometry(this.layout);}
    try{
     const snapshot=this.readPrepareSnapshot();
     if(!this.prepareOwner(o)){o.drop(now,ns,'STALE_DRAW_READ');this.prepareGate.discardOwnerCallback();return;}
     if(snapshot.windowId!==o.windowId||snapshot.windowGeneration!==o.windowGeneration){
      this.prepareGate.invalidate('PREPARE_STALE');this.prepareUpdate();return;
     }
     o.sample(now,ns);
     this.prepareGate.observe(snapshot);
     this.prepareGate.frame(ns,now);
    }catch(e){this.prepareRawFailure659('frame.snapshot',e,o.prepareId,o.pageGeneration,o.branch);}
    this.prepareUpdate();});
   };
   observer.on('didLayout',this.prepareLayoutListener);
   observer.on('willDraw',this.prepareDrawListener);
  }catch(e){
   this.prepareApiFailure('UIObserver.subscribe',e,id,p013NowMs(),generation);
   this.stopPrepareObservation();return false;
  }
  this.prepareBranch=branch;
  try{this.subscribePrepareWindow659();}catch(error){this.prepareRawFailure659('window.rebind',error,id,generation,branch);this.stopPrepareObservation();return false;}
  return true;
 }

 private postPrepareFrame():void {
  const o=this.prepareObservation,context=this.prepareContext;
  if(o===null||context===null||!this.prepareOwner(o)||this.prepareFrame!==null)return;
  const callback=new T426PrepareFrame();callback.owner=this;callback.observation=o;this.prepareFrame=callback;
  try {context.postFrameCallback(callback);o.post(p013NowMs());}
  catch(e){this.prepareRawFailure659('frame.post',e,o.prepareId,o.pageGeneration,o.branch);this.prepareUpdate();}
 }

 prepareFrameEnter(callback:T426PrepareFrame,ns:number):void {
  const o=callback.observation;
  if(o===null)return;
  if(this.prepareFrame!==callback){o.drop(p013NowMs(),ns,'STALE_FRAME_SLOT');return;}
  if(!this.prepareOwner(o)){o.drop(p013NowMs(),ns,'STALE_FRAME_OWNER');this.prepareGate.discardOwnerCallback();return;}
  this.prepareFrame=null;callback.owner=null;
  const now=p013NowMs();
  if(!this.prepareFrameDeadline(o,now))return;
  o.frame(now,ns);
  // A fixed-size status line invalidation makes the static page request draw.
  // The ticket still requires an independently delivered willDraw callback.
  this.preparePaint=!this.preparePaint;
  this.postPrepareFrame();
 }

 private prepareFrameDeadline(o:PrepareFrameObservation,now:number):boolean {
  if(!this.prepareOwner(o)){o.drop(now,0,'STALE_DEADLINE_OWNER');this.prepareGate.discardOwnerCallback();return false;}
  if(now-this.prepareGate.startedMonoMs()>=3000){
   this.prepareGate.reason=o.timeoutReason(this.prepareGate.reason==='WAIT_OBSERVATIONS'?'':this.prepareGate.reason);
  }
  if(this.prepareGate.elapsed(now))return true;
  this.prepareUpdate();return false;
 }
 private observePrepare():void {
  const id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration,branch=this.prepareBranch,w=this.prepareWindow,wg=PrepareMainWindow.generation;
  try{const snapshot=this.readPrepareSnapshot();if(!this.prepareScope659(id,generation,branch,w,wg)){this.prepareGate.discardOwnerCallback();return;}this.prepareGate.observe(snapshot);}
  catch(error){this.prepareRawFailure659('window.snapshot',error,id,generation,branch);}
  this.prepareUpdate();
 }
 private rect(r:window.Rect):PrepareRect {const out=new PrepareRect();out.x=r.left;out.y=r.top;out.width=r.width;out.height=r.height;return out;}
 private readPrepareSnapshot():PrepareSnapshot {
  const readId=this.prepareGate.prepareId,readGeneration=this.prepareGate.pageGeneration,readBranch=this.prepareBranch,readWindowGeneration=PrepareMainWindow.generation;

  this.refreshRaster();
  const s=new PrepareSnapshot(),w=this.prepareWindow,g=this.layout;
  if(w===null||w!==PrepareMainWindow.value){this.prepareGate.invalidate('OWNER_CHANGED');return s;}
  const props=w.getWindowProperties(),area=w.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD);
  const component=this.getUIContext().getComponentUtils().getRectangleById('t463-xcomponent');
  const native=this.singleSand.prepareReadinessCold();
  s.layoutGeneration=this.prepareObservation?.layoutGeneration??0;s.raster=this.raster.snapshot(g.geometry);
  s.windowId=props.id;s.windowGeneration=PrepareMainWindow.generation;
  s.windowActive=this.active&&this.windowShown&&w.isFocused();s.windowRegistered=this.windowSubscribed;
  s.windowRect=this.rect(props.windowRect);s.windowMode=w.getWindowStatus();s.deviceType=deviceInfo.deviceType;
  s.keyboardConfiguration=this.fixedKeyboardConfirmed?'TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD':'UNKNOWN';
  s.avoid=[this.rect(area.topRect),this.rect(area.rightRect),this.rect(area.bottomRect),this.rect(area.leftRect)];
  s.keyboardHeight=this.keyboardHeight;s.geometry=g.geometry;s.configurationRevision=this.prepareGate.configurationRevision;
  s.globalX=g.globalX;s.globalY=g.globalY;s.widthVp=g.widthVp;s.heightVp=g.heightVp;s.vp2px=this.getUIContext().vp2px(1);
  s.fullyVisible=this.fullyVisible;s.componentPresent=component.size.width>0&&component.size.height>0;
  s.component.x=component.windowOffset.x;s.component.y=component.windowOffset.y;s.component.width=component.size.width;s.component.height=component.size.height;
  s.vpBoxWindowPx.x=component.windowOffset.x;s.vpBoxWindowPx.y=component.windowOffset.y;
  s.vpBoxWindowPx.width=component.size.width;s.vpBoxWindowPx.height=component.size.height;
  s.vpBoxScreenPx.x=component.screenOffset.x;s.vpBoxScreenPx.y=component.screenOffset.y;
  s.vpBoxScreenPx.width=component.size.width;s.vpBoxScreenPx.height=component.size.height;
  s.screen.x=component.screenOffset.x;s.screen.y=component.screenOffset.y;s.screen.width=component.size.width;s.screen.height=component.size.height;
  s.transformIdentity=component.scale.x===1&&component.scale.y===1&&component.scale.z===1&&
   component.translate.x===0&&component.translate.y===0&&component.translate.z===0&&component.rotate.angle===0;
  s.orientation=s.windowRect.width>s.windowRect.height?1:0;
  s.controller=native.controller;s.session=native.session;s.surface=native.surface;s.context=native.context;
  s.nativeGeometry=native.geometry;s.viewportWidth=native.width;s.viewportHeight=native.height;
  s.nativeReady=native.ready;s.run=native.run;s.opticsRevision=native.revision;
  if(!this.prepareScope659(readId,readGeneration,readBranch,w,readWindowGeneration)){this.prepareGate.discardOwnerCallback();return new PrepareSnapshot();}
  if(this.prepareObservation!==null&&this.prepareObservation.prepareId===readId&&(this.prepareGate.state==='PREPARING'||this.prepareGate.state==='READY')){const fresh=this.prepareOwnerValue659(s);this.prepareLastOwner659=fresh;this.prepareGate.sampleFrame(fresh);}
  return s;
 }
 private inputCreated(index:number):void {this.prepareGate.inputCreated(index);}
 private inputEdit(index:number,value:boolean):void {this.prepareGate.inputEvent(index,value,null);this.observePrepare();}
 private inputFocus(index:number,value:boolean):void {this.prepareGate.inputEvent(index,null,value);this.observePrepare();}
 private prepareChanged():void {this.prepareGate.inputChanged();this.observePrepare();}
 private prepareApiFailure(api:string,error:Object,id:number,start:number,generation:number):void {
  this.prepareRawFailure659(api,error,id,generation,this.prepareBranch);
  const raw=error as PrepareCallError,record=new PrepareApiRecord();
  record.api=api;record.prepareId=id;record.startMonoMs=start;record.endMonoMs=p013NowMs();
  record.code=raw.code??-1;record.name=raw.name??'';record.message=raw.message??String(error);record.runStatus=String(raw.runStatus??'');
  this.prepareGate.record(record,id,generation);
 }
 private prepare():void {
  if(!this.windowSubscribed){this.prepareGate.fail('PREPARE_WINDOW_OBSERVATION_FAILED');this.prepareUpdate();return;}
  if(!this.prepareGate.begin(p013NowMs(),this.allowed()&&this.active))return;
  this.prepareLastOwner659=null;
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
  if(this.prepareGate.state!=='PREPARING'||!this.startPrepareObservation(id,generation)){
   this.prepareGate.fail('PREPARE_OBSERVER_UNAVAILABLE');this.prepareUpdate();return;
  }
  this.configuring=false;this.prepareUpdate();this.schedule();this.postPrepareFrame();
  this.prepareTimer=setTimeout(()=>{
   if(!this.prepareGate.currentId(id,generation)||this.pendingPrepareId!==id)return;
   const o=this.prepareObservation;if(o!==null)this.prepareFrameDeadline(o,p013NowMs());
  },3000);
 }

 private async reserveImpl598():Promise<void> {
  // A separate configure/close-keyboard action must have completed before reserve.
  if(this.configuring||this.exporting||this.evidence!==null){this.probeStatus='CONFIGURATION_OR_ARCHIVE_BUSY';return;}
  const roi=parseWindowRoi(this.probeRoi);
  if(roi===null){this.probeStatus='ROI_INVALID';return;}
      const reserveId=this.prepareGate.prepareId,reserveGeneration=this.prepareGate.pageGeneration,reserveBranch=this.prepareBranch,reserveWindow=this.prepareWindow,reserveWg=PrepareMainWindow.generation;
    const fresh=this.readPrepareSnapshot();if(!this.prepareScope659(reserveId,reserveGeneration,reserveBranch,reserveWindow,reserveWg)){this.prepareGate.discardOwnerCallback();return;}
    const ticket=this.prepareGate.checkForReserve(fresh,p013NowMs());if(!ticket.valid){this.probeStatus=ticket.reason;this.prepareUpdate();return;}
const r=this.singleSand.reserveCapture(roi,this.probeFixtureKey,Number(this.maxHoldText));
  this.probeStatus=JSON.stringify(r);
  if(r.state==='RESERVED'){if(!this.prepareGate.consumeTicket(ticket)){this.singleSand.capture.invalidate('PREPARE_TICKET_MISMATCH');this.probeStatus='PREPARE_TICKET_MISMATCH';this.prepareUpdate();return;}this.prepareUpdate();
   if(this.t394WaitB){this.t394WaitB=false;this.model.t394InjectB(p013NowMs());}
   this.schedule();
  }
  this.reserveIo598=1;
  const context=getContext(this) as common.UIAbilityContext;
  await new DiagnosticColdFiles(context.filesDir).save('t413-reserve.json',JSON.stringify({readSerial:++this.statusSerial,reply:r}));
  this.reserveIo598=2;
  await this.statusCold();
 }
 private async exportWindow():Promise<void> {
  this.refreshRaster();
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
  this.refreshRaster();
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
  this.refreshRaster();
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
  this.fullyVisible=full;this.prepareLayoutChanged();
  const g=this.singleSand.capture.geometry();g.visible=full&&this.active&&rasterNodeReason(g.raster).length===0;
  g.geometry=++this.t394GeometryEpoch;g.raster.geometry=g.geometry;this.layout=g;this.singleSand.captureGeometry(g);this.schedule();
 }
 private readonly raster:RasterGeometryController=new RasterGeometryController();
 @State private rasterLocalX:number=0;@State private rasterLocalY:number=0;
 @State private rasterW:number=0;@State private rasterH:number=0;@State private rasterRevision:number=0;
 private rasterLayoutSerial540:number=0;private rasterDrawSerial540:number=0;private rasterSampleSerial540:number=0;
 private rasterObserver540:UIObserver|null=null;private rasterLayoutListener540:(()=>void)|null=null;
 private rasterSubscribe540():void {
  this.rasterUnsubscribe540();const page=this.prepareGate.pageGeneration+1;
  try{const observer=this.getUIContext().getUIObserver();this.rasterObserver540=observer;
   this.rasterLayoutListener540=():void=>{if(!this.active||this.prepareGate.pageGeneration!==page)return;this.rasterLayoutSerial540++;this.refreshRaster();};
   observer.on('didLayout',this.rasterLayoutListener540);
  }catch(error){this.singleSand.capture.invalidate('RASTER_OBSERVER_UNAVAILABLE');}
 }
 private rasterUnsubscribe540():void {const o=this.rasterObserver540,listener=this.rasterLayoutListener540;this.rasterObserver540=null;this.rasterLayoutListener540=null;if(o!==null&&listener!==null){try{o.off('didLayout',listener);}catch(error){this.probeStatus='RASTER_OBSERVER_OFF_FAILED';}}}
 private rasterArea540():void {if(this.active)this.refreshRaster();} // Area carries no trusted generation or geometry.
 private rawBytes540(s:string):number {let n=0;for(let i=0;i<s.length;i++){const c=s.charCodeAt(i);if(c<128)n++;else if(c<2048)n+=2;else if(c>=0xd800&&c<=0xdbff&&i+1<s.length&&s.charCodeAt(i+1)>=0xdc00&&s.charCodeAt(i+1)<=0xdfff){n+=4;i++;}else n+=3;}return n;}
 private rawComponent540(rect:componentUtils.ComponentInfo):string {const raw=JSON.stringify(rect);if(this.rawBytes540(raw)>4096)throw new Error('INCOMPLETE_COMPONENT_RAW');return raw;}
 private ancestor540(id:string,parent:string,rect:componentUtils.ComponentInfo):RasterAncestor540 {
  const r=new RasterAncestor540();r.id=id;r.parent=parent;r.raw=JSON.stringify({l:[rect.localOffset.x,rect.localOffset.y],w:[rect.windowOffset.x,rect.windowOffset.y],s:[rect.screenOffset.x,rect.screenOffset.y],m:rect.transform});
  if(this.rawBytes540(JSON.stringify(r))>256)throw new Error('INCOMPLETE_ANCESTRY');return r;
 }
 private rasterRect(rect:componentUtils.ComponentInfo,screen:boolean=false):RasterRect {const r=new RasterRect();r.x=screen?rect.screenOffset.x:rect.windowOffset.x;r.y=screen?rect.screenOffset.y:rect.windowOffset.y;r.width=rect.size.width;r.height=rect.size.height;return r;}
 private rasterTransform(rect:componentUtils.ComponentInfo):boolean {
  if(rect.scale.x!==1||rect.scale.y!==1||rect.scale.z!==1||rect.translate.x!==0||rect.translate.y!==0||rect.translate.z!==0||rect.rotate.angle!==0||rect.transform.length!==16)return false;
  for(let i=0;i<16;i++)if(rect.transform[i]!==((i===0||i===5||i===10||i===15)?1:0))return false;return true;
 }
 private publishRaster540(input:RasterAllocation):void {
  this.rasterRevision=this.raster.plan.generation;const p=this.raster.plan,g=new CaptureGeometry();g.geometry=++this.t394GeometryEpoch;
  g.visible=this.active&&this.fullyVisible&&rasterNodeReason(p).length===0;g.configuration='T532_WINDOW_HOST_RASTER_V3';g.vp2px=input.vp2px;
  g.globalX=p.actual.x/input.vp2px;g.globalY=p.actual.y/input.vp2px;g.widthVp=p.actual.width/input.vp2px;g.heightVp=p.actual.height/input.vp2px;
  g.rasterWidth=this.rasterW;g.rasterHeight=this.rasterH;g.planRevision=this.rasterRevision;g.raster=this.raster.snapshot(g.geometry);
  this.layout=g;this.singleSand.captureGeometry(g);this.surfaceWidth=g.widthVp;this.surfaceHeight=g.heightVp;this.schedule();
 }
 private refreshRaster():boolean {
  if(this.prepareWindow===null||this.prepareWindow!==PrepareMainWindow.value)return false;
  try{
   const layoutSerial=this.rasterLayoutSerial540,page=this.prepareGate.pageGeneration,windowGeneration=PrepareMainWindow.generation;
   const context=this.getUIContext(),utils=context.getComponentUtils(),outer=utils.getRectangleById('t463-allocation'),host=utils.getRectangleById('t532-window-host'),root=utils.getRectangleById('t532-window-root');
   const props=this.prepareWindow.getWindowProperties(),avoid=this.prepareWindow.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD),input=new RasterAllocation();
   input.parent=this.rasterRect(outer);input.screen=this.rasterRect(outer,true);input.host=this.rasterRect(host);input.hostScreen=this.rasterRect(host,true);
   input.window.x=props.windowRect.left;input.window.y=props.windowRect.top;input.window.width=props.windowRect.width;input.window.height=props.windowRect.height;
   input.vp2px=context.vp2px(1);input.windowId=props.id;input.windowGeneration=windowGeneration;input.orientation=input.window.width>input.window.height?1:0;
   input.transformIdentity=this.rasterTransform(outer)&&this.rasterTransform(root)&&this.rasterTransform(host);
   input.rawAllocation=this.rawComponent540(outer);input.rawHost=this.rawComponent540(host);input.rawWindow=JSON.stringify(props);input.rawAvoid=JSON.stringify(avoid);input.keyboardHeight=this.keyboardHeight;
   if(this.rawBytes540(input.rawWindow)>4096||this.rawBytes540(input.rawAvoid)>4096)throw new Error('INCOMPLETE_WINDOW_RAW');
   input.ancestry=[this.ancestor540('t532-window-root','WINDOW',root),this.ancestor540('t532-window-host','t532-window-root',host)];input.ancestryComplete=true;
   const allocated=this.raster.allocate(input);
   if(allocated){
    // Revoke before changing the physical node request, including same inward rect.
    this.singleSand.capture.invalidate('RASTER_LAYOUT_CHANGED');this.prepareLayoutChanged();this.probeFixtureKey='';
    this.raster.requestLayout(layoutSerial);this.rasterLocalX=this.raster.plan.localPx.x;this.rasterLocalY=this.raster.plan.localPx.y;
    this.rasterW=this.raster.plan.rasterW;this.rasterH=this.raster.plan.rasterH;this.publishRaster540(input);return true;
   }
   if(layoutSerial<=this.raster.plan.requestedAtLayout)return true;
   const generation=this.raster.plan.generation;
   const actual=utils.getRectangleById('t463-xcomponent'),inner=utils.getRectangleById('t463-raster'),touch=utils.getRectangleById('t463-touch');
   if(layoutSerial!==this.rasterLayoutSerial540||page!==this.prepareGate.pageGeneration||windowGeneration!==PrepareMainWindow.generation)return true;
   const n=this.singleSand.prepareReadinessCold(),binding=new RasterNative();binding.controller=n.controller;binding.session=n.session;binding.surface=n.surface;binding.context=n.context;binding.run=n.run;binding.geometry=n.geometry;binding.packetWidth=n.packetWidth;binding.packetHeight=n.packetHeight;binding.viewportWidth=n.width;binding.viewportHeight=n.height;
   const changed=this.raster.observe(this.rasterRect(actual),this.rasterRect(actual,true),this.rasterRect(inner),this.rasterRect(touch),binding,this.rasterTransform(actual)&&this.rasterTransform(inner)&&this.rasterTransform(touch),this.rasterRect(inner,true),this.rasterRect(touch,true),generation);
   this.raster.recordRaw(++this.rasterSampleSerial540,layoutSerial,this.rawComponent540(actual),this.rawComponent540(inner),this.rawComponent540(touch));
   this.raster.plan.nodeAncestry=[...input.ancestry,this.ancestor540('t463-raster','t532-window-host',inner),this.ancestor540('t463-xcomponent','t463-raster',actual),this.ancestor540('t463-touch','t463-raster',touch)];
   if(changed||this.rasterRevision!==this.raster.plan.generation){this.singleSand.capture.invalidate('RASTER_OBSERVATION_CHANGED');this.prepareLayoutChanged();this.probeFixtureKey='';this.publishRaster540(input);}
   return changed;
  }catch(error){this.probeStatus=String(error);this.prepareLayoutChanged();this.singleSand.capture.invalidate('RASTER_OBSERVATION_UNAVAILABLE');
   const g=new CaptureGeometry();g.geometry=++this.t394GeometryEpoch;g.configuration='T532_WINDOW_HOST_RASTER_V3';g.raster.reason=String(error);this.layout=g;this.singleSand.captureGeometry(g);this.rasterRevision=0;return true;
  }
 }

 private readonly model=new P051DerivedDemoV2();private readonly renderer=new P013SceneController();private readonly callback=new DerivedFrameV2();private publication:DerivedPublishedSlotV2=this.model.createPublicationCold();private budgetFrames=0;private previous=0;private serial=0;private active=false;private queued=false;private surfaceWidth:number=320;private surfaceHeight:number=480;
 private readonly gravityConfig:P051GravityConfig=new P051GravityConfig();private readonly gravity:P051GravityInputAdapter=new P051GravityInputAdapter(this.gravityConfig);private sensorActive:boolean=false;
 private readonly accelerometer=(data:sensor.AccelerometerResponse):void=>{if(this.active)this.gravity.consumeAccelerometer(data.x,data.y,data.z,p013NowMs());};
 private updateSensor():void{if(this.active&&!this.sensorActive){try{sensor.on(sensor.SensorId.ACCELEROMETER,this.accelerometer,{interval:this.gravityConfig.sensorIntervalNs});this.sensorActive=true;}catch(e){this.status='传感器不可用';}}else if(!this.active&&this.sensorActive){try{sensor.off(sensor.SensorId.ACCELEROMETER,this.accelerometer);}catch(e){}this.sensorActive=false;this.gravity.reset();}}
 @State private status:string='等待新管线';
 aboutToAppear():void{this.callback.owner=this;this.model.budget.enabled=true;this.renderer.budget=this.model.budget;}
 onPageShow():void{this.active=true;this.rasterSubscribe540();this.prepareConnect();this.model.setActive(true);this.renderer.setActive(true);this.updateSensor();this.schedule();}
 onPageHide():void{this.rasterUnsubscribe540();this.prepareDisconnect();this.singleSand.capture.invalidate('PAGE_HIDDEN');this.singleSand.rejectT394('PAGE_HIDDEN');this.active=false;this.model.setActive(false);this.renderer.setActive(false);this.updateSensor();this.previous=0;}
 aboutToDisappear():void{this.onPageHide();this.callback.owner=null;this.renderer.release();}
 private schedule():void{if(this.active&&!this.queued){this.queued=true;this.getUIContext().postFrameCallback(this.callback);}}
 tick(ns:number):void{this.queued=false;if(!this.active||this.singleSand.capture.blocksFrames())return;const rawInterval=this.previous===0?0:ns-this.previous;const dt=this.previous===0?0:Math.min(1/30,(ns-this.previous)/1e9);this.previous=ns;
  this.model.budget.begin(++this.budgetFrames,rawInterval,dt);try{const inputScope=this.model.budget.enter(1);
  const input=this.gravity.latest(p013NowMs());if(input!==null&&!this.fixtureLocked&&this.t394Arm.length===0)this.model.applyDefinedInput(input);
  this.model.budget.leave(inputScope);
  const trace=this.singleSand.t394WantsEvidence();
  if(trace)this.model.t394StartTrace();
  const ok=this.fixtureLocked||this.t394WaitB||this.model.tick(ns,dt);
  const steps=trace?this.model.t394FinishTrace():[];
  if(!ok){
   if(trace)this.singleSand.t394Frame(new T394Input(ns,0,dt,this.publication.effectiveTiltDeg,steps,this.model.t394InputReceipts()));
   this.singleSand.rejectT394('TICK:'+this.model.status);this.model.budget.outcome(3);this.status=this.model.status;this.schedule();return;
  }
  const copyStart=trace?p013NowMs():0;
  const copy=this.model.copyPublicationInto(this.publication);
  if(trace)this.singleSand.t394Frame(new T394Input(ns,p013NowMs()-copyStart,this.fixtureLocked?0:dt,
    this.publication.effectiveTiltDeg,steps,this.model.t394InputReceipts()));
  if(copy!=='OK'){this.singleSand.rejectT394('COPY:'+copy);this.model.budget.outcome(4);this.status=copy;this.schedule();return;}
  const p=this.publication,c:P012FrameContext={physicsCommitId:p.physicsCommitId,particleLayerEnabled:true,fieldEpoch:p.stateEpoch,continuityEpoch:1,geometryEpoch:this.t394GeometryEpoch,frameSerial:++this.serial,sampleTimeMs:ns/1e6,frameDeltaMs:dt*1000,inputMode:'EXPERIENCE',sourceKind:'gesture',physicalRouteId:'dual-field-particle-v1',runIdentity:p.runId,boxX:0,boxY:0,boxW:this.surfaceWidth,boxH:this.surfaceHeight,pixelRatio:1,visualInverted:false,appearanceTier:P012AppearanceTier.OFF,renderSeed:51,configRevision:1,ablation:P012Ablation.OFF,fillBox:P012FillBox.TIGHT,canonicalEvidence:false,gravityDirX:Math.sin(p.effectiveTiltDeg*Math.PI/180),gravityDirY:Math.cos(p.effectiveTiltDeg*Math.PI/180),sandTracerEnabled:false};
  if(this.useSingleSand){
   const receipt=p.drawSingleSand(this.singleSand,c,this.getUIContext().vp2px(1));
   if(this.probeStatus==='WAITING_LOCKED_FRAME'){
    const hold=this.singleSand.t394Hold(),cut=this.singleSand.t394PhaseStatus();
    if(hold!==null)this.probeStatus='CAPTURED · '+hold.name;
    else if(cut!=='WAITING_LOCKED_FRAME')this.probeStatus=cut;
   }
   this.model.budget.outcome(receipt?.currentFrameSucceeded?0:receipt?.presentationState==='STALE'?1:2);
   this.status=receipt?.currentFrameSucceeded?'单沙双态':receipt?.presentationState==='STALE'?'上一有效帧（STALE）':'绘制故障：'+this.singleSand.failure;
   this.schedule();return;
  }
  const receipt=p.drawNewPipeline(this.renderer,c,ns,this.getUIContext().vp2px(1));this.status=receipt?.currentFrameSucceeded?'运行中':receipt?.presentationState==='STALE'?'上一有效帧（STALE）':'绘制故障：'+this.renderer.failure;
  this.schedule();
  }catch(error){this.model.budget.outcome(5);throw error as Error;}finally{this.model.budget.end();}
 }
 private touch(event:TouchEvent):void{if(this.fixtureLocked||this.t394Arm.length>0)return;if(event.type===TouchType.Up||event.type===TouchType.Cancel){this.model.stop();return;}if(event.touches.length<1||rasterNodeReason(this.layout.raster).length>0)return;const p=event.touches[0],x=p.x/this.surfaceWidth*128,y=(1-p.y/this.surfaceHeight)*256;if(event.type===TouchType.Down)this.model.start(x,y);else this.model.move(x,y);}
 build(){
  Stack({alignContent:Alignment.TopStart}){
  Column({space:0}) {
   // Geometry is fixed before reserve. Configuration editor is a separate view.
   Column({space:2}) {
    Text('单沙显式冻结截图 V2').fontSize(16).height(22)
    Text(this.prepareText+' · '+this.captureText+(this.preparePaint?' ·':'  ')).fontSize(10).height(18).maxLines(1).id('t413-status-text')
    Text(this.probeStatus).fontSize(10).height(18).maxLines(1)
    if(this.configuring){
     Row(){
      Button('标定').onClick(()=>{if(!this.allowed())return;this.t394Arm='';this.t394WaitB=false;this.t394Phase('CALIBRATION',true);this.singleSand.setT406Calibration(true);}).id('t413-calibration')
      Button('厚床').onClick(()=>this.t394Start('STATIC')).id('t413-static')
      Button('对照A').onClick(()=>this.t394Start('CONTROL')).id('t413-control')
      Button('锁定A').onClick(()=>this.t394Start('LOCK')).id('t413-lock')
      Button('注入B').onClick(()=>this.t394Next()).id('t413-next')
     }.height(30)
     Row(){Button(this.fixedKeyboardConfirmed?'已确认固定键盘':'确认全屏手机固定键盘').onClick(()=>{if(this.allowed()){this.fixedKeyboardConfirmed=!this.fixedKeyboardConfirmed;this.prepareChanged();}}).fontSize(8).width(120);Button('中性').onClick(()=>this.t394SetOptics(true));Button('纹理').onClick(()=>this.t394SetOptics(false))}.height(28)
     TextInput({controller:this.roiInput,text:this.probeRoi,placeholder:'三组 x,y,w,h'}).defaultFocus(false).onAppear(()=>this.inputCreated(0)).onEditChange((v:boolean)=>this.inputEdit(0,v)).onFocus(()=>this.inputFocus(0,true)).onBlur(()=>this.inputFocus(0,false)).onChange((v:string)=>{if(this.allowed()){this.prepareChanged();this.probeRoi=v;this.probeFixtureKey='';}}).height(30).id('t413-roi')
     TextInput({controller:this.planInput,text:this.planText,placeholder:'calibrationId|planId|三组ROI'}).defaultFocus(false).onAppear(()=>this.inputCreated(1)).onEditChange((v:boolean)=>this.inputEdit(1,v)).onFocus(()=>this.inputFocus(1,true)).onBlur(()=>this.inputFocus(1,false)).onChange((v:string)=>{if(this.allowed()){this.prepareChanged();this.planText=v;this.probeStatus=this.singleSand.setT406Plan(v);}}).height(30).id('t413-plan')
     Row(){TextInput({controller:this.limitInput,text:this.maxHoldText,placeholder:'maxHoldMs'}).defaultFocus(false).onAppear(()=>this.inputCreated(2)).onEditChange((v:boolean)=>this.inputEdit(2,v)).onFocus(()=>this.inputFocus(2,true)).onBlur(()=>this.inputFocus(2,false)).onChange((v:string)=>{if(this.allowed()){this.prepareChanged();this.maxHoldText=v;}}).width(110).height(28)
      Button('收键盘并准备').onClick(()=>this.prepare()).id('t413-prepare')}.height(30)
    }else{
     Row(){
      Text('').width(2).height(2).id('t449-action-'+this.prepareBranch)
      Button('预约').enabled(this.reserveEnabled).onClick(()=>this.reserve()).id('t413-reserve')
      Button('归档').onClick(()=>this.exportWindow()).id('t413-export')
      Button('时钟').onClick(()=>this.clockCold()).id('t413-clock')
      Button('状态').onClick(()=>this.statusCold()).id('t413-status')
     }.height(36)
     Row(){
      Button('封存').onClick(()=>this.terminalCold(true)).id('t413-seal')
      Button('取消').onClick(()=>this.cancel()).id('t413-cancel')
      Button('释放').onClick(()=>this.release()).id('t413-release')
      Button('配置').onClick(()=>{if(this.allowed()){this.prepareGate.configure();this.configuring=true;this.prepareUpdate();}else this.probeStatus='HOLD_ACTIVE';}).id('t413-configure')
     }.height(36)
     Text('HOLD_OPEN 后保持画面；截图取回后才封存').fontSize(10).height(32).maxLines(2)
     Text(JSON.stringify(this.layout)+' · '+this.rasterRevision).fontSize(8).height(46).maxLines(3)
    }
   }.height(236).width('100%').clip(true)
   Stack(){}.id('t463-allocation').width('92%').layoutWeight(1).clip(true).direction(Direction.Ltr)
    .onAreaChange((_old:Area,_area:Area)=>this.rasterArea540())
  }.width('100%').height('100%').padding({top:8,bottom:8}).id('t532-logical-layout')

  Stack({alignContent:Alignment.TopStart}){
    Stack({alignContent:Alignment.TopStart}){
     XComponent({id:'p013_lmat_surface',type:XComponentType.TEXTURE,libraryname:'p013renderer'})
      .id('t463-xcomponent').width(this.rasterW+'px').height(this.rasterH+'px')
      .onLoad(()=>{this.renderer.onLoad();this.refreshRaster();}).hitTestBehavior(HitTestMode.None)
     Stack(){}.id('t463-touch').width(this.rasterW+'px').height(this.rasterH+'px').hitTestBehavior(HitTestMode.Block).onTouch((e:TouchEvent)=>this.touch(e))
    }.id('t463-raster').width(this.rasterW+'px').height(this.rasterH+'px').clip(true).hitTestBehavior(HitTestMode.Transparent)
      .position({x:this.rasterLocalX+'px',y:this.rasterLocalY+'px'}).direction(Direction.Ltr)
      .onAreaChange((_old:Area,_area:Area)=>this.rasterArea540())
      .onVisibleAreaChange([1],(visible:boolean,ratio:number)=>this.captureVisibility(visible,ratio))

  }.id('t532-window-host').width('100%').height('100%').position({x:'0px',y:'0px'}).direction(Direction.Ltr).hitTestBehavior(HitTestMode.Transparent)
  }.id('t532-window-root').width('100%').height('100%').direction(Direction.Ltr)

 }


 private reserveIo598:number=0;private reserveSerial598:number=0;private reserveFirst598:Uint32Array=new Uint32Array(64);private reserveLast598:Uint32Array=new Uint32Array(64);
 private async reserve():Promise<void>{const id=++this.reserveSerial598;const before=this.prepareGate.state,attemptPrepareId=this.prepareGate.prepareId,attemptPageGeneration=this.prepareGate.pageGeneration,attemptBranch=this.prepareBranch;let failure='';this.reserveIo598=0;try{await this.reserveImpl598();}catch(error){failure=String(error);this.prepareRawFailure659('reserve.attempt',error,attemptPrepareId,attemptPageGeneration,attemptBranch);if(this.reserveIo598===1)this.reserveIo598=3;}finally{const core=new Uint32Array(64);core[0]=id;core[1]=attemptPrepareId;core[2]=attemptPageGeneration;core[15]=this.prepareGate.currentId(attemptPrepareId,attemptPageGeneration)?0:1;core[3]=this.prepareGate.observationSerial;core[4]=this.prepareGate.inputSerial;core[5]=this.configuring?1:0;core[6]=this.exporting?1:0;core[7]=this.evidence!==null?1:0;core[8]=this.reserveIo598;core[11]=parseWindowRoi(this.probeRoi)===null?0:1;core[12]=this.singleSand.capture.status().state==='RESERVED'?1:0;core[13]=this.prepareGate.reason==='PREPARE_PERMIT_CONSUMED'?1:0;core[14]=this.prepareGate.ownerSerial;core[9]=before==='READY'?1:0;core[10]=this.prepareGate.state==='READY'?1:0;const reason=failure||this.prepareGate.reason||this.probeStatus;for(let i=0;i<Math.min(48,reason.length);i++)core[16+i]=reason.charCodeAt(i);if(this.reserveFirst598[0]===0)this.reserveFirst598.set(core);this.reserveLast598.set(core);this.prepareUpdate();}}
 reserveAttemptWitnessV1(id:number):string{const first:number[]=[],last:number[]=[];if(id===0||id===this.reserveFirst598[0])for(let i=0;i<64;i++)first.push(this.reserveFirst598[i]);if(id===0||id===this.reserveLast598[0])for(let i=0;i<64;i++)last.push(this.reserveLast598[i]);const raw=JSON.stringify({revision:'ReserveAttemptWitnessV1',coreBytes:256,first:first,last:last});return this.rawBytes540(raw)<=4096?raw:'{"state":"INCOMPLETE_WITNESS_UTF8_LIMIT"}';}

 
 private prepareCallback659(stage:string,o:PrepareFrameObservation,action:()=>void):void {if(!this.prepareOwner(o)){o.drop(p013NowMs(),0,'STALE_CALLBACK_OWNER');this.prepareGate.discardOwnerCallback();return;}try{action();}catch(error){if(this.prepareObservation===o&&this.prepareScope659(o.prepareId,o.pageGeneration,o.branch,this.prepareObservedWindow659,o.windowGeneration))this.prepareRawFailure659(stage,error,o.prepareId,o.pageGeneration,o.branch);else this.prepareGate.discardOwnerCallback();}this.prepareUpdate();}
 private prepareOwnerValue659(s:PrepareSnapshot):PrepareAttemptOwner {
  const o=new PrepareAttemptOwner();o.prepareId=this.prepareGate.prepareId;o.pageGeneration=this.prepareGate.pageGeneration;o.branch=this.prepareBranch;o.windowId=s.windowId;o.windowGeneration=s.windowGeneration;o.controller=s.controller;o.session=s.session;o.surface=s.surface;o.context=s.context;o.run=s.run;return o;
 }
 private prepareScope659(id:number,generation:number,branch:number,w:window.Window|null,windowGeneration:number):boolean {
  return this.active&&this.prepareGate.active&&this.prepareGate.prepareId===id&&this.prepareGate.pageGeneration===generation&&this.prepareBranch===branch&&w!==null&&this.prepareWindow===w&&PrepareMainWindow.value===w&&PrepareMainWindow.generation===windowGeneration;
 }
 private prepareRawFailure659(stage:string,error:Object,id:number,generation:number,branch:number):void {
  if(error===null||error===undefined){this.prepareGate.ownerFailure(stage,-1,'',String(error),this.prepareGate.ownerSnapshot(),this.prepareLastOwner659,id,generation,branch);return;}
  const raw=error as PrepareCallError,before=this.prepareGate.ownerSnapshot();
  this.prepareGate.ownerFailure(stage,typeof raw.code==='number'?raw.code:-1,typeof raw.name==='string'?raw.name:'',typeof raw.message==='string'?raw.message:String(error),before,this.prepareLastOwner659,id,generation,branch);
 }
 private prepareLastOwner659:PrepareAttemptOwner|null=null;
 private subscribePrepareWindow659():void {
  const w=this.prepareWindow;if(w===null)return;
  if(this.avoidListener!==null)w.off('avoidAreaChange',this.avoidListener);
  if(this.windowListener!==null)w.off('windowEvent',this.windowListener);
  if(this.heightListener!==null)w.off('keyboardHeightChange',this.heightListener);
  if(this.modeListener!==null)w.off('windowStatusChange',this.modeListener);
  this.windowSubscribed=false;
  const id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration,branch=this.prepareBranch,wg=PrepareMainWindow.generation;
  const current=():boolean=>this.prepareScope659(id,generation,branch,w,wg);
  const run=(stage:string,action:()=>void):void=>{if(!current()){this.prepareGate.discardOwnerCallback();return;}try{action();}catch(error){if(current())this.prepareRawFailure659(stage,error,id,generation,branch);else this.prepareGate.discardOwnerCallback();}this.prepareUpdate();};
  this.avoidListener=(event:window.AvoidAreaOptions):void=>run('window.avoid',():void=>{if(event.type===window.AvoidAreaType.TYPE_KEYBOARD){this.prepareLayoutChanged();this.prepareGate.changed('KEYBOARD_AVOID_CHANGED');this.observePrepare();}});
  this.windowListener=(event:window.WindowEventType):void=>run('window.event',():void=>{if(event===window.WindowEventType.WINDOW_HIDDEN||event===window.WindowEventType.WINDOW_INACTIVE||event===window.WindowEventType.WINDOW_DESTROYED){this.windowShown=false;this.prepareGate.invalidate('PREPARE_WINDOW_CHANGED');}else if(event===window.WindowEventType.WINDOW_SHOWN||event===window.WindowEventType.WINDOW_ACTIVE)this.windowShown=true;this.observePrepare();});
  this.heightListener=(height:number):void=>run('window.keyboardHeight',():void=>{this.keyboardHeight=height;this.prepareLayoutChanged();this.prepareGate.changed('KEYBOARD_HEIGHT_CHANGED');this.observePrepare();});
  this.modeListener=(_mode:window.WindowStatusType):void=>run('window.mode',():void=>{this.prepareLayoutChanged();this.prepareGate.invalidate('WINDOW_CONFIGURATION_CHANGED');this.observePrepare();});
  w.on('avoidAreaChange',this.avoidListener);w.on('windowEvent',this.windowListener);w.on('keyboardHeightChange',this.heightListener);w.on('windowStatusChange',this.modeListener);this.windowSubscribed=true;
 }
}
