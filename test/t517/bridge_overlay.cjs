const fs=require('node:fs'),path=require('node:path');
module.exports=function(main){
 const file=path.join(main,'cpp/gpu_resident/app_bridge.cpp');let s=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 const f=n=>fs.readFileSync(path.join(main,'cpp/gpu_resident/m3b',n),'utf8').replaceAll('\r\n','\n');
 const block=t=>'\n#ifdef GRS_APP_M1\n'+t+'\n#endif\n';
 const one=(a,b)=>{if(s.split(a).length!==2)throw Error('T517_BRIDGE_ANCHOR '+a);s=s.replace(a,b);};
 one('#include "foundation.h"','#include "m3b/controller.h"\n#include "m3b/scene.h"\n#include "m3b/kernel_assets.h"\n#include "foundation.h"');
 one('struct App : BridgeState {','struct App : BridgeState {\n'+block(f('bridge_fields.inc')));
 one('void RevokeLocked(const char* why){','void RevokeLocked(const char* why){if(m5Controller)m5Controller->Stop();');
 one('void ObserveControlLocked(uint64_t stamp){','void ObserveControlLocked(uint64_t stamp){if(m5Selected)return;');
 one('void loop(){',block('void InvalidateV5Locked(){if(m5Controller)m5Controller->Stop();}')+'\nvoid loop(){');
 one('void InvalidateLocked(){if(m3Controller)m3Controller->Stop();','void InvalidateLocked(){if(m5Controller)m5Controller->Stop();if(m3Controller)m3Controller->Stop();');
 one('active=value;','active=value;'+block('if(m5Controller)m5Controller->Stop();'));
 one('a.K=uint32_t(k);a.Kw=uint32_t(kw);','if(a.m5Controller)a.m5Controller->Stop();a.m5Selected=false;a.K=uint32_t(k);a.Kw=uint32_t(kw);');
 one('a.m3Selected=true;','a.m5Selected=false;a.m3Selected=true;');
 one('std::unique_ptr<AppTransport> gpu;std::unique_ptr<AppRun> run;AppScene scene;','std::unique_ptr<AppTransport> gpu;std::unique_ptr<AppRun> run;AppScene scene;'+block('std::shared_ptr<grs::m3b::ControllerV5> m5Local;grs::m3b::Scene m5Scene;uint64_t m5LastDisplay=0,m5LastDrawNs=0;bool m5Working=false;'));
 one('const bool runnable=','const bool runnable=(m5Selected&&m5Working&&!failed)||');
 one('run.reset();if(gpu)gpu->ContextLost();',block('if(m5Local){m5Local->Stop();m5Local->Release(true);m5Local.reset();m5Scene.Reset(true);std::lock_guard<std::mutex> lock(mutex);m5Controller.reset();}')+'\n run.reset();if(gpu)gpu->ContextLost();');
 one('||(!gpu&&!m3Local&&enabled&&!failed&&!resetBlocked)','||(!gpu&&!m3Local&&!m5Local&&enabled&&!failed&&!resetBlocked)');
 one('transportEpoch=seen;transportIdentity=id;',block(f('bridge_create.inc'))+'\n transportEpoch=seen;transportIdentity=id;');
 one('   if(run&&!failed&&!gap){',block(f('bridge_pump.inc'))+'\n   if(run&&!failed&&!gap){');
 one('napi_value init(napi_env e,napi_value out){',block(f('napi.inc'))+'\nnapi_value init(napi_env e,napi_value out){');
 const withWords=['m5Configure','m5Pulse','m5Target','m5Offer','m5ColdBegin','m5ColdState','m5ColdPage','m5ColdEnd','m5ResearchConfig','m5OfferInput64'];
 const noArgs=['m5Session','m5Quota','m5ColdCapabilities','m5SourceStats','m5InputClock','queryLiftClockCapability','verifyLiftClockEpoch','m5ResumeInput'];
 one('napi_property_descriptor d[]={','napi_property_descriptor d[]={\n'+block([...withWords,...noArgs].map(n=>' {"'+n+'",nullptr,'+n+',nullptr,nullptr,nullptr,napi_default,nullptr},').join('\n')));
 for(const name of ['coldCapabilities','coldTraceBeginV1','coldTraceStateV1','coldTracePageV1','coldTraceEndV1','executionDiagnosticsV1']){
  const at=s.indexOf('napi_value '+name+'('),brace=s.indexOf('{',at);if(at<0||brace<0)throw Error('T517_COLD_ANCHOR '+name);
  s=s.slice(0,brace+1)+block('if(app().m5Selected)return text(e,"{\\\"state\\\":\\\"UNSUPPORTED_COLD_REVISION\\\",\\\"required\\\":\\\"T527_V5_BOUNDED_COLD_V1\\\"}");')+s.slice(brace+1);
 }
 fs.writeFileSync(file,s);
 const types=path.join(main,'cpp/types/libgrsm1/index.d.ts');fs.appendFileSync(types,'\n'+withWords.map(n=>'export const '+n+':(words:Uint32Array)=>string;').join('\n')+'\n'+noArgs.map(n=>'export const '+n+':()=>string;').join('\n')+'\n');
};
