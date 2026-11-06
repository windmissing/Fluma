const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t351/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets'),cpp=path.join(out,'entry/src/main/cpp/p013renderer');
function patch(root,rel,from,to){
 const file=path.join(root,rel),text=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 if(text.split(from).length!==2)throw Error('T372_ANCHOR '+rel+' '+from);
 fs.writeFileSync(file,text.replace(from,to));
}
for(const file of ['window_probe.h','window_probe_napi.inc'])fs.copyFileSync(path.join(__dirname,file),path.join(cpp,file));
fs.copyFileSync(path.join(__dirname,'SingleSandWindowCold.ets'),path.join(ets,'render/SingleSandWindowCold.ets'));
patch(cpp,'single_sand_pass.h','#include "egl_context.h"','#include "egl_context.h"\n#include "window_probe.h"');
patch(cpp,'single_sand_pass.h','Draw(EglContext&,const SingleSandPacket&);','Draw(EglContext&,const SingleSandPacket&,SingleSandWindowProbe* = nullptr);');
patch(cpp,'single_sand_pass.cpp','Draw(EglContext& egl,const SingleSandPacket& p){','Draw(EglContext& egl,const SingleSandPacket& p,SingleSandWindowProbe* probe){');
patch(cpp,'single_sand_pass.cpp',' out.stage="SWAP";',' if(probe){probe->config=v;probe->Capture(targets.fbos[targets.candidate]);}\n out.stage="SWAP";');
patch(cpp,'single_sand_napi.inc','napi_value DrawSingleSand(napi_env e,napi_callback_info info){\n size_t n=4;napi_value a[4]{};', '#include "window_probe_napi.inc"\nnapi_value DrawSingleSand(napi_env e,napi_callback_info info){\n size_t n=5;napi_value a[5]{};');
patch(cpp,'single_sand_napi.inc',' if(n!=4||',' if((n!=4&&n!=5)||');
patch(cpp,'single_sand_napi.inc',' SurfaceInfo surface{};auto current=',` SingleSandWindowProbe* probe=nullptr;
 if(n==5&&!windowProbe.available){
  windowProbe.packet=p;windowProbe.target={};windowProbe.ms=0;windowProbe.version[0]=windowProbe.renderer[0]=0;
  windowProbe.count=windowProbe.used=0;windowProbe.config={};windowProbe.unbound=false;windowProbe.unbindError=0;windowProbe.drawEglError=0;
  windowProbe.dither=windowProbe.blend=false;
  windowProbe.stage="INVALID_ROI";
  size_t length=0,offset=0;napi_value buffer=nullptr;napi_typedarray_type type;void* coords=nullptr;
  if(napi_get_typedarray_info(e,a[4],&type,&length,&coords,&buffer,&offset)==napi_ok&&type==napi_int32_array&&
     length<=12&&coords&&windowProbe.Configure(static_cast<const int32_t*>(coords),int(length))){
   probe=&windowProbe;windowProbe.stage="PRE_PRESENT";
  }else{windowProbe.count=0;windowProbe.used=0;}
 }
 const auto finishProbe=[&](){
  if(n==5&&!windowProbe.available){windowReceipt=r;windowProbe.available=true;}
 };
 SurfaceInfo surface{};auto current=`);
patch(cpp,'single_sand_napi.inc','r.rawEgl=g.egl.error;return SingleResult(e,r,p);}','r.rawEgl=g.egl.error;finishProbe();return SingleResult(e,r,p);}');
patch(cpp,'single_sand_napi.inc','g.singleSand.Draw(g.egl,p)','g.singleSand.Draw(g.egl,p,probe)');
patch(cpp,'single_sand_napi.inc',' if(g.egl.Unbind()!=Status::OK){',' if(probe)probe->drawEglError=r.rawEgl;\n const auto unbind=g.egl.Unbind();\n if(probe){probe->unbound=unbind==Status::OK;probe->unbindError=probe->unbound?EGL_SUCCESS:g.egl.error;}\n if(unbind!=Status::OK){');
patch(cpp,'single_sand_napi.inc','r.rawEgl=g.egl.error;}\n return SingleResult(e,r,p);','r.rawEgl=g.egl.error;}\n finishProbe();\n return SingleResult(e,r,p);');
patch(cpp,'napi_bridge.cpp','    {"drawSingleSandScene",','    {"takeSingleSandWindowProbe",nullptr,TakeSingleSandWindowProbe,nullptr,nullptr,nullptr,napi_default,nullptr},\n    {"drawSingleSandScene",');
patch(ets,'physics/DiagnosticColdFiles.ets','  const bytes=new util.TextEncoder().encodeInto(text);const f=', `  return this.saveBytes(name,new util.TextEncoder().encodeInto(text));
 }
 async saveBytes(name:string,bytes:Uint8Array):Promise<ColdFile> {
  if(!/^[A-Za-z0-9_.-]+$/.test(name))throw new Error('COLD_FILENAME');
  const f=`);
patch(ets,'physics/DiagnosticColdFiles.ets','bytes.buffer.slice(offset)','bytes.buffer.slice(bytes.byteOffset+offset,bytes.byteOffset+bytes.byteLength)');
const types='render/P013SingleSandTypes.ets';
patch(ets,types,'export enum P013SingleSandPipeline {',"import { WindowProbeEvidence } from './SingleSandWindowCold';\nexport enum P013SingleSandPipeline {");
patch(ets,types,'export interface SingleSandNative {','export interface SingleSandNative {\n  takeSingleSandWindowProbe():WindowProbeEvidence|null;');
patch(ets,types,'meta:Uint32Array):SingleSandReceipt;','meta:Uint32Array,roi?:Int32Array):SingleSandReceipt;');
const controller='render/P013SingleSandController.ets';
patch(ets,controller,'  private tile:',`  private pendingRoi:Int32Array|null=null;
  private probePending=false;
  armWindowProbe(roi:Int32Array):boolean {
    if(this.pendingRoi!==null||this.probePending)return false;
    this.pendingRoi=roi;return true;
  }
  takeWindowProbe():WindowProbeEvidence|null {
    const evidence=this.native.takeSingleSandWindowProbe();
    this.probePending=false;
    return evidence;
  }
  setProbeNeutral(neutral:boolean):void {
    if(this.pendingRoi!==null||this.probePending)return;
    this.values[8]=neutral?0:.08;this.values[9]=neutral?0:.10;
    this.values[14]=neutral?0:.10;this.values[15]=neutral?0:.16;
    this.config.revision++;this.tile=null;this.bound=false;
  }
  private tile:`);
patch(ets,controller,'import nativeRenderer', "import { WindowProbeEvidence } from './SingleSandWindowCold';\nimport nativeRenderer");
patch(ets,controller,'    const receipt=this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta);',`    const roi=this.pendingRoi;
    const receipt=roi===null?this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta):
      this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta,roi);
    if(roi!==null){this.pendingRoi=null;this.probePending=true;}`);
const page='pages/P051DerivedV2Demo.ets';
patch(ets,page,'import { P013SingleSandController }',"import { WindowProbeEvidence,parseWindowRoi,saveWindowProbe } from '../render/SingleSandWindowCold';\nimport { P013SingleSandController }");
patch(ets,page," private readonly useSingleSand:boolean=AppStorage.get<boolean>('singleSandDemo')===true;",` private readonly useSingleSand:boolean=true;
 @State private probeRoi:string='0,0,16,16';
 @State private probeStatus:string='窗口读回未预约';
 @State private fixtureLocked:boolean=false;
 private evidence:WindowProbeEvidence|null=null;
 private exporting:boolean=false;
 private async exportWindow():Promise<void>{
  if(this.exporting)return;
  if(this.evidence===null)this.evidence=this.singleSand.takeWindowProbe();
  if(this.evidence===null){this.probeStatus='没有完成的读回';return;}
  this.exporting=true;
  try{
   const context=getContext(this) as common.UIAbilityContext;
   const manifest=await saveWindowProbe(context.filesDir,this.evidence);
   this.evidence=null;this.probeStatus=manifest;
  }catch(e){this.probeStatus='窗口读回导出失败，可重试';}
  finally{this.exporting=false;}
 }`);
patch(ets,page,'if(input!==null)this.model.applyDefinedInput(input);','if(input!==null&&!this.fixtureLocked)this.model.applyDefinedInput(input);');
patch(ets,page,'if(!this.model.tick(ns,dt)){','if(!this.fixtureLocked&&!this.model.tick(ns,dt)){');
patch(ets,page,'private touch(event:TouchEvent):void{','private touch(event:TouchEvent):void{if(this.fixtureLocked)return;');
patch(ets,page,"Button('重置').onClick(()=>{this.model.reset();","Button('重置').onClick(()=>{this.fixtureLocked=false;this.model.reset();");
patch(ets,page,"    if(this.useSingleSand){\n      Button('导出材质收据')",`    Text(this.probeStatus).fontSize(12)
    Button(this.fixtureLocked?'恢复仿真':'锁定当前样本').onClick(()=>{this.model.stop();this.fixtureLocked=!this.fixtureLocked;})
    TextInput({text:this.probeRoi,placeholder:'x,y,w,h; x,y,w,h'}).onChange((value:string)=>this.probeRoi=value)
    Row({space:4}) {
      Button('中性').onClick(()=>this.singleSand.setProbeNeutral(true))
      Button('纹理').onClick(()=>this.singleSand.setProbeNeutral(false))
      Button('预约一次').onClick(()=>{
        const roi=parseWindowRoi(this.probeRoi);
        if(roi===null){this.probeStatus='ROI 无效';return;}
        this.probeStatus=this.evidence===null&&this.singleSand.armWindowProbe(roi)?'等待下一次有效绘制':'请先导出上一笔';
      })
      Button('导出读回').onClick(()=>{this.exportWindow();})
    }
    if(this.useSingleSand){
      Button('导出材质收据')`);
const sourceSha256=Object.fromEntries(['window_probe.h','window_probe_napi.inc','SingleSandWindowCold.ets','generate.cjs'].map(file=>[file,crypto.createHash('sha256').update(fs.readFileSync(path.join(__dirname,file))).digest('hex')]));
fs.writeFileSync(path.join(out,'t372-manifest.json'),JSON.stringify({revision:'T372_SINGLE_SAND_WINDOW_PROBE_V1',isolated:true,device:'NOT_RUN',sourceSha256},null,2));
