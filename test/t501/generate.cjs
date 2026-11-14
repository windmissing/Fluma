const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]),route=process.argv[3];if(!['m1','m2'].includes(route))throw Error('ROUTE');
cp.execFileSync(process.execPath,[require.resolve('../t492/generate.cjs'),out,route],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident');
function one(s,a,b){if(s.split(a).length!==2)throw Error('T501_ANCHOR '+a);return s.replace(a,b);}
function edit(file,fn){const p=path.join(main,file);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
for(const name of ['resource_ledger.h','resource_gl.h'])fs.copyFileSync('entry/src/main/cpp/gpu_resident/'+name,path.join(base,name));
edit('cpp/gpu_resident/program_cache/program_factory.h',s=>'#include "../resource_gl.h"\n'+s);
for(const target of ['m1',...(route==='m2'?['m2route']:[])]){
 for(const name of fs.readdirSync(path.join(base,target)).filter(n=>n.endsWith('.cpp')))
  edit('cpp/gpu_resident/'+target+'/'+name,s=>'#include "../resource_gl.h"\n'+s);
 edit('cpp/gpu_resident/'+target+'/source_transport.cpp',s=>one(s,'std::vector<uint32_t> cold(words,0),workspace(workWords,0);','std::vector<uint32_t> cold(words,0),workspace(workWords,0);resource501::HostScope coldStorage("SOURCE_COLD_VECTOR",cold.data(),cold.size()*4,cold.capacity()*4),workspaceStorage("SOURCE_WORK_VECTOR",workspace.data(),workspace.size()*4,workspace.capacity()*4);'));
 edit('cpp/gpu_resident/'+target+'/quantum_candidate.cpp',s=>one(s,'BuildPassTable();return TransportCode::OK;','BuildPassTable();if(resource501::current)resource501::current->HostStorage("QUANTUM_PASS_TABLE",reinterpret_cast<uintptr_t>(this),passTable.size()*sizeof(decltype(passTable)::value_type),passTable.capacity()*sizeof(decltype(passTable)::value_type));return TransportCode::OK;'));
 edit('cpp/gpu_resident/'+target+'/quantum_candidate.h',s=>'#include "../resource_gl.h"\n'+one(s,'class QuantumCandidate {\npublic:', 'class QuantumCandidate {\npublic:\n ~QuantumCandidate(){if(resource501::current)resource501::current->HostStorage("QUANTUM_PASS_TABLE",reinterpret_cast<uintptr_t>(this),0,0,false);}'));
}
edit('cpp/p013renderer/fullframe_targets.cpp',s=>'#include "../gpu_resident/resource_gl.h"\n'+s);
edit('cpp/gpu_resident/evidence_capture.h',s=>'#include "resource_ledger.h"\n'+one(s,'class EvidenceCapture {\npublic:', 'class EvidenceCapture {\npublic:\n std::shared_ptr<resource501::Ledger> resourceLedger;\n ~EvidenceCapture(){if(resourceLedger)resourceLedger->HostStorage("EVIDENCE_CAPTURE_BLOCK",reinterpret_cast<uintptr_t>(this),0,0,false);}'));
edit('cpp/gpu_resident/evidence_capture_request.inc',s=>one(one(s,'static std::unique_ptr<EvidenceCapture> NewCaptureStorage(){','std::unique_ptr<EvidenceCapture> NewCaptureStorage(){'), 'auto block=std::make_unique<EvidenceCapture>();','auto block=std::make_unique<EvidenceCapture>();block->resourceLedger=resources;resources->HostStorage("EVIDENCE_CAPTURE_BLOCK",reinterpret_cast<uintptr_t>(block.get()),sizeof(EvidenceCapture),sizeof(EvidenceCapture));'));
edit('cpp/gpu_resident/program_cache/artifact_store.h',s=>one(s,'  size_t Count() {',`  std::string Stats501(){std::lock_guard<std::mutex> lock(mutex_);size_t pins=0,capacity=0;for(const auto& entry:entries_){pins+=entry.second.use_count()>1;capacity+=entry.second->bytes.capacity();}std::ostringstream o;o<<"{\\"entries\\":"<<entries_.size()<<",\\"pinnedArtifacts\\":"<<pins<<",\\"queueJobs\\":"<<jobs_.size()<<",\\"payloadSize\\":"<<bytes_<<",\\"payloadCapacity\\":"<<capacity<<",\\"tempBytes\\":\\"NOT_OBSERVED_BY_T475\\"}";return o.str();}
  size_t Count() {`));
edit('cpp/gpu_resident/program_cache/artifact_store.h',s=>'#include <sstream>\n'+s);
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s='#include "resource_gl.h"\n'+s;
 s=one(s,'struct App : BridgeState {','struct App : BridgeState {\n std::shared_ptr<resource501::Ledger> resources{std::make_shared<resource501::Ledger>()};');
 s=one(s,' App(bool launch=true){',` App(bool launch=true){
 #ifdef GRS_APP_M1
 resources->HostStorage("APP_EMBEDDED",reinterpret_cast<uintptr_t>(this),sizeof(App),sizeof(App));
 resources->HostStorage("RESOURCE_LEDGER",reinterpret_cast<uintptr_t>(resources.get()),sizeof(resource501::Ledger),sizeof(resource501::Ledger));
 resources->HostStorage("TIMELINE_SNAPSHOT",reinterpret_cast<uintptr_t>(stableTimeline.get()),sizeof(TimelineSnapshot),sizeof(TimelineSnapshot));
 #endif`);
 s=one(s,'  bool retirePending=false;',`  bool retirePending=false;bool resourceShutdown=false;
  const auto resourceExit=[&](const char* stage,uint32_t error=0,bool complete=false){resources->ExitStage(stage,seen,resourceShutdown,reinterpret_cast<uintptr_t>(display),reinterpret_cast<uintptr_t>(surface),reinterpret_cast<uintptr_t>(context),reinterpret_cast<uintptr_t>(held),error,complete);};`);
 s=one(s,'   run.reset();if(gpu)gpu->ContextLost();gpu.reset();scene.Reset(false);if(display!=EGL_NO_DISPLAY){',`   resources->Tombstone(reinterpret_cast<uintptr_t>(context),seen);
   if(run)resources->HostStorage("APP_RUN",reinterpret_cast<uintptr_t>(run.get()),0,0,false);
   if(gpu)resources->HostStorage("APP_TRANSPORT",reinterpret_cast<uintptr_t>(gpu.get()),0,0,false);
   run.reset();if(gpu)gpu->ContextLost();gpu.reset();scene.Reset(false);resourceExit("RETIRE_ENTER");if(display!=EGL_NO_DISPLAY){`);
 for(const [call,name] of [['eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT)','UNBIND'],['eglDestroySurface(display,surface)','DESTROY_SURFACE'],['eglDestroyContext(display,context)','DESTROY_CONTEXT'],['eglTerminate(display)','TERMINATE']]){
  const old='if(!'+call+'){retirePending=true;return false;}';
  s=one(s,old,'resourceExit("'+name+'_ENTER");if(!'+call+'){const auto error=eglGetError();resourceExit("'+name+'_FAILED",uint32_t(error));retirePending=true;return false;}'+(name==='DESTROY_CONTEXT'?'resources->Retired(reinterpret_cast<uintptr_t>(context),seen);':'')+'resourceExit("'+name+'_EXIT");');
 }
 s=one(s,'return true;};\n  auto publish=', 'resourceExit("RETIRED",0,true);resource501::Bind(nullptr,0);return true;};\n  auto publish=');
 // Generation is bound only after the actual makeCurrent path succeeded.
 s=one(s,'    transportEpoch=seen;transportIdentity=id;', '    transportEpoch=seen;transportIdentity=id;resource501::Bind(resources.get(),seen);');
 s=one(s,'gpu=std::make_unique<AppTransport>();','gpu=std::make_unique<AppTransport>();resources->HostStorage("APP_TRANSPORT",reinterpret_cast<uintptr_t>(gpu.get()),sizeof(AppTransport),sizeof(AppTransport));');
 s=one(s,'run=std::make_unique<AppRun>(*gpu);','run=std::make_unique<AppRun>(*gpu);resources->HostStorage("APP_RUN",reinterpret_cast<uintptr_t>(run.get()),sizeof(AppRun),sizeof(AppRun));');
 s=one(s,'auto copy=std::make_unique<diagnostic492::Capture>();auto& a=app();','auto copy=std::make_unique<diagnostic492::Capture>();auto& a=app();resource501::HostScope measuredCopy(a.resources.get(),"EXECUTION_DIAGNOSTIC_COPY",copy.get(),sizeof(diagnostic492::Capture),sizeof(diagnostic492::Capture));');
 s=one(s,'  }retire();','  }resourceShutdown=true;retire();resource501::Bind(nullptr,0);');
 if(route==='m2'){
  s=one(s,' struct M2ColdTrace {',' struct M2ColdTrace {\n  std::shared_ptr<resource501::Ledger> ledger;\n  ~M2ColdTrace(){if(ledger)ledger->HostStorage("M2_TRACE_GENERATION",reinterpret_cast<uintptr_t>(this),0,0,false);}');
  s=one(s,'auto trace=std::make_shared<M2ColdTrace>();','auto trace=std::make_shared<M2ColdTrace>();trace->ledger=resources;');
  s=one(s,'  std::lock_guard<std::mutex> lock(mutex);\n  if(!CurrentLocked(seen,held)){m2TraceResult=', '  resources->HostStorage("M2_TRACE_GENERATION",reinterpret_cast<uintptr_t>(trace.get()),sizeof(M2ColdTrace)+(trace->state.size()+trace->events.size())*4,sizeof(M2ColdTrace)+(trace->state.capacity()+trace->events.capacity())*4);\n  std::lock_guard<std::mutex> lock(mutex);\n  if(!CurrentLocked(seen,held)){m2TraceResult=');
 }
 const napi=`#ifdef GRS_APP_M1
 napi_value resourceState(napi_env e,napi_callback_info){auto& a=app();auto snapshot=a.resources->Copy();std::ostringstream out;auto value=snapshot.Json();value.pop_back();out<<value<<",\\"cache\\":"<<programcache::Store().Stats501()<<",\\"snapshotOwnership\\":";{std::lock_guard<std::mutex> lock(a.mutex);out<<"{\\"active\\":"<<(a.stableTimeline->active?"true":"false")<<",\\"payload\\":753664,\\"storageRetainedAfterEnd\\":true}";}
 ${route==='m2'?'out<<",\\"trace\\":";{std::lock_guard<std::mutex> lock(a.mutex);out<<"{\\"currentId\\":\\""<<(a.m2Trace?a.m2Trace->id:0)<<"\\",\\"currentReadPins\\":"<<(a.m2Trace?a.m2Trace.use_count()-1:0)<<",\\"historicalPinnedGenerations\\":\\"SEE_HOST_OWNERSHIP_ROWS\\"}";}':''}
 out<<"}";return text(e,out.str());}
 #endif
 `;
 s=one(s,'napi_value timeline(napi_env',napi+'\nnapi_value timeline(napi_env');
 s=one(s,'{"timeline",nullptr,timeline','\n#ifdef GRS_APP_M1\n{"resourceState",nullptr,resourceState,nullptr,nullptr,nullptr,napi_default,nullptr},\n#endif\n{"timeline",nullptr,timeline');
 return s;
});
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const resourceState:()=>string;\n');
const manifest={revision:'T501_RESOURCE_OVERLAY_V1',route,files:{}};
for(const file of ['app_bridge.cpp','resource_ledger.h','resource_gl.h','program_cache/program_factory.h','program_cache/artifact_store.h',...(route==='m2'?['m2route/source_transport.cpp','m2route/quantum_candidate.h']:['m1/source_transport.cpp','m1/quantum_candidate.h'])])manifest.files[file]=crypto.createHash('sha256').update(fs.readFileSync(path.join(base,file))).digest('hex');
fs.writeFileSync(path.join(out,'t501-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
