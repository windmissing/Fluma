const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t469/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets');
function edit(name,fn){const p=path.join(ets,name);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T516_ANCHOR '+a);return s.replace(a,b);}
fs.copyFileSync(path.join(__dirname,'RasterGeometry.ets'),path.join(ets,'render/RasterGeometry.ets'));
edit('render/PrepareReadiness.ets',s=>s.replaceAll('T463_PREPARE_V1','T509_PREPARE_V2'));
edit('pages/P051DerivedV2Demo.ets',s=>{
 s=one(s,'@State private rasterW:number=0;', '@State private rasterLocalX:number=0;@State private rasterLocalY:number=0;\n @State private rasterW:number=0;');
 s=one(s,'   let changed=this.raster.allocate(input);','   let changed=this.raster.allocate(input);\n   const observationGeneration=this.raster.plan.generation;\n   this.rasterLocalX=this.raster.plan.localPx.x;this.rasterLocalY=this.raster.plan.localPx.y;');
 s=one(s,'binding,transforms)||changed;', 'binding,transforms,this.rasterRect(inner,true),this.rasterRect(touch,true),observationGeneration)||changed;');
 s=one(s,".id('t463-raster').width(this.rasterW+'px').height(this.rasterH+'px')", ".id('t463-raster').width(this.rasterW+'px').height(this.rasterH+'px')\n      .position({x:this.rasterLocalX+'px',y:this.rasterLocalY+'px'}).direction(Direction.Ltr)");
 s=one(s,".id('t463-allocation').width('92%').layoutWeight(1).clip(true)",".id('t463-allocation').width('92%').layoutWeight(1).clip(true).direction(Direction.Ltr)");
 return s.replaceAll('FIXED_PANEL_INTEGER_RASTER_V1','T509_INWARD_INTEGER_RASTER_V2').replaceAll('t469-prepare-','t516-prepare-');
});
edit('render/P013SingleSandController.ets',s=>one(s,'p.x=context.boxX*scale;p.y=context.boxY*scale;p.w=raster.rasterWidth;p.h=raster.rasterHeight;', 'p.x=raster.raster.actual.x;p.y=raster.raster.actual.y;p.w=raster.rasterWidth;p.h=raster.rasterHeight;'));
const files={};for(const name of ['render/RasterGeometry.ets','render/PrepareReadiness.ets','render/T413Capture.ets','render/P013SingleSandController.ets','pages/P051DerivedV2Demo.ets'])files[name]=crypto.createHash('sha256').update(fs.readFileSync(path.join(ets,name))).digest('hex');
fs.writeFileSync(path.join(out,'t516-manifest.json'),JSON.stringify({revision:'T509_RASTER_GEOMETRY_V2',prepare:'T509_PREPARE_V2',base:'T469',files,device:'NOT_RUN'},null,2));
