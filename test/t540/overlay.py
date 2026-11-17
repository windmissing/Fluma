from pathlib import Path
import sys,re,shutil,json,hashlib
ROOT=Path(__file__).resolve().parents[2]
def one(s,a,b):
 if s.count(a)!=1:raise ValueError('T540_ANCHOR '+a[:100])
 return s.replace(a,b)
def overlay(out):
 profile=out/'build-profile.json5';doc=json.loads(profile.read_text(encoding='utf8'));doc['app'].pop('signingConfigs',None)
 for product in doc['app']['products']:product.pop('signingConfig',None)
 profile.write_text(json.dumps(doc,indent=2))
 ets=out/'entry/src/main/ets'
 shutil.copyfile(ROOT/'test/t540/RasterGeometry.ets',ets/'render/RasterGeometry.ets')
 p=ets/'pages/P051DerivedV2Demo.ets';s=p.read_text(encoding='utf8')
 s=s.replace('RasterGeometryController, RasterAllocation, RasterRect, RasterNative, rasterNodeReason','RasterGeometryController, RasterAllocation, RasterRect, RasterNative, RasterAncestor540, rasterNodeReason')
 start=s.index(' private readonly raster:RasterGeometryController=');end=s.index(' private readonly model=',start)
 s=s[:start]+(ROOT/'test/t540/page_raster.inc').read_text(encoding='utf8')+'\n'+s[end:]
 s=one(s,'onPageShow():void{this.active=true;', 'onPageShow():void{this.active=true;this.rasterSubscribe540();')
 s=one(s,'onPageHide():void{this.prepareDisconnect();','onPageHide():void{this.rasterUnsubscribe540();this.prepareDisconnect();')
 s=one(s,"if(this.refreshRaster()){o.drop", "if(this.refreshRaster()||this.raster.plan.didLayoutSerial<=this.raster.plan.requestedAtLayout){o.drop")
 s=one(s,'const now=p013NowMs();\n    if(!this.prepareFrameDeadline', 'const now=p013NowMs();\n    this.rasterDrawSerial540++;\n    if(!this.prepareFrameDeadline')
 s=one(s,'if(ns===0)return;\n    try{','if(ns===0)return;if(this.raster.noteDraw(this.rasterDrawSerial540,ns)){this.layout.raster=this.raster.snapshot(this.layout.geometry);this.singleSand.captureGeometry(this.layout);}\n    try{')
 s=s.replace('T509_INWARD_INTEGER_RASTER_V2','T532_WINDOW_HOST_RASTER_V3').replace('T463_FRAME_OBSERVATION_V1','T532_FRAME_OBSERVATION_V3').replace('t516-prepare-','t540-prepare-')
 # Keep the old logical layout exactly, but remove render's fractional parent.
 start=s.index("   Stack({alignContent:Alignment.TopStart}){",s.index(' build(){'))
 end=s.index("  }.width('100%').height('100%').padding",start)
 nodes=s[start:end];inner=nodes[nodes.index('    Stack({alignContent:Alignment.TopStart}){'):nodes.index("   }.id('t463-allocation')")]
 inner=inner.replace(".onAreaChange((_old:Area,_area:Area)=>this.refreshRaster())", ".onAreaChange((_old:Area,_area:Area)=>this.rasterArea540())")
 # Blank allocation remains a logical, percent-sized sibling, never host parent.
 logical="""   Stack(){}.id('t463-allocation').width('92%').layoutWeight(1).clip(true).direction(Direction.Ltr)
    .onAreaChange((_old:Area,_area:Area)=>this.rasterArea540())
"""
 s=s[:start]+logical+s[end:]
 s=one(s,' build(){\n  Column({space:0}) {', " build(){\n  Stack({alignContent:Alignment.TopStart}){\n  Column({space:0}) {")
 anchor="  }.width('100%').height('100%').padding({top:8,bottom:8})"
 host="""
  Stack({alignContent:Alignment.TopStart}){
INNER540
  }.id('t532-window-host').width('100%').height('100%').position({x:'0px',y:'0px'}).direction(Direction.Ltr).hitTestBehavior(HitTestMode.Transparent)
  }.id('t532-window-root').width('100%').height('100%').direction(Direction.Ltr)
""".replace('INNER540',inner)
 s=one(s,anchor,anchor+".id('t532-logical-layout')\n"+host)
 # Host itself never intercepts touches outside the actual clipped target.
 s=s.replace(".id('t463-raster').width(this.rasterW+'px').height(this.rasterH+'px')", ".id('t463-raster').width(this.rasterW+'px').height(this.rasterH+'px').clip(true).hitTestBehavior(HitTestMode.Transparent)")
 s=s.replace(".id('t463-touch').width(this.rasterW+'px').height(this.rasterH+'px')", ".id('t463-touch').width(this.rasterW+'px').height(this.rasterH+'px').hitTestBehavior(HitTestMode.Block)")
 p.write_text(s,encoding='utf8')
 for name in ['PrepareReadiness.ets','PrepareFrameObservation.ets','T413Capture.ets','P013SingleSandController.ets']:
  p=ets/'render'/name;s=p.read_text(encoding='utf8').replace('T509_PREPARE_V2','T532_PREPARE_V3').replace('T463_FRAME_OBSERVATION_V1','T532_FRAME_OBSERVATION_V3')
  if name=='T413Capture.ets':s=one(s,'export class CaptureGeometry {',"export class CaptureGeometry {\n rasterRevision:string='T532_WINDOW_HOST_RASTER_V3';")
  if name=='P013SingleSandController.ets':s=one(s,'if(raster.rasterWidth<1||',"if(raster.rasterRevision!=='T532_WINDOW_HOST_RASTER_V3'||raster.raster.revision!=='T532_WINDOW_HOST_RASTER_V3'||raster.rasterWidth<1||")
  p.write_text(s,encoding='utf8')
 files={str(p.relative_to(ets)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [ets/'pages/P051DerivedV2Demo.ets',*(ets/'render'/n for n in ['RasterGeometry.ets','PrepareReadiness.ets','PrepareFrameObservation.ets','T413Capture.ets','P013SingleSandController.ets'])]}
 (out/'t540-manifest.json').write_text(json.dumps({'revision':'T532_WINDOW_HOST_RASTER_V3','prepare':'T532_PREPARE_V3','base':'T516','tests':'NOT_RUN_BY_USER_DIRECTION','device':'NOT_RUN','files':files},indent=2))
if __name__=='__main__':overlay(Path(sys.argv[1]))
