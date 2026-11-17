export class RasterRect {x:number=0;y:number=0;width:number=0;height:number=0;}
export class RasterAncestor540 {id:string='';parent:string='';raw:string='';}
export class RasterAllocation {
 host:RasterRect=new RasterRect();hostScreen:RasterRect=new RasterRect();
 rawAllocation:string='';rawHost:string='';rawWindow:string='';rawAvoid:string='';keyboardHeight:number=-1;
 ancestry:RasterAncestor540[]=[];ancestryComplete:boolean=false;
 parent:RasterRect=new RasterRect();screen:RasterRect=new RasterRect();window:RasterRect=new RasterRect();
 vp2px:number=0;windowId:number=0;windowGeneration:number=0;orientation:number=0;transformIdentity:boolean=false;
}
export class RasterNative {
 controller:number=0;session:number=0;surface:number=0;context:number=0;run:string='';geometry:number=0;
 packetWidth:number=0;packetHeight:number=0;viewportWidth:number=0;viewportHeight:number=0;
}
export class RasterGeometryPlan {
 revision:string='T532_WINDOW_HOST_RASTER_V3';generation:number=0;geometry:number=0;
 hostNode:string='t532-window-host';hostOriginWindowPx:RasterRect=new RasterRect();hostOriginScreenPx:RasterRect=new RasterRect();requestedLocalOnHost:RasterRect=new RasterRect();
 firstDrawSerial:number=0;frameAnchorNs:string='NOT_OBSERVED';observationSerial:number=0;didLayoutSerial:number=0;requestedAtLayout:number=0;
 nodeAncestry:RasterAncestor540[]=[];rawRender:string='';rawInner:string='';rawTouch:string='';
 renderNode:string='t463-xcomponent';innerNode:string='t463-raster';touchNode:string='t463-touch';
 allocation:RasterAllocation=new RasterAllocation();rasterW:number=0;rasterH:number=0;
 desiredWindowRect:RasterRect=new RasterRect();desiredScreenRect:RasterRect=new RasterRect();localPx:RasterRect=new RasterRect();
 actual:RasterRect=new RasterRect();screen:RasterRect=new RasterRect();inner:RasterRect=new RasterRect();touch:RasterRect=new RasterRect();
 innerScreen:RasterRect=new RasterRect();touchScreen:RasterRect=new RasterRect();
 native:RasterNative=new RasterNative();transformIdentity:boolean=false;valid:boolean=false;reason:string='RASTER_NOT_ALLOCATED';layoutAcknowledged:number=0;
}
export class RasterGeometryController {
 readonly plan:RasterGeometryPlan=new RasterGeometryPlan();private allocationKey:string='';private actualKey:string='';
 private advance():void{if(this.plan.generation>=Number.MAX_SAFE_INTEGER){this.plan.valid=false;this.plan.reason='RASTER_GENERATION_EXHAUSTED';return;}this.plan.generation++;this.plan.layoutAcknowledged=0;this.plan.observationSerial=0;this.plan.didLayoutSerial=0;this.plan.firstDrawSerial=0;this.plan.frameAnchorNs='NOT_OBSERVED';}
 allocate(input:RasterAllocation):boolean {
  const key=JSON.stringify(input);if(key===this.allocationKey)return false;this.allocationKey=key;this.advance();
  this.plan.allocation=JSON.parse(key) as RasterAllocation;
  const r=input.parent;
  if(this.plan.reason==='RASTER_GENERATION_EXHAUSTED'){this.plan.rasterW=this.plan.rasterH=0;return true;}
  if(![r.x,r.y,r.width,r.height,input.screen.x,input.screen.y,input.window.x,input.window.y,input.window.width,input.window.height,input.vp2px].every((n:number)=>Number.isFinite(n))||
     r.width<=0||r.height<=0||r.width>2147483647||r.height>2147483647||input.vp2px<=0||input.windowId<=0||input.windowGeneration<=0||!input.transformIdentity){
   this.plan.valid=false;this.plan.reason='RASTER_ALLOCATION_INVALID';this.plan.rasterW=this.plan.rasterH=0;return true;
  }
  const right=r.x+r.width,bottom=r.y+r.height;
  const left=Math.ceil(r.x),top=Math.ceil(r.y),lastX=Math.floor(right),lastY=Math.floor(bottom);
  if(![left,top,lastX,lastY].every((n:number)=>Number.isSafeInteger(n))||r.x<0||r.y<0||right>input.window.width||bottom>input.window.height||
     input.screen.x!==input.window.x+r.x||input.screen.y!==input.window.y+r.y){
   this.plan.valid=false;this.plan.reason='RASTER_ALLOCATION_INVALID';this.plan.rasterW=this.plan.rasterH=0;return true;
  }
  const w=lastX-left,h=lastY-top;
  const hostReason=integerHostReason(input,left,top,w,h);
  if(hostReason){this.plan.valid=false;this.plan.reason=hostReason;this.plan.rasterW=this.plan.rasterH=0;return true;}
  this.plan.hostOriginWindowPx=input.host;this.plan.hostOriginScreenPx=input.hostScreen;
  this.plan.desiredWindowRect={x:left,y:top,width:w,height:h};
  this.plan.desiredScreenRect={x:input.window.x+left,y:input.window.y+top,width:w,height:h};
  this.plan.localPx={x:left-input.host.x,y:top-input.host.y,width:w,height:h};this.plan.requestedLocalOnHost=this.plan.localPx;
  if(w<1||h<1){this.plan.valid=false;this.plan.reason='RASTER_ALLOCATION_EMPTY';this.plan.rasterW=this.plan.rasterH=0;return true;}
  this.plan.rasterW=w;this.plan.rasterH=h;this.plan.valid=true;this.plan.reason='';return true;
 }
 observe(actual:RasterRect,screen:RasterRect,inner:RasterRect,touch:RasterRect,native:RasterNative,transformIdentity:boolean,innerScreen:RasterRect,touchScreen:RasterRect,generation:number):boolean {
  if(generation!==this.plan.generation)return false;
  // Packet geometry follows our own geometry; it is evidence, never a new
  // allocation input. Excluding it from this key prevents a feedback loop.
  const key=JSON.stringify([actual,screen,inner,touch,innerScreen,touchScreen,transformIdentity,native.controller,native.session,native.surface,native.context,native.run,native.packetWidth,native.packetHeight,native.viewportWidth,native.viewportHeight]);
  const changed=key!==this.actualKey;this.actualKey=key;if(changed)this.advance();
  this.plan.actual=JSON.parse(JSON.stringify(actual)) as RasterRect;this.plan.screen=JSON.parse(JSON.stringify(screen)) as RasterRect;
  this.plan.inner=JSON.parse(JSON.stringify(inner)) as RasterRect;this.plan.touch=JSON.parse(JSON.stringify(touch)) as RasterRect;this.plan.transformIdentity=transformIdentity;this.plan.native=JSON.parse(JSON.stringify(native)) as RasterNative;
  this.plan.innerScreen=JSON.parse(JSON.stringify(innerScreen)) as RasterRect;this.plan.touchScreen=JSON.parse(JSON.stringify(touchScreen)) as RasterRect;
  return changed;
 }
 acknowledge(revision:number,layout:number):boolean {if(revision!==this.plan.generation||layout<1||!this.plan.valid)return false;this.plan.layoutAcknowledged=layout;return true;}
 recordRaw(serial:number,didLayout:number,render:string,inner:string,touch:string):void {
  if(this.plan.observationSerial===0||this.plan.didLayoutSerial===0){this.plan.observationSerial=serial;this.plan.didLayoutSerial=didLayout;}
  this.plan.rawRender=render;this.plan.rawInner=inner;this.plan.rawTouch=touch;
 }
 noteDraw(serial:number,ns:number):boolean {if(this.plan.firstDrawSerial===0&&Number.isSafeInteger(ns)&&ns>0){this.plan.firstDrawSerial=serial;this.plan.frameAnchorNs=String(ns);return true;}return false;}
 requestLayout(serial:number):void {this.plan.requestedAtLayout=serial;this.plan.didLayoutSerial=0;this.plan.observationSerial=0;}
 snapshot(geometry:number):RasterGeometryPlan {const copy=JSON.parse(JSON.stringify(this.plan)) as RasterGeometryPlan;copy.geometry=geometry;return copy;}
}
export function rasterPredicate(p:RasterGeometryPlan,geometry:number,layout:number):string {
 if(p.revision!=='T532_WINDOW_HOST_RASTER_V3'||!p.valid||p.generation<1)return p.reason||'RASTER_PLAN_INVALID';
 if(p.didLayoutSerial<=p.requestedAtLayout)return 'RASTER_WAIT_NEW_DID_LAYOUT';
 if(p.firstDrawSerial<1||p.frameAnchorNs==='NOT_OBSERVED')return 'RASTER_WAIT_DRAW_PAIR';
 if(!p.layoutAcknowledged||p.layoutAcknowledged!==layout)return 'RASTER_WAIT_NEW_LAYOUT';
 if(p.geometry!==geometry||p.renderNode!=='t463-xcomponent')return 'RASTER_STALE_PLAN';
 const reason=rasterNodeReason(p);if(reason)return reason;
 const n=p.native;
 if(!n.controller||!n.session||!n.surface||!n.context||!n.run||n.geometry!==geometry||n.packetWidth!==p.rasterW||n.packetHeight!==p.rasterH||n.viewportWidth!==p.rasterW||n.viewportHeight!==p.rasterH)return 'RASTER_NATIVE_MISMATCH';
 return '';
}
export function rasterNodeReason(p:RasterGeometryPlan):string {
 if(!p.valid)return p.reason||'RASTER_PLAN_INVALID';
 if(p.revision!=='T532_WINDOW_HOST_RASTER_V3')return 'RASTER_REVISION_MISMATCH';
 if(p.didLayoutSerial<=p.requestedAtLayout)return 'RASTER_WAIT_NEW_DID_LAYOUT';
 if(!p.transformIdentity)return 'RASTER_TRANSFORM_UNSUPPORTED';
 const r=p.actual,parent=p.allocation.parent,w=p.allocation.window;
 if(!Number.isFinite(r.x)||!Number.isFinite(r.y)||!Number.isFinite(p.screen.x)||!Number.isFinite(p.screen.y)||r.width!==p.rasterW||r.height!==p.rasterH)return 'RASTER_COMPONENT_SIZE';
 if(parent.x<0||parent.y<0||parent.x+parent.width>w.width||parent.y+parent.height>w.height||r.x<parent.x||r.y<parent.y||r.x+r.width>parent.x+parent.width||r.y+r.height>parent.y+parent.height)return 'RASTER_OUTSIDE_ALLOCATION';
 if(!sameRasterRect(r,p.desiredWindowRect)||!sameRasterRect(p.screen,p.desiredScreenRect))return 'RASTER_PLACEMENT_MISMATCH';
 if(p.screen.x!==r.x+w.x||p.screen.y!==r.y+w.y)return 'RASTER_POSITION_DOMAIN';
 if(!sameRasterRect(p.inner,r)||!sameRasterRect(p.touch,r)||!sameRasterRect(p.innerScreen,p.desiredScreenRect)||!sameRasterRect(p.touchScreen,p.desiredScreenRect))return 'RASTER_PLACEMENT_MISMATCH';
 return '';
}
function sameRasterRect(a:RasterRect,b:RasterRect):boolean {
 return a.x===b.x&&a.y===b.y&&a.width===b.width&&a.height===b.height;
}

export function integerHostReason(input:RasterAllocation,x:number,y:number,w:number,h:number):string {
 const host=input.host,screen=input.hostScreen;
 if(!input.ancestryComplete||input.ancestry.length<2||input.ancestry.length>16)return 'INCOMPLETE_ANCESTRY';
 for(const item of input.ancestry)if(item.raw.length>256)return 'INCOMPLETE_ANCESTRY';
 if(![host.x,host.y,screen.x,screen.y].every((n:number)=>Number.isSafeInteger(n)))return 'RASTER_HOST_NONINTEGRAL';
 if(![host.width,host.height,screen.width,screen.height,x,y,w,h].every((n:number)=>Number.isFinite(n))||host.width<=0||host.height<=0||host.width!==screen.width||host.height!==screen.height||
    screen.x!==input.window.x+host.x||screen.y!==input.window.y+host.y||host.x<0||host.y<0||host.x+host.width>input.window.width||host.y+host.height>input.window.height||
    x<host.x||y<host.y||x+w>host.x+host.width||y+h>host.y+host.height)return 'RASTER_HOST_DOMAIN_INVALID';
 if(!Number.isSafeInteger(x-host.x)||!Number.isSafeInteger(y-host.y))return 'RASTER_HOST_NONINTEGRAL';
 return '';
}
