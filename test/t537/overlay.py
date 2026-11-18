from pathlib import Path
import json,re,sys,hashlib
ROOT=Path(__file__).resolve().parents[2]
def one(s,a,b):
 if s.count(a)!=1:raise ValueError('T537_ANCHOR '+a[:100])
 return s.replace(a,b)
def end_delimiter(s,start,left='{',right='}'):
 depth=0;quote='';i=start
 while i<len(s):
  c=s[i]
  if quote:
   if c=='\\':i+=1
   elif c==quote:quote=''
  elif c in ['"',"'",'`']:quote=c
  elif c==left:depth+=1
  elif c==right:
   depth-=1
   if depth==0:return i+1
  i+=1
 raise ValueError('T537_DELIMITER')
def overlay(out,route):
 main=out/'entry/src/main';base=main/'cpp/gpu_resident'
 module=base/'input_diagnostic';module.mkdir(exist_ok=True)
 (module/'input_stage.h').write_bytes((ROOT/'entry/src/main/cpp/gpu_resident/input_diagnostic/input_stage.h').read_bytes())
 (module/'source_claim_witness.h').write_bytes((ROOT/'entry/src/main/cpp/gpu_resident/input_diagnostic/source_claim_witness.h').read_bytes())
 (main/'ets/physics/GpuInputStage.ets').write_bytes((ROOT/'entry/src/main/ets/physics/GpuInputStage.ets').read_bytes())
 read=lambda n:(ROOT/'test/t537'/n).read_text(encoding='utf8')
 wrap=lambda body:'\n#ifdef GRS_APP_M1\n'+body+'\n#endif\n'
 p=base/'cold_diagnostics.h';s=p.read_text(encoding='utf8')
 s=one(s,'#include "cold_protocol.h"','#include "cold_protocol.h"\n#include "input_diagnostic/input_stage.h"\n#include "input_diagnostic/source_claim_witness.h"')
 s=one(s,'size_t limit,bool stale){\n w.Raw("{\\\"revision\\\":\\\"T503_EXECUTION_DIAGNOSTICS_V1', 'size_t limit,bool stale,const input537::Ledger* input=nullptr,const claim575::Ledger* claims=nullptr){\n w.Raw("{\\\"revision\\\":\\\"T503_EXECUTION_DIAGNOSTICS_V1')
 anchor='CountsJson(w,summary);w.Raw(",\\\"stages\\\":[");'
 s=one(s,anchor,'CountsJson(w,summary);if(input){w.Raw(",\\\"inputStages\\\":");input->Json(w);}if(claims){w.Raw(",\\\"sourceClaimWitness\\\":");claims->Json(w);}w.Raw(",\\\"stages\\\":[");');p.write_text(s,encoding='utf8')
 for folder in ['m1']+(['m2route'] if route=='m2' else []):
  p=base/folder/'scene.h';s=p.read_text(encoding='utf8');s=one(s,'#pragma once','#pragma once\n#include "../input_diagnostic/input_stage.h"')
  s=one(s,' void Reset(bool);',' bool DrawDiagnostic(const ResidentSceneFrame&,uint64_t,CandidateCheck,ResidentScene::Swap,void*,input537::Pixel&);\n void Reset(bool);')
  s=one(s,' GLuint full=0,',' GLuint diagnosticPublication=0;\n GLuint full=0,');p.write_text(s,encoding='utf8')
  p=base/folder/'scene.cpp';s=p.read_text(encoding='utf8')
  s=s.replace('M1_SCENE_ABI_V1|','T537_SCENE_ABI_V2_LAYER7|')
  s=one(s,' targets.Reset(current);',' targets.Reset(current);if(current&&diagnosticPublication)glDeleteBuffers(1,&diagnosticPublication);diagnosticPublication=0;')
  at=s.index('ResidentSceneReceipt Scene::Draw(');s=s[:at]+read('scene_diagnostic.inc')+'\n'+s[at:];p.write_text(s,encoding='utf8')
 p=base/'m1/scene.frag';s=p.read_text(encoding='utf8');s=one(s,'void main(){','void main(){\n if(uLayer==7){float x=gl_FragCoord.x/uResolution.x;color=x<1./3.?vec4(1.,0.,0.,1.):x<2./3.?vec4(0.,1.,0.,1.):vec4(0.,0.,1.,1.);return;}');p.write_text(s,encoding='utf8')
 p=base/'app_bridge.cpp';s=p.read_text(encoding='utf8');s='#include "input_diagnostic/input_stage.h"\n'+s
 fields=''' input537::Ledger inputStages;claim575::Ledger claimWitness;
 std::atomic<bool> pixelSelected{false};bool pixelPulseOpen=false,pixelFailed=false,pixelDrawn=false;
 uint64_t pixelLastNative=0,pixelLastUi=0;input537::Pixel pixel;std::string pixelGeometry;
 bool PixelAllowedLocked(uint64_t candidate,void* held,uint64_t stamp)const{return pixelSelected&&!pixelFailed&&pixelPulseOpen&&active&&held&&CurrentLocked(candidate,held)&&stamp>=pixelLastNative&&stamp-pixelLastNative<=33333333;}
'''
 s=one(s,'struct App : BridgeState {','struct App : BridgeState {\n'+fields)
 s=one(s,'void DiagnosticOfferLocked(const std::array<uint32_t,32>& words,const char* result){','void DiagnosticOfferLocked(const std::array<uint32_t,32>& words,const char* result){inputStages.Packet(DiagnosticPointLocked("T537_NATIVE_OFFER"),words,result);if(std::strcmp(result,"ENQUEUED_NOT_ACK")==0){(void)claimWitness.onEnqueue(DiagnosticPointLocked("T575_SOURCE_CLAIM"),uint64_t(words[6])|(uint64_t(words[7])<<32),uint32_t(count),ui.boundary,(uint64_t(input.lo)|uint64_t(input.hi)<<32));}')
 s=one(s,'void DiagnosticActionLocked(const std::string& raw){','void DiagnosticActionLocked(const std::string& raw){const char* reached="{\\\"origin\\\":\\\"NATIVE_ACTION_API_REACHED\\\"}";inputStages.Add(9,DiagnosticPointLocked("T537_NATIVE_ACTION"),reached,std::strlen(reached));')
 s=one(s,'||(!gpu&&enabled&&!failed&&!resetBlocked)','||(!gpu&&!pixelSelected&&enabled&&!failed&&!resetBlocked)')
 s=one(s,'transportEpoch=seen;transportIdentity=id;resource501::Bind(resources.get(),seen);','transportEpoch=seen;transportIdentity=id;resource501::Bind(resources.get(),seen);'+wrap(read('pixel_create.inc')))
 s=one(s,'   if(run&&!failed&&!gap){',wrap(read('pixel_pump.inc'))+'\n   if(run&&!failed&&!gap){')
 # T575: claim witness at the actual head-consume point. The witness records
 # the first named no-claim reason or the accepted claim with real values.
 s=one(s,'FrontLocked(seen,gpu->InputGeneration(),command)){MarkLocked(BridgeStage::Consume,seen);auto c=run->Offer(command);lastRunOffer=int(c);if(c==ControlCode::OK)PopLocked();}}','FrontLocked(seen,gpu->InputGeneration(),command)){MarkLocked(BridgeStage::Consume,seen);\n#ifdef GRS_APP_M1\n{std::lock_guard<std::mutex> l(mutex);claimWitness.onClaim(0,DiagnosticPointLocked("T575_SOURCE_CLAIM"),uint32_t((Stamp()-sourceEnqueued[head])/1000000ull),(uint64_t(run->Tick().latest.nonce.hi)<<32)|run->Tick().latest.nonce.lo,(uint64_t(run->Tick().latest.nonce.hi)<<32)|run->Tick().latest.nonce.lo,run->Tick().latest.nonce.lo||run->Tick().latest.nonce.hi?1u:0u,(AllowedLocked()?1u:0u)|(epoch.load()==seen?8u:0u)|(run->Tick().active?16u:0u)|(1u<<2),(uint64_t(gpu->Owner().Header()[15])<<32)|gpu->Owner().Header()[14],"CLAIMED");}\n#endif\nauto c=run->Offer(command);lastRunOffer=int(c);if(c==ControlCode::OK)PopLocked();}}')
 # A single diagnostic gate owns both the original Capture copy and the new
 # input copy. The same pre-reserved UTF8 writer enforces the old total limit.
 s=one(s,'auto copy=std::unique_ptr<diagnostic492::Capture>', 'auto inputCopy=std::unique_ptr<input537::Ledger>(new(std::nothrow) input537::Ledger);\n auto copy=std::unique_ptr<diagnostic492::Capture>')
 s=one(s,'if(!copy||!buffer){','if(!copy||!buffer||!inputCopy){')
 s=one(s,'bool stale=false;','resource501::HostScope inputOwnership(a.resources.get(),"T537_INPUT_STAGE_COPY",inputCopy.get(),sizeof(*inputCopy),sizeof(*inputCopy));\n auto claimCopy=std::unique_ptr<claim575::Ledger>(new(std::nothrow) claim575::Ledger);\n bool stale=false;')
 s=one(s,'*copy=a.diagnostics;','*copy=a.diagnostics;*inputCopy=a.inputStages;*claimCopy=a.claimWitness;')
 s=one(s,'cold503::DiagnosticJson(w,*copy,summary,limit,stale);','cold503::DiagnosticJson(w,*copy,summary,limit,stale,inputCopy.get(),claimCopy.get());')
 s=one(s,'napi_value init(napi_env e,napi_value out){',wrap(read('napi.inc'))+'\nnapi_value init(napi_env e,napi_value out){')
 names=['recordInputStageV1','pixelDiagnosticStart','pixelDiagnosticPulse','pixelDiagnosticStop','pixelDiagnosticSnapshot']
 s=one(s,'napi_property_descriptor d[]={','napi_property_descriptor d[]={'+wrap('\n'.join('{"'+n+'",nullptr,'+n+',nullptr,nullptr,nullptr,napi_default,nullptr},' for n in names)))
 p.write_text(s,encoding='utf8')
 types=main/'cpp/types/libgrsm1/index.d.ts';s=types.read_text(encoding='utf8');s+='\nexport const recordInputStageV1:(code:number,raw:string)=>string;\nexport const pixelDiagnosticStart:(geometry:string)=>string;\nexport const pixelDiagnosticPulse:(ns:number)=>string;\nexport const pixelDiagnosticStop:()=>string;\nexport const pixelDiagnosticSnapshot:()=>string;\n';types.write_text(s,encoding='utf8')
 p=main/'ets/physics/GpuM1DemoController.ets';s=p.read_text(encoding='utf8');s="import { recordInputStage, InputStagePort } from './GpuInputStage';\n"+s
 s=one(s,'export interface GpuM1DemoPort {','export interface GpuM1DemoPort extends InputStagePort {')
 report=''' private reportAllowed537(s:GpuM1DemoStatus|null):void {
  if(s===null){recordInputStage(this.port,5,'{"origin":"CONTROLLER_ALLOWED","snapshot":false}');return;}
  const fields=[s.runLo,s.runHi,s.inputLo,s.inputHi,s.geometryLo,s.geometryHi,s.configLo,s.configHi,s.sourceControlLo,s.sourceControlHi];
  recordInputStage(this.port,5,JSON.stringify({origin:'CONTROLLER_ALLOWED',active:this.active,revision:s.sourceUiRevision===this.revision,
   ready:s.ready,permission:s.permissionState==='READY',epochFinite:Number.isSafeInteger(s.epoch)&&s.epoch>0,
   readyEpoch:s.epoch===s.readyEpoch,wordsValid:fields.every((v:number)=>this.word(v)),sourceOpen:s.sourceOpen,
   sourceKind:s.sourceKind,closePending:s.sourceClosePending??false,boundary:[s.sourceControlLo,s.sourceControlHi],displayState:s.state}));
 }
'''
 s=one(s,' private allowed(s:GpuM1DemoStatus)',report+' private allowed(s:GpuM1DemoStatus)')
 at=s.index(' down(');end=s.index('\n move(',at);body=s[at:end];body=one(body,'const s=this.snapshot;','const s=this.snapshot;this.reportAllowed537(s);');s=s[:at]+body+s[end:]
 s=one(s,'this.port.recordDiagnosticAction(JSON.stringify({','const inputApiReply537=this.port.recordDiagnosticAction(JSON.stringify({')
 s=one(s,'detail:detail}));','detail:detail}));recordInputStage(this.port,6,JSON.stringify({origin:"CONTROLLER_ACTION_REPLY",event:event,reply:inputApiReply537}));')
 p.write_text(s,encoding='utf8')
 for page,render in ([('GpuResidentM1.ets','grs-m1')] if route=='m1' else [('GpuSandWaterExperience.ets','grs-m1-experience')]):
  p=main/'ets/pages'/page
  if not p.exists():continue
  s=p.read_text(encoding='utf8');s="import componentUtils from '@ohos.arkui.componentUtils';\nimport { recordPageTouch } from '../physics/GpuInputStage';\n"+s
  requeue='queued=false;this.schedule();' if page=='GpuResidentM1.ets' else 'getUIContext().postFrameCallback(this.frame);'
  methods=read('page_methods.inc').replace('T537_RENDER_ID','t537-render').replace('this.PIXEL_REQUEUE_537','this.'+requeue)
  methods=methods.replace("if(!this.pixelMode)return false;","if(!this.pixelMode)return false;if(!this.active)return true;")
  name=page[:-4];s=one(s,'struct '+name+' {','struct '+name+' {\n'+methods)
  sensor=re.search(r'private readonly accelerometer[\s\S]*?=>\s*\{',s)
  if sensor:s=s[:sensor.end()]+'if(this.pixelMode)return;'+s[sensor.end():]
  s=one(s,'const code:string=this.pulse.pulse(ns);','if(this.owner.pixelFrame537(ns))return;const code:string=this.pulse.pulse(ns);')
  s=one(s,'private restart():void{','private restart():void{this.pixelMode=false;m1.pixelDiagnosticStop();')
  match=re.search(r'private touch\((\w+):\s*TouchEvent\):\s*void\s*\{',s)
  if not match:raise ValueError('TOUCH_HANDLER '+page)
  event=match[1];s=s[:match.end()]+f'recordPageTouch(m1,{event},this.inputWidth537,this.inputHeight537,this.inputLayout537,"t537-touch");if(this.pixelMode)return;'+s[match.end():]
  build=re.search(r'\bbuild\(\)\s*\{',s);column=re.search(r'Column\([^)]*\)\s*\{',s[build.end():]);start=build.end()+column.end()-1;end=end_delimiter(s,start);body=s[start+1:end-1]
  x=body.index('XComponent(');touch=body.index('.onTouch(',x);touchEnd=end_delimiter(body,touch+len('.onTouch'),'(',')');body=body[:x]+body[touchEnd:]
  body=re.sub(r'if\s*\(this\.validCapability\)\s*\{\s*\}','',body)
  stack="""Stack({alignContent:Alignment.TopStart}) {
   XComponent({id:'RENDER_ID',type:XComponentType.SURFACE,libraryname:'grsm1'}).id('t537-render').width('100%').height('100%').zIndex(0).hitTestBehavior(HitTestMode.None)
   Column(){}.id('t537-touch').width('100%').height('100%').zIndex(1).hitTestBehavior(HitTestMode.Block).onTouch((event:TouchEvent)=>this.touch(event))
  }.id('t537-render-stack').width('100%').layoutWeight(1).onAreaChange((_old:Area,area:Area)=>this.geometry537(area))
  Scroll(){Column({space:4}) {
   Text(this.pixelMode?'DIAGNOSTIC_NOT_PHYSICS · '+this.pixelMessage:'物理研究模式').fontSize(10).height(28).maxLines(2)
   Row({space:4}){Button('诊断三色图').onClick(()=>this.beginPixelDiagnostic537());Button('导出诊断帧').onClick(()=>this.exportPixel537());Button('返回新空场').onClick(()=>this.restart());}
  if(!this.pixelMode){BODY_537}
  }.width('100%')}.width('100%').height(300)
""".replace('RENDER_ID',render).replace('BODY_537',body)
  stack=stack.replace('  if(!this.pixelMode){', 'if(!this.pixelMode){\n  ').replace(' }\n  }.width', ' \n  }}.width').replace('    }\n  }.width','    \n  }}.width')
  s=s[:start+1]+stack+s[end-1:];p.write_text(s,encoding='utf8')
 manifest={'revision':'T537_STABLE_INPUT_PIXEL_DIAGNOSTIC_V1','route':route,'tests':'NOT_RUN_BY_USER_DIRECTION','device':'NOT_RUN','files':{}}
 for rel in ['cpp/gpu_resident/app_bridge.cpp','cpp/gpu_resident/cold_diagnostics.h','ets/physics/GpuM1DemoController.ets','ets/physics/GpuInputStage.ets','ets/pages/GpuResidentM1.ets','ets/pages/GpuSandWaterExperience.ets']:
  manifest['files'][rel]=hashlib.sha256((main/rel).read_bytes()).hexdigest()
 (out/'t537-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
if __name__=='__main__':overlay(Path(sys.argv[1]),sys.argv[2])
