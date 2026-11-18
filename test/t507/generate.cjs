const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]),route=process.argv[3];if(!['m1','m2'].includes(route))throw Error('ROUTE');
cp.execFileSync(process.execPath,[require.resolve('../t502/generate.cjs'),out,route],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident');
const read=n=>fs.readFileSync(path.join(__dirname,n),'utf8').replaceAll('\r\n','\n');
function one(s,a,b){if(s.split(a).length!==2)throw Error('T507_ANCHOR '+a);return s.replace(a,b);}
function edit(name,fn){const p=path.join(main,name);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
function replaceFunction(s,name,body){const begin=s.indexOf('napi_value '+name+'(');if(begin<0)throw Error('FUNCTION '+name);let at=s.indexOf('{',begin),level=0,quote='';for(let i=at;i<s.length;i++){const c=s[i];if(quote){if(c==='\\')i++;else if(c===quote)quote='';continue;}if(c==='"'||c==="'"){quote=c;continue;}if(c==='{')level++;if(c==='}'&&!--level)return s.slice(0,begin)+'napi_value '+name+'(napi_env e,napi_callback_info){'+body+'}'+s.slice(i+1);}throw Error('UNBALANCED');}
for(const name of ['cold_protocol.h','cold_trace.h','cold_diagnostics.h','cold_slot_broker.h','cold_slot_broker.cpp','cold_slot_broker.cmake'])fs.copyFileSync('entry/src/main/cpp/gpu_resident/'+name,path.join(base,name));
fs.appendFileSync(path.join(base,'CMakeLists.txt'),'\ninclude("${CMAKE_CURRENT_SOURCE_DIR}/cold_slot_broker.cmake")\n');
if(route==='m2'){
 edit('cpp/gpu_resident/m2route/full_transport.h',s=>one(s,' TransportCode Freeze(bool);',' TransportCode CaptureSpan(uint32_t*,size_t,size_t&,size_t&,GLenum&,const char*&);\n TransportCode Freeze(bool);'));
 edit('cpp/gpu_resident/m2route/full_transport.cpp',s=>{const marker='TransportCode FullM1Transport::Capture(std::vector';const at=s.indexOf(marker);if(at<0)throw Error('CAPTURE');return s.slice(0,at)+read('span.inc')+'\n'+s.slice(at);});
}
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s='#include "cold_trace.h"\n#include "cold_diagnostics.h"\n'+s;
 const anchor=' '+(route==='m2'?'m2route':'m1')+'::ProgressClockV2 progress;';
 s=one(s,anchor,read('bridge.inc')+'\n'+anchor);
 s=one(s,'   CheckEvidenceLocked();if(progressV2)', '   ColdMaintainLocked();CheckEvidenceLocked();if(progressV2)');
 if(route==='m2'){
  const begin=s.indexOf(' struct M2ColdTrace {'),end=s.indexOf('\n#endif\n\n void loop()',begin);if(begin<0||end<0)throw Error('LEGACY_TRACE_OWNER');
  s=s.slice(0,begin)+read('service.inc')+s.slice(end);
  s=one(s,'ServiceM2Trace(*gpu,seen,held);','ServiceColdTrace(*gpu,seen,held);');
  const beginNapi=s.indexOf('napi_value requestM2Trace('),endNapi=s.indexOf('\n#endif',beginNapi);if(beginNapi<0||endNapi<0)throw Error('LEGACY_TRACE_API');
  s=s.slice(0,beginNapi)+'napi_value requestM2Trace(napi_env e,napi_callback_info){return text(e,cold503::Upgrade);}\nnapi_value readM2Trace(napi_env e,napi_callback_info){return text(e,cold503::Upgrade);}\n'+s.slice(endNapi);
  const line=s.split('\n').find(l=>l.includes('historicalPinnedGenerations'));if(!line)throw Error('TRACE_LEDGER');
  s=one(s,line,' out<<",\\\"trace\\\":";{std::lock_guard<std::mutex> lock(a.mutex);out<<"{\\\"revision\\\":\\\"T503_M2_COLD_TRACE_V1\\\",\\\"capacityBytes\\\":"<<a.coldTrace.Capacity()<<",\\\"reservedBytes\\\":"<<a.coldTrace.Reserved()<<",\\\"readerActive\\\":"<<(a.coldTrace.reading?"true":"false")<<",\\\"builderActive\\\":"<<(a.coldTrace.builder>=0?"true":"false")<<"}";}');
 }
 s=replaceFunction(s,'executionDiagnostics','return text(e,cold503::Upgrade);');
 s=one(s,'napi_value newRunAction(',read('napi.inc')+'\nnapi_value newRunAction(');
 const apis=['coldCapabilities','coldTraceBeginV1','coldTraceStateV1','coldTracePageV1','coldTraceEndV1','executionDiagnosticsV1'];
 s=one(s,'{"executionDiagnostics",nullptr,executionDiagnostics',apis.map(n=>'{"'+n+'",nullptr,'+n+',nullptr,nullptr,nullptr,napi_default,nullptr},').join('\n')+'\n{"executionDiagnostics",nullptr,executionDiagnostics');
 return s;
});
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const coldCapabilities:()=>string;\nexport const coldTraceBeginV1:(scope:string,replaceId:string)=>string;\nexport const coldTraceStateV1:(requestId:string)=>string;\nexport const coldTracePageV1:(traceId:string,page:number)=>string;\nexport const coldTraceEndV1:(traceId:string)=>string;\nexport const executionDiagnosticsV1:(scope:string,remaining:number)=>string;\n');
require('./pages.cjs')(main,edit,one,route);
const manifest={revision:'T507_BOUNDED_COLD_OVERLAY_V1',route,files:{},integrationFiles:{}};
for(const name of ['app_bridge.cpp','cold_protocol.h','cold_trace.h','cold_diagnostics.h','execution_diagnostics.h',...(route==='m2'?['m2route/full_transport.cpp','m2route/full_transport.h']:[])])manifest.files[name]=crypto.createHash('sha256').update(fs.readFileSync(path.join(base,name))).digest('hex');
for(const name of ['physics/GpuTimelineSnapshotCold.ets','physics/GpuBoundedCold.ets','physics/GpuColdTraceV1.ets','physics/GpuBridgeTimelineCold.ets','pages/GpuResidentM1.ets','pages/GpuSandWaterExperience.ets'])manifest.integrationFiles[name]=crypto.createHash('sha256').update(fs.readFileSync(path.join(main,'ets',name))).digest('hex');
fs.writeFileSync(path.join(out,'t507-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
