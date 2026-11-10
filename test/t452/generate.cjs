const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
if(!process.argv[2])throw Error('Usage: node test/t452/generate.cjs <new-project-directory>');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t438/generate.cjs'),out,'experiment'],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident');
const source=path.resolve('entry/src/main/cpp/gpu_resident/m2');
const route=path.join(base,'m2route');fs.mkdirSync(route);
for(const name of ['layout_generated.h','root_gate.h','capability.h'])fs.copyFileSync(path.join(source,name),path.join(base,'m2',name));
const files=['source_owner.cpp','source_owner.h','source_transport.cpp','source_transport.h','quantum_owner.inc',
 'quantum_candidate.cpp','quantum_candidate.h','full_transport.cpp','full_transport.h','source_kernel.cpp','quantum_kernel.cpp',
 'scene.cpp','scene.h','ui_state.h','layout_generated.h','profile.cpp','profile.h','root_isolation.h','run.cpp','run.h','progress_clock_v2.h'];
for(const n of files){const body=fs.readFileSync(path.join(base,'m1',n),'utf8').replaceAll('\r\n','\n').replaceAll('namespace m1 {','namespace m2route {');fs.writeFileSync(path.join(route,n),body);}
const rel=n=>'cpp/gpu_resident/m2route/'+n;
function edit(file,fn){const p=path.join(main,file);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T452_ROUTE_BASE_DRIFT '+a);return s.replace(a,b);}
function insert(s,at,value){return one(s,at,value+at);}
function span(s,start,end,value){const a=s.indexOf(start),b=s.indexOf(end,a);if(a<0||b<0)throw Error('T452_ROUTE_SPAN '+start);return s.slice(0,a)+value+s.slice(b);}
const shader=[];
for(const [i,n] of ['source','quantum'].entries()){
 const temp=path.join(out,n+'-v3.glsl');
 cp.execFileSync('python',['-X','utf8',path.join(source,'assemble_kernels.py'),temp,...(i?['--quantum']:[])]);
 shader.push(fs.readFileSync(temp,'utf8'));
}
const shaderSha=crypto.createHash('sha256').update(shader.join('\n')).digest('hex');
const layoutSha=crypto.createHash('sha256').update(fs.readFileSync('docs/analysis-T445-gpu-M2-wire-v3.json')).digest('hex');
fs.writeFileSync(path.join(route,'full_kernel.cpp'),`#include "full_transport.h"
namespace grs::m2route {
const char* FullM1ShaderSha(){return "${shaderSha}";}
const char* FullSourceKernel(){return R"GRS_M2(${shader[0]})GRS_M2";}
const char* QuantumCommitKernel(){return R"GRS_M2(${shader[1]})GRS_M2";}
}
`);
edit(rel('layout_generated.h'),s=>s.replace(/manifestSha="[0-9a-f]+"/,'manifestSha="'+layoutSha+'"').replace('revision=2,features=3','revision=3,features=7'));
edit(rel('progress_clock_v2.h'),()=>`#pragma once
#include "../m1/progress_clock_v2.h"
namespace grs::m2route {using grs::m1::ProgressClockV2;using grs::m1::PlanGrantV2;}
`);
edit(rel('source_owner.cpp'),s=>{
 s=insert(s,'#include "source_owner.h"','#include "../m2/root_gate.h"\n');
 s=s.replace('header[1]=2;','header[1]=3;').replace('for(size_t i=27;i<=30;i++)header[i]=2;header[31]=3;','header[27]=2;for(size_t i=28;i<=30;i++)header[i]=3;header[31]=7;');
 s=insert(s,' bank=0;lastNonce=', ' header[132]=1;header[133]=header[134]=122333;header[135]=65536;header[136]=K;header[140]=128;\n');
 s=s.replace('r.eventCapacity!=K+1||r.dispatchCount!=8||r.barrierCount!=9','r.eventCapacity!=2*K+1||r.dispatchCount!=13||r.barrierCount!=14');
 s=span(s,'bool SourceRootOwner::Valid(', 'bool SourceRootOwner::InspectCompletion(',`bool SourceRootOwner::Valid(const uint32_t* c,size_t n,uint32_t error)const{
 const auto* p=grs::m2::SelectProfile(K,Kw,3,7,1);if(!p)return false;
 if(c&&n==256&&(c[59]!=command[14]||c[60]!=command[15]||
    (c[28]==1&&((command[2]==0&&c[158]!=0)||(command[2]==1&&(c[158]<1||c[158]>2))))))return false;
 grs::m2::RootGate gate(*p,identity,header,bank);AcceptedRoot out;
 return gate.Begin(pending)&&gate.Inspect(c,n,error,out);
}
`);
 s=insert(s,' auto& pub=pubs[out.publication];',' for(size_t i=0;i<13;++i)header[132+i]=c[160+i];header[81]=3;\n');
 return insert(s,'#include "source_owner.h"','#include "../m2/capability.h"\n');
});
edit(rel('source_owner.h'),s=>s.replace('#include "profile.h"','#include "profile.h"\n#include "../m2/root_gate.h"').replace(' uint32_t Bank()const',' std::string RootToken()const{return grs::m2::RootTokenV3(rootTail); }\n uint32_t Bank()const').replace(' Words256 header{};',' Words256 rootTail{},header{};'));
edit(rel('source_owner.cpp'),s=>insert(s,' for(size_t i=0;i<13;++i)header[132+i]=c[160+i];',' rootTail=out.completion;\n'));
edit(rel('quantum_owner.inc'),s=>s.replace('c[1]!=2','c[1]!=3').replace('for(size_t i=160;i<256;i++)if(c[i])return false;', 'for(size_t i=173;i<256;i++)if(c[i])return false;for(size_t i=0;i<13;++i)if(c[160+i]!=header[132+i])return false;'));
edit(rel('source_transport.h'),s=>one(s,'GLuint program=0','GLuint source128[2]{},scratch128=0;\n GLuint program=0'));
edit(rel('source_transport.cpp'),s=>{
 s=insert(s,'#include "source_transport.h"','#include "../m2/capability.h"\n');
 s=one(s,'workWords=96+4096*64+words+4096*16;', 'workWords=96+4096*64+words+4096*16+2*(K==192?256:512)+8320;');
 s=insert(s,' if(!alloc(work,workWords,workspace.data()))',' for(auto& b:source128)if(!alloc(b,size_t(2*K+1)*128,nullptr))return fail(TransportCode::MEMORY_PROFILE_UNSUPPORTED);\n if(!alloc(scratch128,size_t(2*K+1)*128,nullptr))return fail(TransportCode::MEMORY_PROFILE_UNSUPPORTED);\n');
 s=s.replace('work,events[0],submitObserver','work,source128[0],submitObserver');
 s=insert(s,' for(size_t i=0;i<2;i++)if(!alloc(banks',` const uint32_t configBase=uint32_t(workWords)-64;
 const uint32_t frozen[]={1,122333,122333,244666,65536,K,K,Kw,K==192?256u:512u,4096,128,2*K+1,
  3,3,3,3,3,7,0,1,1,1,2,1,1,1};
 std::copy(std::begin(frozen),std::end(frozen),workspace.begin()+configBase);
`);
 s=insert(s,' if(!uniforms){owner.Stop();',` const int32_t angle=int32_t(owner.Header()[45]);
 const int tangentLocation=glGetUniformLocation(program,"uBaseTangent");
 if(angle< -7680||angle>7680||tangentLocation<0)uniforms=false;
 else glUniform1i(tangentLocation,int32_t(kAngleWords[4u*uint32_t(angle+7680)]));
`);
 s=insert(s,'#include "../m2/capability.h"','#include <algorithm>\n#include "../angles_generated.h"\n');
 s=s.replace('r.uploadedBytes=128+12*4+3*8+8*4;', 'r.uploadedBytes=128+9*4+3*8+13*4;');
 s=s.replace('r.eventCapacity=K+1;r.dispatchCount=8;r.barrierCount=9','r.eventCapacity=2*K+1;r.dispatchCount=13;r.barrierCount=14');
 s=s.replace('Set("uWater",16*K)&&Set("uColumn",16*K+16*Kw)&&Set("uHeader",uint32_t(headerAt))&&Set("uWords",uint32_t(words))&&','');
 s=s.replace('pass<8;pass++','pass<13;pass++').replace('pass==5||pass==6?pubs[r.publicationSlot]:pass==7?results[r.publicationSlot]:isolation?scratchEvent:events[r.eventArena]','pass==10||pass==11?pubs[r.publicationSlot]:pass==12?results[r.publicationSlot]:scratch128');
 s=s.replace('row.groups=uint32_t((words+63)/64);','row.groups=pass==0||pass==6?uint32_t(words/64):pass==2?(K==192?4:8):1;');
 s=s.replace('(pass==5||pass==6?pubWords:pass==7?256:size_t(K+1)*64)*4','(pass==10||pass==11?pubWords:pass==12?256:size_t(2*K+1)*128)*4');
 s=s.replace('if(isolation)std::swap(events[pending.eventArena],scratchEvent);','std::swap(source128[pending.eventArena],scratch128);');
 s=s.replace('glDeleteBuffers(2,events);','glDeleteBuffers(2,source128);glDeleteBuffers(1,&scratch128);source128[0]=source128[1]=scratch128=0;glDeleteBuffers(2,events);');
 return s;
});
edit(rel('full_transport.cpp'),s=>{
 s=s.replace('uint32_t expanded=replayFrozen+256;','uint32_t expanded=replayFrozen+256+8192;');
 s=s.replace('if(order==0&&frameHasSource&&lastOrdinal==0){prefix=source.tail[53];prefixBuffer=source.events[source.pending.eventArena];}else if','if');
 s=s.replace('ledgerPrefix=preparation.ledgerPrefix;','ledgerPrefix=true;').replace('h[1]==2','h[1]==3').replace('h[28]==2&&h[29]==2&&h[30]==2&&h[31]==3','h[28]==3&&h[29]==3&&h[30]==3&&h[31]==7');
 s=s.replace('uint32_t a=acceptedArena,n=tail[53];','uint32_t a=acceptedArena,n=acceptedQuantum?tail[53]:tail[169],stride=acceptedQuantum?64u:128u;');
 s=s.replace('eventArenas[a]:source.events[a]','eventArenas[a]:source.source128[a]').replace('GLsizeiptr(n)*256','GLsizeiptr(n)*stride*4').replace('log.resize(size_t(n)*64)','log.resize(size_t(n)*stride)');
 return s;
});
edit(rel('scene.cpp'),s=>s.replace('GRS_GPU_SCENE_Q20_M1_V2','GRS_GPU_SCENE_Q20_M2_V3').replace('header[1]!=2||header[31]!=3','header[1]!=3||header[31]!=7').replace('r.requestedToken=Token(lease,f,optics);',"r.requestedToken=Token(lease,f,optics)+\"|ROOT|\"+source.Owner().RootToken();"));
edit('cpp/gpu_resident/CMakeLists.txt',s=>s+'\ntarget_sources(grs_foundation PRIVATE '+[...files.filter(n=>n.endsWith('.cpp')),'full_kernel.cpp'].map(n=>'m2route/'+n).join(' ')+')\n');
edit('cpp/gpu_resident/app_bridge.cpp',s=>s.replaceAll('"m1/run.h"','"m2route/run.h"').replaceAll('"m1/scene.h"','"m2route/scene.h"').replaceAll('"m1/ui_state.h"','"m2route/ui_state.h"').replace(/m1::(?!kExperienceProgressV2|ExperienceCapabilities)/g,'m2route::').replace('commandRevision=2','commandRevision=3').replace('feature!=3||revision!=2','feature!=7||revision!=3'));
edit('cpp/gpu_resident/evidence_capture_app.inc',s=>s.replaceAll('m1::completion256','m2route::completion256'));
edit('cpp/gpu_resident/m1/experience_capabilities.h',s=>s.replace('T438_EXPERIMENT_V2','T452_M2_EXPERIMENT_V3').replace('features\\\":3','features\\\":7').replace('layoutRevision\\\":2','layoutRevision\\\":3').replace('sceneRevision\\\":2','sceneRevision\\\":3').replace('GRS_GPU_SCENE_Q20_M1_V2','GRS_GPU_SCENE_Q20_M2_V3').replace('M2\\\":\\\"UNSUPPORTED_M1_CONFIG','M2\\\":\\\"M2_SAND_SAME_ORIGIN_AIR_V1'));
edit('ets/physics/GpuM1DemoController.ets',s=>s.replace('w[1]=2;','w[1]=3;').replace('configureM1(k,kw,3,2)','configureM1(k,kw,7,3)'));
edit('ets/pages/Index.ets',s=>s.replace('进入 GPU 沙水体验','开启独立 GPU M2 V3 空场'));
edit('ets/pages/GpuSandWaterExperience.ets',s=>s.replace('caps.features === 3','caps.features === 7').replace('GPU 沙水体验 ·','GPU M2 V3 ·'));
fs.writeFileSync(path.join(out,'t452-m2-manifest.json'),JSON.stringify({shaderSha,layoutSha,policy:'M2_SAND_SAME_ORIGIN_AIR_V1',base:'0228a7d'},null,2));
const local=n=>fs.readFileSync(path.join(__dirname,n),'utf8');
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s=insert(s,' void loop(){',local('trace_app.inc')+'\n');
 s=insert(s,'   if(run&&!failed&&!gap){','   if(run&&gpu)ServiceM2Trace(*gpu,seen,held);\n');
 s=insert(s,'napi_value setProgressV2(',local('trace_napi.inc')+'\n');
 return one(s,'{"setSource",nullptr,sourceMode','{"requestM2Trace",nullptr,requestM2Trace,nullptr,nullptr,nullptr,napi_default,nullptr},{"readM2Trace",nullptr,readM2Trace,nullptr,nullptr,nullptr,napi_default,nullptr},{"setSource",nullptr,sourceMode');
});
edit('cpp/gpu_resident/evidence_capture_napi.inc',s=>span(s,'napi_value beginEvidenceCapture(', 'napi_value captureRequestApi(',`napi_value beginEvidenceCapture(napi_env e,napi_callback_info){return text(e,"{\\\"state\\\":\\\"REJECTED\\\",\\\"reason\\\":\\\"V3_USE_M2_TRACE\\\"}");}
`));
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const requestM2Trace:()=>string;\nexport const readM2Trace:(id:string,page:number)=>string;\n');
edit('ets/pages/GpuSandWaterExperience.ets',s=>{
 s=`import { common } from '@kit.AbilityKit';
import { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';
import { ColdFile } from '../physics/GpuBridgeTimelineCold';
`+s;
 s=insert(s,'interface ExperienceCapability',`interface M2TraceMeta { state:string; traceId:string; page:number; pages:number; }
`);
 s=insert(s,'  build() {',' private exportingTrace:boolean=false;\n'+local('trace_page.inc'));
 return insert(s,"      Text(this.operation)",`      Row({space:8}) {
        Button('锁存 V3 冷档').onClick(()=>{if(!this.exportingTrace)this.operation=m1.requestM2Trace();})
        Button('导出 V3 冷档').onClick(()=>this.exportM2Trace())
      }
`);
});
edit('cpp/gpu_resident/bridge_state.h',s=>one(s,'if(w[1]!=2||','if(w[1]!=commandRevision||'));
edit(rel('full_transport.cpp'),s=>{
 s=insert(s,'#include "full_transport.h"','#include "../m2/capability.h"\n');
 return insert(s,' if(active||source.ready',` grs::m2::Limits m2Limits;
 const auto m2Capability=grs::m2::ProbeCurrent(grs::m2::SelectProfile(K,Kw,3,7,1),m2Limits);
 if(m2Capability!=grs::m2::ProbeCode::OK)return TransportCode::CAPABILITY_UNSUPPORTED;
`);
});
