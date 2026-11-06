'use strict';
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t372/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets'),cpp=path.join(out,'entry/src/main/cpp/p013renderer');
function edit(root,rel,from,to){
 const file=path.join(root,rel),text=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 if(text.split(from).length!==2)throw Error('T385_ANCHOR '+rel+' '+from.slice(0,120));
 fs.writeFileSync(file,text.replace(from,to));
}
function span(root,rel,first,last,to){
 const s=fs.readFileSync(path.join(root,rel),'utf8').replaceAll('\r\n','\n');
 const i=s.indexOf(first),j=s.indexOf(last,i+first.length);
 if(i<0||j<0)throw Error('T385_SPAN '+rel+' '+first);
 edit(root,rel,s.slice(i,j),to);
}
function insert(name){return fs.readFileSync(path.join(__dirname,name),'utf8').replace(/^\uFEFF/,'').trimEnd();}
fs.copyFileSync(path.join(__dirname,'roi_decode.h'),path.join(cpp,'roi_decode.h'));
fs.copyFileSync(path.join(__dirname,'RoiFixture.ets'),path.join(ets,'render/RoiFixture.ets'));
const n='single_sand_napi.inc', w='window_probe_napi.inc';
edit(cpp,w,'SingleSandWindowProbe windowProbe;',`#include "roi_decode.h"
SingleSandWindowProbe windowProbe;
T385RoiDecode windowDecode;
uint64_t windowRequestId=0,windowJsElements=0,windowJsBytes=0;
int windowCallbackStatus=-1;`);
edit(cpp,w,'Put(e,o,"captureRevision","T372_SINGLE_SAND_WINDOW_PROBE_V1");',`Put(e,o,"captureRevision","T385_SINGLE_SAND_WINDOW_PROBE_V2");
 Put(e,o,"requestId",double(windowRequestId));
 Put(e,o,"jsElements",double(windowJsElements));Put(e,o,"jsByteLength",double(windowJsBytes));
 Put(e,o,"nativeCallbackStatus",double(windowCallbackStatus));Put(e,o,"nativeStatus",double(windowDecode.status));
 Put(e,o,"nativeTypedType",double(windowDecode.type));Put(e,o,"nativeRawByteLength",double(windowDecode.rawBytes));
 Put(e,o,"nativeByteOffset",double(windowDecode.offset));Put(e,o,"nativeBackingBytes",double(windowDecode.backingBytes));
 Put(e,o,"effectiveElements",double(windowDecode.elements));Put(e,o,"decodeStage",windowDecode.stage);
 napi_value coords=nullptr;napi_create_array_with_length(e,windowDecode.elements,&coords);
 for(int i=0;i<windowDecode.elements;i++){
  napi_value v=nullptr;napi_create_int32(e,windowDecode.words[i],&v);napi_set_element(e,coords,i,v);
 }
 napi_set_named_property(e,o,"requestedRoiWords",coords);`);
span(cpp,n,' SingleSandWindowProbe* probe=nullptr;',' SurfaceInfo surface{};auto current=',` SingleSandWindowProbe* probe=nullptr;
 if(reserved){
  windowProbe.packet=p;
  windowDecode=T385DecodeRoi(e,a[4]);
  windowProbe.stage=windowDecode.stage;
  if(windowDecode.elements){
   if(!windowRequestId||uint64_t(windowDecode.elements)!=windowJsElements||windowDecode.rawBytes!=windowJsBytes)windowProbe.stage="ROI_JS_NATIVE_IDENTITY";
   else if(!windowProbe.Configure(windowDecode.words.data(),windowDecode.elements)){
    windowProbe.stage="ROI_BOUNDS";windowProbe.count=windowProbe.used=0;
   }else{probe=&windowProbe;windowProbe.stage="PRE_PRESENT";}
  }
 }
`);
edit(cpp,n,' size_t n=5;napi_value a[5]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);',` size_t n=6;napi_value a[6]{};
 const auto cb=napi_get_cb_info(e,info,&n,a,nullptr,nullptr);`);
const header=' std::lock_guard<std::mutex> lock(g.mutex);SingleSandPacket p;SingleSandReceipt r;\n if((n!=4&&n!=5)||';
edit(cpp,n,header,` std::lock_guard<std::mutex> lock(g.mutex);SingleSandPacket p;SingleSandReceipt r;
 const bool reserved=n>=5&&!windowProbe.available;
 if(reserved){
  windowCallbackStatus=int(cb);windowDecode={};windowRequestId=windowJsElements=windowJsBytes=0;
  windowProbe.target={};windowProbe.ms=0;windowProbe.version[0]=windowProbe.renderer[0]=0;
  windowProbe.count=windowProbe.used=0;windowProbe.config={};windowProbe.unbound=false;windowProbe.unbindError=0;windowProbe.drawEglError=0;
  windowProbe.dither=windowProbe.blend=false;
  windowProbe.stage=cb!=napi_ok||n!=5?"ROI_ARITY":"PACKET_EARLY_RETURN";
  if(cb==napi_ok&&a[0]){
   SingleU64(e,a[0],"probeRequestId",windowRequestId);
   SingleU64(e,a[0],"probeElements",windowJsElements);
   SingleU64(e,a[0],"probeBytes",windowJsBytes);
   SingleText(e,a[0],"token",p.token);
  }
 }
 const auto finishProbe=[&](){
  if(reserved){windowProbe.packet=p;windowReceipt=r;windowProbe.available=true;}
 };
 if(cb!=napi_ok||(n!=4&&n!=5)||`);
const body=fs.readFileSync(path.join(cpp,n),'utf8');
const tail=body.slice(body.indexOf('napi_value DrawSingleSand'));
let patched=tail
 .replace('r.reason=p.Validate();if(r.reason!="OK")return SingleResult(e,r,p);','r.reason=p.Validate();if(r.reason!="OK"){r.stage="PACKET_VALIDATION";return SingleResult(e,r,p);}')
 .replaceAll('return SingleResult(e,r,p);','finishProbe();return SingleResult(e,r,p);').replaceAll('finishProbe();finishProbe();','finishProbe();').replace('finishProbe();\n finishProbe();','finishProbe();');
patched=patched.replace(' const double begin=NowMs();',` if(probe&&(surface.width!=p.width||surface.height!=p.height)){
  probe->stage="ROI_WINDOW_MISMATCH";probe=nullptr;
 }
 const double begin=NowMs();`)
 .replace(' finishProbe();\n return SingleResult', ' finishProbe();\n return SingleResult')
 .replace('r.reason="SINGLE_VERSION_UNSUPPORTED";finishProbe();','r.reason="SINGLE_VERSION_UNSUPPORTED";r.stage="PACKET_VERSION";finishProbe();')
 .replace('r.reason="UNSUPPORTED_MATERIAL_SET";finishProbe();','r.reason="UNSUPPORTED_MATERIAL_SET";r.stage="PACKET_MATERIAL";finishProbe();')
 .replace('r.reason="SINGLE_TYPED_RANGE";finishProbe();','r.reason="SINGLE_TYPED_RANGE";r.stage="PACKET_FIELDS";finishProbe();')
 .replace('r.reason="SINGLE_TYPED_RANGE";finishProbe();','r.reason="SINGLE_TYPED_RANGE";r.stage=stage;finishProbe();')
 .replace('r.reason="SINGLE_PUBLICATION_MISMATCH";finishProbe();','r.reason="SINGLE_PUBLICATION_MISMATCH";r.stage="PACKET_OWNER_EPOCH";finishProbe();');
edit(cpp,n,tail,patched);
edit(cpp,w,'Put(e,o,"captureStage",probe.stage);',`const char* captureStage=probe.stage;
 if(std::strcmp(captureStage,"PACKET_EARLY_RETURN")==0)captureStage=r.stage;
 else if(std::strcmp(captureStage,"PRE_PRESENT")==0&&r.state!="CURRENT")captureStage=r.stage;
 else if(std::strcmp(captureStage,"PRE_SWAP")==0){
  if(!clean(probe.target[0]))captureStage="CANDIDATE_GL";
  else if(!clean(probe.target[1]))captureStage="DEFAULT_BACK_GL";
  else if(!r.swapped)captureStage="SWAP";
  else if(!probe.unbound)captureStage="UNBIND";
  else if(!ok)captureStage="TOKEN_MISMATCH";
  else captureStage="COMPLETE";
 }
 Put(e,o,"captureStage",captureStage);`);
edit(ets,'render/P013SingleSandTypes.ets','  packet:string=', '  probeRequestId:number=0;probeElements:number=0;probeBytes:number=0;\n  packet:string=');
const ctrl='render/P013SingleSandController.ets', page='pages/P051DerivedV2Demo.ets';
span(ets,ctrl,'  private pendingRoi:','  setProbeNeutral(',insert('controller.inc')+'\n');
edit(ets,ctrl,"import { WindowProbeEvidence } from './SingleSandWindowCold';",
 "import { WindowProbeEvidence, WindowProbeRequest, cancelledWindowProbe } from './SingleSandWindowCold';\nimport { chooseWindowFixture, windowFixtureKey } from './RoiFixture';");
edit(ets,ctrl,'if(this.pendingRoi!==null||this.probePending)return;','if(this.probeBusy())return;');
span(ets,ctrl,'    const roi=this.pendingRoi;','    this.lastReceipt=receipt;',insert('draw.inc')+'\n');
edit(ets,page,"    Button(this.fixtureLocked?'恢复仿真':'锁定当前样本').onClick(()=>{this.model.stop();this.fixtureLocked=!this.fixtureLocked;})",`    Button(this.fixtureLocked?'恢复仿真':'锁定当前样本').onClick(()=>{this.singleSand.cancelWindowProbe('FIXTURE_CHANGED');this.probeFixtureKey='';this.model.stop();this.fixtureLocked=!this.fixtureLocked;})
    Row(){
      Button('预填默认 ROI').onClick(()=>{this.probeFixtureKey='';this.probeRoi='0,0,16,16';})
      Button('预填 BED/AIR/BG').onClick(()=>{
        const value=this.fixtureLocked?this.singleSand.windowFixture():'';
        if(value.length===0){this.probeStatus='夹具未就绪：需床、AIR 和背景';return;}
        this.probeFixtureKey=this.singleSand.windowFixtureKey();
        this.probeRoi=value;this.probeStatus='BED; AIR; BG · '+value;
      })
    }`);
edit(ets,page," @State private fixtureLocked:boolean=false;"," @State private fixtureLocked:boolean=false;\n private probeFixtureKey:string='';");
edit(ets,page,".onChange((value:string)=>this.probeRoi=value)",".onChange((value:string)=>{if(value!==this.probeRoi){this.probeFixtureKey='';this.probeRoi=value;}})");
edit(ets,page,'this.singleSand.armWindowProbe(roi)','this.singleSand.armWindowProbe(roi,this.probeFixtureKey)');
edit(ets,page,'this.evidence=null;this.probeStatus=manifest;','this.evidence=null;this.singleSand.ackWindowProbe();this.probeStatus=manifest;');
edit(ets,page,'onPageHide():void{this.active=false;', "onPageHide():void{this.singleSand.cancelWindowProbe('PAGE_HIDDEN');this.active=false;");
edit(ets,page,"Button('重置').onClick(()=>{this.fixtureLocked=false;","Button('重置').onClick(()=>{this.singleSand.cancelWindowProbe('RESET');this.probeFixtureKey='';this.fixtureLocked=false;");
edit(ets,page,'this.surfaceWidth=Number(area.width);this.surfaceHeight=Number(area.height);',`const w=Number(area.width),h=Number(area.height);
      if(w!==this.surfaceWidth||h!==this.surfaceHeight){
        this.singleSand.cancelWindowProbe('GEOMETRY_CHANGED');this.probeFixtureKey='';
      }
      this.surfaceWidth=w;this.surfaceHeight=h;`);
edit(ets,'render/SingleSandWindowCold.ets','interface WindowManifest {',`export class WindowProbeRequest {
 readonly id:number;readonly roi:Int32Array;readonly fixtureKey:string;readonly ownerKey:string;
 constructor(id:number,roi:Int32Array,fixtureKey:string,ownerKey:string){
  this.id=id;this.roi=roi;this.fixtureKey=fixtureKey;this.ownerKey=ownerKey;
 }
}
interface WindowManifest {
  requestId:number;
  captureRevision:string;
  captureStage:string;
  jsElements:number;
  jsByteLength:number;`);
edit(ets,'render/SingleSandWindowCold.ets','export function parseWindowRoi',`export function cancelledWindowProbe(request:WindowProbeRequest,stage:string):WindowProbeEvidence {
 const candidate:WindowTargetEvidence={},back:WindowTargetEvidence={};
 const manifest:WindowManifest={requestId:request.id,captureRevision:'T385_SINGLE_SAND_WINDOW_PROBE_V2',
  captureStage:stage,captureStatus:'CAPTURE_FAILED',jsElements:request.roi.length,jsByteLength:request.roi.byteLength,
  session:0,frameSerial:0,requestedToken:'',actualToken:'',candidate:candidate,back:back};
 const out:WindowProbeEvidence={manifest:manifest};return out;
}
export function parseWindowRoi`);
edit(ets,'render/SingleSandWindowCold.ets',"'t372-'+Date.now()+'-'+m.session+'-'+m.frameSerial","'t385-'+m.requestId+'-'+m.session+'-'+m.frameSerial");
const names=['generate.cjs','roi_decode.h','controller.inc','draw.inc','RoiFixture.ets'];
const sourceSha256=Object.fromEntries(names.map(n=>[n,crypto.createHash('sha256').update(fs.readFileSync(path.join(__dirname,n))).digest('hex')]));
fs.writeFileSync(path.join(out,'t385-manifest.json'),JSON.stringify({revision:'T385_SINGLE_SAND_WINDOW_PROBE_V2',isolated:true,device:'NOT_RUN',sourceSha256},null,2));
