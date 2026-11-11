const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t455/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets');
const local=n=>fs.readFileSync(path.join(__dirname,n),'utf8').replace(/^\uFEFF/,'').replaceAll('\r\n','\n');
function edit(name,fn){const p=path.join(ets,name);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T469_ANCHOR '+a);return s.replace(a,b);}
function span(s,a,b,body){const i=s.indexOf(a),j=s.indexOf(b,i);if(i<0||j<0)throw Error('T469_SPAN '+a);return s.slice(0,i)+body+s.slice(j);}
fs.writeFileSync(path.join(ets,'render/RasterGeometry.ets'),local('RasterGeometry.ets'));
edit('render/T413Capture.ets',s=>"import { RasterGeometryPlan } from './RasterGeometry';\n"+one(s,'export class CaptureGeometry {','export class CaptureGeometry {\n rasterWidth:number=0;rasterHeight:number=0;planRevision:number=0;raster:RasterGeometryPlan=new RasterGeometryPlan();'));
edit('render/PrepareReadiness.ets',s=>{
 s="import { RasterGeometryPlan, rasterPredicate } from './RasterGeometry';\n"+s.replaceAll('T426_PREPARE_V1','T463_PREPARE_V1');
 s=one(s,' layoutGeneration:number=0;',' layoutGeneration:number=0;\n raster:RasterGeometryPlan=new RasterGeometryPlan();');
 return one(s,'  if(!s.nativeReady||','  const rasterReason=rasterPredicate(s.raster,s.geometry,s.layoutGeneration);if(rasterReason)return rasterReason;\n  if(!s.nativeReady||');
});
edit('render/PrepareFrameObservation.ets',s=>s.replace('T449_FRAME_OBSERVATION_V2','T463_FRAME_OBSERVATION_V1'));
edit('render/P013SingleSandController.ets',s=>{
 s="import { rasterNodeReason } from './RasterGeometry';\n"+s;
 s=one(s,'controller:number;session:number;', 'packetWidth:number;packetHeight:number;controller:number;session:number;');
 s=one(s,'geometry:p.geometry,width:n.widthPx,height:n.heightPx,','geometry:p.geometry,width:n.widthPx,height:n.heightPx,packetWidth:p.width,packetHeight:p.height,');
 s=one(s,"p.width===n.widthPx&&p.height===n.heightPx&&this.lastReceipt?.presentationState==='CURRENT'", "p.width===n.widthPx&&p.height===n.heightPx&&this.lastReceipt?.presentationState==='CURRENT'&&this.lastReceipt.actualToken===p.token&&this.lastReceipt.requestedToken===p.token&&this.lastReceipt.swapSucceeded");
 s=one(s,'  captureGeometry(g:CaptureGeometry):void {this.capture.setGeometry(g);}', '  private rasterForDraw:CaptureGeometry=new CaptureGeometry();\n  captureGeometry(g:CaptureGeometry):void {this.rasterForDraw=g;this.capture.setGeometry(g);}');
 s=one(s,'    p.x=context.boxX*scale;p.y=context.boxY*scale;p.w=context.boxW*scale;p.h=context.boxH*scale;',`    const raster=this.rasterForDraw;
    if(raster.rasterWidth<1||raster.rasterHeight<1||raster.vp2px!==scale||raster.geometry!==context.geometryEpoch||
       rasterNodeReason(raster.raster).length>0){
      this.failure='RASTER_PLAN_NOT_READY';return false;
    }
    p.x=context.boxX*scale;p.y=context.boxY*scale;p.w=raster.rasterWidth;p.h=raster.rasterHeight;`);
 return one(s,'    p.session=this.session;p.surface=ready.surfaceEpoch;p.context=ready.contextEpoch;',`    if(ready.widthPx!==raster.rasterWidth||ready.heightPx!==raster.rasterHeight){this.failure='RASTER_NATIVE_SIZE_MISMATCH';return false;}
    p.session=this.session;p.surface=ready.surfaceEpoch;p.context=ready.contextEpoch;`);
});
edit('pages/P051DerivedV2Demo.ets',s=>{
 s="import componentUtils from '@ohos.arkui.componentUtils';\nimport { RasterGeometryController, RasterAllocation, RasterRect, RasterNative, rasterNodeReason } from '../render/RasterGeometry';\n"+s;
 s=span(s,' private area(area:Area):void {',' private readonly model=',local('page_raster.inc')+'\n');
 s=one(s,"const component=this.getUIContext().getComponentUtils().getRectangleById('t426-surface');","const component=this.getUIContext().getComponentUtils().getRectangleById('t463-xcomponent');");
 s=one(s,'  const s=new PrepareSnapshot(),w=this.prepareWindow,g=this.layout;','  this.refreshRaster();\n  const s=new PrepareSnapshot(),w=this.prepareWindow,g=this.layout;');
 s=one(s,'  s.layoutGeneration=this.prepareObservation?.layoutGeneration??0;','  s.layoutGeneration=this.prepareObservation?.layoutGeneration??0;s.raster=this.raster.snapshot(g.geometry);');
 s=span(s,'  s.vpBoxWindowPx.x=','  s.screen.x=',`  s.vpBoxWindowPx.x=component.windowOffset.x;s.vpBoxWindowPx.y=component.windowOffset.y;
  s.vpBoxWindowPx.width=component.size.width;s.vpBoxWindowPx.height=component.size.height;
  s.vpBoxScreenPx.x=component.screenOffset.x;s.vpBoxScreenPx.y=component.screenOffset.y;
  s.vpBoxScreenPx.width=component.size.width;s.vpBoxScreenPx.height=component.size.height;
`);
 s=one(s,'   return JSON.stringify([rect.size.width,rect.size.height,rect.windowOffset.x,rect.windowOffset.y]);',`   const actual=context.getComponentUtils().getRectangleById('t463-xcomponent');
   return JSON.stringify([rect.size.width,rect.size.height,rect.windowOffset.x,rect.windowOffset.y,
     this.rasterRevision,'t463-xcomponent',this.rasterRect(actual),this.rasterRect(actual,true)]);`);
 s=one(s,'    o.didLayout(p013NowMs(),this.prepareAnchor(context,branch));',`    if(this.refreshRaster()){o.drop(p013NowMs(),0,'RASTER_PLAN_WAIT_LAYOUT');return;}
    o.didLayout(p013NowMs(),this.prepareAnchor(context,branch));
    if(o.matchesLayout(this.prepareAnchor(context,branch)))this.raster.acknowledge(this.rasterRevision,o.layoutGeneration);`);
 s=one(s,'    const o=this.prepareObservation;\n    if(this.prepareGate.state===', '    this.refreshRaster();\n    const o=this.prepareObservation;\n    if(this.prepareGate.state===');
 s=one(s,'   this.prepareWindow=w;','   this.prepareWindow=w;this.refreshRaster();');
 for(const method of ['statusCold():Promise<void> {','exportWindow():Promise<void> {','clockCold():Promise<void> {','terminalCold(seal:boolean):Promise<void> {']){
  const anchor=' private async '+method;
  s=one(s,anchor,anchor+'\n  this.refreshRaster();');
 }
 s=one(s,'g.visible=full&&this.active;', "g.visible=full&&this.active&&rasterNodeReason(g.raster).length===0;");
 s=one(s,'g.geometry=++this.t394GeometryEpoch;this.layout=g;this.singleSand.captureGeometry(g);this.schedule();', 'g.geometry=++this.t394GeometryEpoch;g.raster.geometry=g.geometry;this.layout=g;this.singleSand.captureGeometry(g);this.schedule();');
 s=one(s,'if(event.touches.length<1)return;','if(event.touches.length<1||rasterNodeReason(this.layout.raster).length>0)return;');
 s=s.replaceAll('T449_FRAME_OBSERVATION_V2','T463_FRAME_OBSERVATION_V1').replaceAll('t455-prepare-','t469-prepare-');
 s=one(s,"Text(JSON.stringify(this.layout))","Text(JSON.stringify(this.layout)+' · '+this.rasterRevision)");
 return span(s,"   Stack(){\n    XComponent({id:'p013_lmat_surface'", "  }.width('100%').height('100%').padding",`   Stack({alignContent:Alignment.TopStart}){
    Stack({alignContent:Alignment.TopStart}){
     XComponent({id:'p013_lmat_surface',type:XComponentType.TEXTURE,libraryname:'p013renderer'})
      .id('t463-xcomponent').width(this.rasterW+'px').height(this.rasterH+'px')
      .onLoad(()=>{this.renderer.onLoad();this.refreshRaster();}).hitTestBehavior(HitTestMode.None)
     Stack(){}.id('t463-touch').width(this.rasterW+'px').height(this.rasterH+'px').onTouch((e:TouchEvent)=>this.touch(e))
    }.id('t463-raster').width(this.rasterW+'px').height(this.rasterH+'px')
      .onAreaChange((_old:Area,_area:Area)=>this.refreshRaster())
      .onVisibleAreaChange([1],(visible:boolean,ratio:number)=>this.captureVisibility(visible,ratio))
   }.id('t463-allocation').width('92%').layoutWeight(1).clip(true)
    .onAreaChange((_old:Area,_area:Area)=>this.refreshRaster())
`);
});
fs.writeFileSync(path.join(out,'t469-manifest.json'),JSON.stringify({revision:'T463_RASTER_GEOMETRY_V1',base:'T455',device:'NOT_RUN'},null,2));
