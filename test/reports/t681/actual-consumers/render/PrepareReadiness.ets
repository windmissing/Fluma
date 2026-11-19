import { RasterGeometryPlan, rasterPredicate } from './RasterGeometry';
export class PrepareRect {
 x:number=0;y:number=0;width:number=0;height:number=0;
}
export class PrepareSnapshot {
 layoutGeneration:number=0;
 raster:RasterGeometryPlan=new RasterGeometryPlan();
 windowId:number=0;windowGeneration:number=0;windowActive:boolean=false;windowRegistered:boolean=false;
 keyboardConfiguration:string='UNKNOWN';deviceType:string='';windowMode:number=0;
 windowRect:PrepareRect=new PrepareRect();avoid:PrepareRect[]=[];avoidUnit:string='WINDOW_PX';
 keyboardHeight:number=-1;keyboardAnimation:boolean=false;
 geometry:number=0;configurationRevision:number=0;globalX:number=0;globalY:number=0;
 widthVp:number=0;heightVp:number=0;vp2px:number=0;fullyVisible:boolean=false;
 vpBoxWindowPx:PrepareRect=new PrepareRect();vpBoxScreenPx:PrepareRect=new PrepareRect();
 component:PrepareRect=new PrepareRect();screen:PrepareRect=new PrepareRect();componentPresent:boolean=false;
 transformIdentity:boolean=false;orientation:number=0;
 controller:number=0;session:number=0;surface:number=0;context:number=0;nativeGeometry:number=0;
 viewportWidth:number=0;viewportHeight:number=0;nativeReady:boolean=false;run:string='';opticsRevision:number=0;
}
export class PreparePermit {
 revision:string='T532_PREPARE_V3';prepareId:number=0;pageGeneration:number=0;observationSerial:number=0;
 inputSerial:number=0;firstFrame:number=0;secondFrame:number=0;readyMonoMs:number=0;snapshot:PrepareSnapshot=new PrepareSnapshot();
}
export class PrepareApiRecord {
 api:string='';startMonoMs:number=0;endMonoMs:number=0;prepareId:number=0;
 code:number=0;name:string='';message:string='';runStatus:string='';
}
export class PrepareInput {
 created:boolean=false;editing:boolean=false;focused:boolean=false;everEditing:boolean=false;
}
// Shared by the actual generated page and host page harness.
export class PrepareReadiness {
 private static nextPrepareId:number=1;
 readonly session:string='s'+Date.now()+'-'+PrepareReadiness.nextPrepareId;
 readonly revision:string='T532_PREPARE_V3';
 state:string='CONFIGURING';reason:string='';active:boolean=false;pageGeneration:number=0;prepareId:number=0;
 configurationRevision:number=0;observationSerial:number=0;inputSerial:number=0;
 readonly inputs:PrepareInput[]=[new PrepareInput(),new PrepareInput(),new PrepareInput()];
 readonly api:PrepareApiRecord[]=[];readonly events:string[]=[];private diagnosticOverflow:boolean=false;
 private started:number=0;private previous:string='';private firstFrame:number=0;private lastFrame:number=0;
 private current:PrepareSnapshot=new PrepareSnapshot();private keyboardAreaSeen:boolean=false;
 private sessionEverEdited:boolean=false;private permit:PreparePermit|null=null;
 private readonly invalidateHold:(reason:string)=>void;
 private trace(kind:string,value:string):void {
  if(this.events.length>=256){this.diagnosticOverflow=true;return;}
  this.events.push(JSON.stringify({kind:kind,value:value,prepareId:this.prepareId,pageGeneration:this.pageGeneration,serial:this.observationSerial}));
 }
 constructor(invalidate:(reason:string)=>void){this.invalidateHold=invalidate;}
 show():void {
  this.pageGeneration++;this.active=true;this.state='CONFIGURING';this.reason='';this.permit=null;
  this.previous='';this.firstFrame=this.lastFrame=0;this.api.length=0;this.trace('SHOW','');
 }
 hide():void {this.invalidate('PAGE_HIDDEN');this.active=false;this.pageGeneration++;this.trace('HIDE','');}
 currentId(id:number,generation:number):boolean {return this.active&&this.prepareId===id&&this.pageGeneration===generation;}
 invalidate(reason:string):void {
  this.permit=null;this.previous='';this.firstFrame=0;
  if(this.state==='READY'||this.state==='PREPARING'){this.state='INVALIDATED';this.reason=reason;}
  this.invalidateHold(reason);
 }
 changed(reason:string):void {
  this.observationSerial++;this.previous='';this.firstFrame=0;this.trace('CHANGE',reason);
  if(this.state==='READY')this.invalidate(reason);else this.invalidateHold(reason);
 }
 configure():void {this.invalidate('BACK_TO_CONFIGURATION');this.configurationRevision++;this.state='CONFIGURING';}
 inputCreated(index:number):void {
  if(this.inputs[index].created)return;
  this.inputs[index].created=true;this.inputSerial++;this.changed('INPUT_CREATED');
 }
 inputEvent(index:number,editing:boolean|null,focused:boolean|null):void {
  const input=this.inputs[index];
  if(!input.created){this.fail('INPUT_HISTORY_UNKNOWN');return;}
  if(editing!==null)input.editing=editing;if(focused!==null)input.focused=focused;
  if(input.editing||input.focused){input.everEditing=true;this.sessionEverEdited=true;}
  this.inputSerial++;this.trace('INPUT',JSON.stringify({index:index,editing:editing,focused:focused}));this.changed('INPUT_SESSION_CHANGED');
 }
 inputChanged():void {this.configurationRevision++;this.changed('INPUT_CONFIGURATION_CHANGED');}
 begin(now:number,allowed:boolean):boolean {
  if(!this.active||!allowed||this.state==='PREPARING')return false;
  if(PrepareReadiness.nextPrepareId>=Number.MAX_SAFE_INTEGER){this.fail('PREPARE_ID_EXHAUSTED');return false;}
  this.events.length=0;this.diagnosticOverflow=false;this.prepareId=PrepareReadiness.nextPrepareId++;this.started=now;this.state='PREPARING';this.reason='WAIT_OBSERVATIONS';
  this.permit=null;this.previous='';this.firstFrame=this.lastFrame=0;this.api.length=0;this.trace('BEGIN',String(now));return true;
 }
 fail(reason:string):void {
  if(this.state==='FAILED')return;
  this.trace('FAILED',reason);this.permit=null;this.state='FAILED';this.reason=reason;this.previous='';this.invalidateHold(reason);
 }
 startedMonoMs():number{return this.started;}
 elapsed(now:number):boolean {
  if(this.state!=='PREPARING')return true;
  if(!Number.isFinite(now)||now<this.started){this.fail('PREPARE_CLOCK_INVALID');return false;}
  if(now-this.started>=3000){this.fail('PREPARE_TIMEOUT:'+this.reason);return false;}
  return true;
 }
 record(record:PrepareApiRecord,id:number,generation:number):void {
  if(!this.currentId(id,generation))return;
  this.api.push(record);
  if(record.code!==0||record.name||record.message)this.fail('COMPONENT_API_FAILED:'+record.api);
 }
 observe(s:PrepareSnapshot):void {
  if(!this.active)return;
  const text=JSON.stringify(s);
  if(text!==JSON.stringify(this.current))this.changed('OBSERVATION_CHANGED');
  const prior=this.current;
  if(prior.windowId!==s.windowId||prior.windowGeneration!==s.windowGeneration){
   this.keyboardAreaSeen=false;
   this.sessionEverEdited=this.inputs.some((i:PrepareInput)=>i.editing||i.focused);
  }
  if(prior.windowId>0&&(s.windowId!==prior.windowId||s.windowGeneration!==prior.windowGeneration||
     (prior.nativeReady&&(s.controller!==prior.controller||s.session!==prior.session||s.surface!==prior.surface||s.context!==prior.context||s.run!==prior.run))))
    this.invalidate('PREPARE_STALE');
  if(text!==JSON.stringify(prior))this.trace('SNAPSHOT',text);
  this.current=JSON.parse(text) as PrepareSnapshot;
  if(s.avoid.some((r:PrepareRect)=>r.width>0&&r.height>0))this.keyboardAreaSeen=true;
 }
 private predicate(s:PrepareSnapshot):string {
  if(this.diagnosticOverflow)return 'PREPARE_DIAGNOSTIC_OVERFLOW';
  if(!this.active||!s.windowActive||!s.windowRegistered||s.windowId<=0)return 'PREPARE_WINDOW_UNKNOWN';
  if(this.inputs.some((i:PrepareInput)=>!i.created))return 'INPUT_HISTORY_UNKNOWN';
  if(this.inputs.some((i:PrepareInput)=>i.editing||i.focused))return 'INPUT_EDITING_OR_FOCUSED';
  if(s.keyboardConfiguration!=='TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD'||s.deviceType!=='phone'||s.windowMode!==1||
     s.avoidUnit!=='WINDOW_PX'||s.avoid.length!==4||!s.avoid.every((r:PrepareRect)=>[r.x,r.y,r.width,r.height].every((n:number)=>Number.isFinite(n))&&r.width>=0&&r.height>=0)||(this.sessionEverEdited&&!this.keyboardAreaSeen))return 'KEYBOARD_VISIBILITY_UNKNOWN';
  if(s.keyboardAnimation||s.keyboardHeight>0||s.avoid.some((r:PrepareRect)=>r.width>0&&r.height>0))return 'KEYBOARD_OCCLUDED';
  if(!s.fullyVisible||!s.componentPresent||!s.transformIdentity)return 'COMPONENT_VISIBILITY_UNKNOWN';
  const r=s.component,w=s.windowRect;
  if(![r.x,r.y,r.width,r.height,w.x,w.y,w.width,w.height,s.globalX,s.globalY,s.widthVp,s.heightVp,s.vp2px].every((n:number)=>Number.isFinite(n))||
     r.width<=0||r.height<=0||s.vp2px<=0||r.x<0||r.y<0||r.x+r.width>w.width||r.y+r.height>w.height)return 'COMPONENT_OUTSIDE_WINDOW';
  const converted=s.vpBoxWindowPx,screen=s.screen;
  if(![converted.x,converted.y,converted.width,converted.height,screen.x,screen.y,screen.width,screen.height].every((n:number)=>Number.isFinite(n))||
     converted.width<=0||converted.height<=0||converted.x<0||converted.y<0||converted.x+converted.width>w.width||converted.y+converted.height>w.height||
     screen.x<w.x||screen.y<w.y||screen.x+screen.width>w.x+w.width||screen.y+screen.height>w.y+w.height)return 'COMPONENT_OUTSIDE_WINDOW';
  const rasterReason=rasterPredicate(s.raster,s.geometry,s.layoutGeneration);if(rasterReason)return rasterReason;
  if(!s.nativeReady||!s.session||!s.controller||!s.surface||!s.context||!s.run||s.nativeGeometry!==s.geometry||
     s.viewportWidth!==r.width||s.viewportHeight!==r.height)return 'PREPARE_NATIVE_NOT_READY';
  return '';
 }
 frame(frame:number,now:number):void {
  if(!this.active||!this.elapsed(now))return;
  const reason=this.predicate(this.current);
  if(this.state==='READY'){if(reason)this.invalidate(reason);return;}
  if(this.state!=='PREPARING')return;
  this.trace('FRAME',JSON.stringify({frame:frame,now:now,reason:reason}));this.reason=reason;
  if(reason){this.previous='';this.firstFrame=0;return;}
  const text=JSON.stringify(this.current)+'|'+this.observationSerial+'|'+this.inputSerial+'|'+this.configurationRevision;
  if(text!==this.previous){this.previous=text;this.firstFrame=frame;this.lastFrame=frame;return;}
  if(frame<=this.lastFrame)return;this.lastFrame=frame;
  const p=new PreparePermit();p.prepareId=this.prepareId;p.pageGeneration=this.pageGeneration;
  p.observationSerial=this.observationSerial;p.inputSerial=this.inputSerial;p.firstFrame=this.firstFrame;p.secondFrame=frame;
  p.readyMonoMs=now;p.snapshot=JSON.parse(JSON.stringify(this.current)) as PrepareSnapshot;
  this.permit=p;this.state='READY';this.reason='';this.trace('READY',JSON.stringify(p));
 }
 check(s:PrepareSnapshot,now:number):string {
  this.observe(s);this.elapsed(now);
  if(this.state!=='READY'||this.permit===null)return this.reason||'PREPARE_NOT_READY';
  const reason=this.predicate(s),p=this.permit;
  if(reason||p.pageGeneration!==this.pageGeneration||p.prepareId!==this.prepareId||p.observationSerial!==this.observationSerial||
     p.inputSerial!==this.inputSerial||JSON.stringify(p.snapshot)!==JSON.stringify(s)){
   this.invalidate(reason||'PREPARE_STALE');return reason||'PREPARE_STALE';
  }
  return '';
 }
 consume():void {this.permit=null;this.state='INVALIDATED';this.reason='PREPARE_PERMIT_CONSUMED';}
 report():string {
  return JSON.stringify({revision:this.revision,prepareId:this.prepareId,pageGeneration:this.pageGeneration,state:this.state,
   session:this.session,readinessReason:this.reason,observationSerial:this.observationSerial,inputSerial:this.inputSerial,startedMonoMs:this.started,
   inputs:this.inputs,keyboardAreaSeen:this.keyboardAreaSeen,sessionEverEdited:this.sessionEverEdited,snapshot:this.current,permit:this.permit,api:this.api,events:this.events,diagnosticOverflow:this.diagnosticOverflow});
 }
}
