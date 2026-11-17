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
