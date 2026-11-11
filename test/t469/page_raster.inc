 private readonly raster:RasterGeometryController=new RasterGeometryController();
 @State private rasterW:number=0;@State private rasterH:number=0;@State private rasterRevision:number=0;
 private rasterRect(rect:componentUtils.ComponentInfo,screen:boolean=false):RasterRect {
  const r=new RasterRect();r.x=screen?rect.screenOffset.x:rect.windowOffset.x;r.y=screen?rect.screenOffset.y:rect.windowOffset.y;r.width=rect.size.width;r.height=rect.size.height;return r;
 }
 private rasterTransform(rect:componentUtils.ComponentInfo):boolean {
  return rect.scale.x===1&&rect.scale.y===1&&rect.scale.z===1&&rect.translate.x===0&&rect.translate.y===0&&rect.translate.z===0&&rect.rotate.angle===0;
 }
 private refreshRaster():boolean {
  if(this.prepareWindow===null||this.prepareWindow!==PrepareMainWindow.value)return false;
  try{
   const context=this.getUIContext(),utils=context.getComponentUtils(),outer=utils.getRectangleById('t463-allocation');
   const props=this.prepareWindow.getWindowProperties(),input=new RasterAllocation();
   input.parent=this.rasterRect(outer);input.screen=this.rasterRect(outer,true);input.window.x=props.windowRect.left;input.window.y=props.windowRect.top;input.window.width=props.windowRect.width;input.window.height=props.windowRect.height;
   input.vp2px=context.vp2px(1);input.windowId=props.id;input.windowGeneration=PrepareMainWindow.generation;input.orientation=input.window.width>input.window.height?1:0;input.transformIdentity=this.rasterTransform(outer);
   let changed=this.raster.allocate(input);
   // Publish allocation even while the inner nodes are awaiting layout.
   this.rasterW=this.raster.plan.rasterW;this.rasterH=this.raster.plan.rasterH;
   const actual=utils.getRectangleById('t463-xcomponent'),inner=utils.getRectangleById('t463-raster'),touch=utils.getRectangleById('t463-touch');
   const n=this.singleSand.prepareReadinessCold(),binding=new RasterNative();
   binding.controller=n.controller;binding.session=n.session;binding.surface=n.surface;binding.context=n.context;binding.run=n.run;binding.geometry=n.geometry;binding.packetWidth=n.packetWidth;binding.packetHeight=n.packetHeight;binding.viewportWidth=n.width;binding.viewportHeight=n.height;
   const transforms=this.rasterTransform(actual)&&this.rasterTransform(inner)&&this.rasterTransform(touch);
   changed=this.raster.observe(this.rasterRect(actual),this.rasterRect(actual,true),this.rasterRect(inner),this.rasterRect(touch),binding,transforms)||changed;
   changed=changed||this.rasterRevision!==this.raster.plan.generation;
   if(changed){
    this.rasterW=this.raster.plan.rasterW;this.rasterH=this.raster.plan.rasterH;this.rasterRevision=this.raster.plan.generation;
    this.prepareLayoutChanged();this.probeFixtureKey='';
    const g=new CaptureGeometry();g.geometry=++this.t394GeometryEpoch;g.visible=this.active&&this.fullyVisible&&rasterNodeReason(this.raster.plan).length===0;g.configuration='FIXED_PANEL_INTEGER_RASTER_V1';
    g.vp2px=input.vp2px;g.globalX=actual.windowOffset.x/input.vp2px;g.globalY=actual.windowOffset.y/input.vp2px;g.widthVp=actual.size.width/input.vp2px;g.heightVp=actual.size.height/input.vp2px;
    g.rasterWidth=this.rasterW;g.rasterHeight=this.rasterH;g.planRevision=this.rasterRevision;g.raster=this.raster.snapshot(g.geometry);
    this.layout=g;this.singleSand.captureGeometry(g);this.surfaceWidth=g.widthVp;this.surfaceHeight=g.heightVp;this.schedule();
   }
   return changed;
  }catch(e){this.probeStatus='RASTER_OBSERVATION_UNAVAILABLE';this.prepareLayoutChanged();this.singleSand.capture.invalidate('RASTER_OBSERVATION_UNAVAILABLE');
   if(this.layout.raster.valid){const g=new CaptureGeometry();g.geometry=++this.t394GeometryEpoch;this.layout=g;this.singleSand.captureGeometry(g);this.rasterRevision=0;}
   return true;}
 }
