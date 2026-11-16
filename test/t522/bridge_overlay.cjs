const fs=require('node:fs'),path=require('node:path');
module.exports=function(main){
 const file=path.join(main,'cpp/gpu_resident/app_bridge.cpp');let source=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 const fragment=n=>fs.readFileSync(path.join(main,'cpp/gpu_resident/m3a',n),'utf8').replaceAll('\r\n','\n');
 const m3=s=>'\n#ifdef GRS_APP_M1\n'+s+'\n#endif\n';
 const one=(a,b)=>{if(source.split(a).length!==2)throw Error('M3_BRIDGE_ANCHOR '+a);source=source.replace(a,b);};
 one('#include "foundation.h"','#include "m3a/controller.h"\n#include "m3a/scene.h"\n#include "m3a/kernel_assets.h"\n#include "m3a/cold_contract.h"\n#include "angles_generated.h"\n#include "foundation.h"');
 one('struct App : BridgeState {','struct App : BridgeState {\n'+m3(fragment('bridge_fields.inc')));
 one('void RevokeLocked(const char* why){','void RevokeLocked(const char* why){\n if(m3Controller)m3Controller->Stop();');
 one('void ObserveControlLocked(uint64_t stamp){','void ObserveControlLocked(uint64_t stamp){\n if(m3Selected)return;');
 one('void loop(){',m3('void InvalidateLocked(){if(m3Controller)m3Controller->Stop();BridgeState::InvalidateLocked();}')+'\n void loop(){');
 one('active=value;','active=value;'+m3('if(m3Controller)m3Controller->Stop();'));
 one('a.K=uint32_t(k);a.Kw=uint32_t(kw);','if(a.m3Controller)a.m3Controller->Stop();a.m3Selected=false;a.K=uint32_t(k);a.Kw=uint32_t(kw);');
 one('std::unique_ptr<AppTransport> gpu;std::unique_ptr<AppRun> run;AppScene scene;','std::unique_ptr<AppTransport> gpu;std::unique_ptr<AppRun> run;AppScene scene;'+m3('std::shared_ptr<grs::m3a::ControllerV4> m3Local;grs::m3a::Scene m3Scene;uint64_t m3LastDisplay=0,m3LastDrawNs=0;bool m3Working=false;'));
 one('const bool runnable=AllowedLocked()&&run','const bool runnable=(m3Selected&&m3Working&&!failed)||AllowedLocked()&&run');
 one('run.reset();if(gpu)gpu->ContextLost();',m3('if(m3Local){m3Local->Stop();m3Local->Release(true);m3Local.reset();m3Scene.Reset(true);std::lock_guard<std::mutex> lock(mutex);m3Controller.reset();}')+'\n   run.reset();if(gpu)gpu->ContextLost();');
 one('||(!gpu&&enabled&&!failed&&!resetBlocked)','||(!gpu&&!m3Local&&enabled&&!failed&&!resetBlocked)');
 one('transportEpoch=seen;transportIdentity=id;',m3(fragment('bridge_create.inc'))+'\n    transportEpoch=seen;transportIdentity=id;');
 one('   if(run&&!failed&&!gap){',m3(fragment('bridge_pump.inc'))+'\n   if(run&&!failed&&!gap){');
 one('napi_value init(napi_env e,napi_value out){',m3(fragment('napi.inc'))+'\nnapi_value init(napi_env e,napi_value out){');
 one('napi_property_descriptor d[]={','napi_property_descriptor d[]={\n'+m3(['m3Configure','m3Session','m3Pulse','m3Target','m3Offer','m3Quota','m3ColdBegin','m3ColdState','m3ColdPage','m3ColdEnd','m3ColdCapabilities','m3SourceStats'].map(n=>' {"'+n+'",nullptr,'+n+',nullptr,nullptr,nullptr,napi_default,nullptr},').join('\n')));
 for(const name of ['coldCapabilities','coldTraceBeginV1','coldTraceStateV1','coldTracePageV1','coldTraceEndV1','executionDiagnosticsV1']){
  const signature='napi_value '+name+'(';const at=source.indexOf(signature);if(at<0)throw Error('M3_LEGACY_COLD_ANCHOR '+name);const brace=source.indexOf('{',at);
  source=source.slice(0,brace+1)+m3('if(app().m3Selected)return text(e,"{\\\"state\\\":\\\"UNSUPPORTED_COLD_REVISION\\\",\\\"required\\\":\\\"GRS_M3_BOUNDED_COLD_V1\\\"}");')+source.slice(brace+1);
 }
 fs.writeFileSync(file,source);
 const types=path.join(main,'cpp/types/libgrsm1/index.d.ts');fs.appendFileSync(types,'\n'+['m3Configure','m3Pulse','m3Target','m3Offer','m3ColdBegin','m3ColdState','m3ColdPage','m3ColdEnd'].map(n=>'export const '+n+':(words:Uint32Array)=>string;').join('\n')+'\nexport const m3Session:()=>string;\nexport const m3Quota:()=>string;\nexport const m3ColdCapabilities:()=>string;\nexport const m3SourceStats:()=>string;\n');
};
