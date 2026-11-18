const fs=require('node:fs'),path=require('node:path');
module.exports=function(main){
 const file=path.join(main,'cpp/gpu_resident/app_bridge.cpp');let s=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 const f=n=>fs.readFileSync(path.join(main,'cpp/gpu_resident/g9spf',n),'utf8').replaceAll('\r\n','\n');
 const block=t=>'\n#ifdef GRS_APP_M1\n'+t+'\n#endif\n';
 const one=(a,b)=>{if(s.split(a).length!==2)throw Error('T529_BRIDGE_ANCHOR '+a);s=s.replace(a,b);};
 one('#include "foundation.h"','#include "g9spf/candidate_v2.h"\n#include "g9spf/controller.h"\n#include "g9spf/scene.h"\n#include "foundation.h"');
 one('struct App : BridgeState {','struct App : BridgeState {\n'+block(f('bridge_fields.inc')));
 one('void RevokeLocked(const char* why){','void RevokeLocked(const char* why){if(g9Controller)g9Controller->Stop();');
 one('void ObserveControlLocked(uint64_t stamp){','void ObserveControlLocked(uint64_t stamp){if(g9Selected)return;');
 one('void loop(){',block('void InvalidateG9Locked(){if(g9Controller)g9Controller->Stop();}')+'\nvoid loop(){');
 one('void InvalidateLocked(){if(m3Controller)m3Controller->Stop();','void InvalidateLocked(){if(g9Controller)g9Controller->Stop();if(m3Controller)m3Controller->Stop();');
 one('active=value;','active=value;'+block('if(g9Controller)g9Controller->Stop();'));
 one('a.K=uint32_t(k);a.Kw=uint32_t(kw);','if(a.g9Controller)a.g9Controller->Stop();a.g9Selected=false;a.K=uint32_t(k);a.Kw=uint32_t(kw);');
 one('a.m3Selected=true;','if(a.g9Controller&&a.g9Controller.use_count()>2)return text(e,\"G9_RETIRE_READER_BUSY\");a.g9Selected=false;a.m3Selected=true;');
 one('std::unique_ptr<AppTransport> gpu;std::unique_ptr<AppRun> run;AppScene scene;','std::unique_ptr<AppTransport> gpu;std::unique_ptr<AppRun> run;AppScene scene;'+block('std::shared_ptr<grs::g9spf::Controller> g9Local;grs::g9spf::Scene g9Scene;uint64_t g9LastDisplay=0,g9LastDraw=0;bool g9Working=false;'));
 one('const bool runnable=','const bool runnable=(g9Selected&&g9Working&&!failed)||');
 one('run.reset();if(gpu)gpu->ContextLost();',block('if(g9Local){g9Local->Stop();g9Local->Release(true);g9Local.reset();g9Scene.Reset(true);std::lock_guard<std::mutex> lock(mutex);g9Controller.reset();}')+'\n run.reset();if(gpu)gpu->ContextLost();');
 one('||(!gpu&&!m3Local&&enabled&&!failed&&!resetBlocked)','||(!gpu&&!m3Local&&!g9Local&&enabled&&!failed&&!resetBlocked)');
 one('transportEpoch=seen;transportIdentity=id;',block(f('bridge_create.inc'))+'\n transportEpoch=seen;transportIdentity=id;');
 one('   if(run&&!failed&&!gap){',block(f('bridge_pump.inc'))+'\n   if(run&&!failed&&!gap){');
 one('napi_value init(napi_env e,napi_value out){',block(f('napi.inc'))+'\nnapi_value init(napi_env e,napi_value out){');
 const withWords=['g9ResearchConfig64','g9Configure64','g9Configure','g9Pulse','g9Target','g9OfferModel','g9ColdBegin','g9ColdState','g9ColdPage','g9ColdEnd','g9ResearchConfig'];
 const noArgs=['g9Session','g9Quota','g9ColdCapabilities'];
 one('napi_property_descriptor d[]={','napi_property_descriptor d[]={\n'+block([...withWords,...noArgs].map(n=>' {"'+n+'",nullptr,'+n+',nullptr,nullptr,nullptr,napi_default,nullptr},').join('\n')));
 for(const name of ['m3ColdCapabilities','m3ColdBegin','m3ColdState','m3ColdPage','m3ColdEnd','coldCapabilities','coldTraceBeginV1','coldTraceStateV1','coldTracePageV1','coldTraceEndV1','executionDiagnosticsV1']){
  const at=s.indexOf('napi_value '+name+'('),brace=s.indexOf('{',at);if(at<0||brace<0)throw Error('T529_COLD_ANCHOR '+name);
  s=s.slice(0,brace+1)+block('if(app().g9Selected)return text(e,"{\\\"state\\\":\\\"UNSUPPORTED_COLD_REVISION\\\",\\\"required\\\":\\\"G9_SPF_COLD_V1\\\"}");')+s.slice(brace+1);
 }
 fs.writeFileSync(file,s);
 const types=path.join(main,'cpp/types/libgrsm1/index.d.ts');fs.appendFileSync(types,'\n'+withWords.map(n=>'export const '+n+':(words:Uint32Array)=>string;').join('\n')+'\n'+noArgs.map(n=>'export const '+n+':()=>string;').join('\n')+'\n');
 // T550 (problem T546 §5): strict decode contract same-source injection.
 require('../t550_words_contract_overlay.cjs')(main,process.cwd());
 require('../t582/overlay.cjs')(main);
 require('../t572/t572_touch_overlay.cjs')(main);
};
