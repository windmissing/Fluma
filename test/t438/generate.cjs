const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
const profile=process.argv[3];
if(!['regular','experiment'].includes(profile))throw Error('profile must be regular or experiment');
require('../t434/frozen.cjs').freeze(out);
const main=path.join(out,'entry/src/main');
function edit(rel,fn){const file=path.join(main,rel);fs.writeFileSync(file,fn(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')));}
function replaceOnce(s,needle,value){if(s.split(needle).length!==2)throw Error('T438_SOURCE_DRIFT '+needle);return s.replace(needle,value);}
for(const rel of ['ets/physics/P051GravityInputAdapter.ets']){
 fs.writeFileSync(path.join(main,rel),cp.execFileSync('git',['show','0228a7d:entry/src/main/'+rel]));
}
for(const page of ['Index','GpuSandWaterExperience'])fs.copyFileSync(path.join(__dirname,page+'.ets'),path.join(main,'ets/pages',page+'.ets'));
fs.copyFileSync(path.join(__dirname,'experience_capabilities.h'),path.join(main,'cpp/gpu_resident/m1/experience_capabilities.h'));
edit('ets/entryability/EntryAbility.ets',s=>replaceOnce(s,"'pages/P013P1G0Probe' : 'pages/SandCanvas';","'pages/P013P1G0Probe' : 'pages/Index';"));
edit('resources/base/profile/main_pages.json',s=>{const j=JSON.parse(s);j.src.push('pages/GpuSandWaterExperience');return JSON.stringify(j,null,2)+'\n';});
edit('cpp/CMakeLists.txt',s=>s+`
# T438 isolated build profile. Only grsm1 receives the experiment macro.
option(T438_M1_EXPERIMENT_V2 "Enable T438 M1 experimental progress clock" ${profile==='experiment'?'ON':'OFF'})
if(T438_M1_EXPERIMENT_V2)
  target_compile_definitions(grsm1 PRIVATE GRS_PROGRESS_V2_EXPERIMENT)
endif()
`);
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s=replaceOnce(s,'#include "m1/ui_state.h"','#include "m1/ui_state.h"\n#include "m1/experience_capabilities.h"');
 s=replaceOnce(s,'napi_value setProgressV2(',`napi_value experienceCapabilities(napi_env e,napi_callback_info){
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);
 return text(e,m1::ExperienceCapabilities(a.K,a.Kw));
}
napi_value setProgressV2(`);
 return replaceOnce(s,'{"setProgressV2",nullptr,setProgressV2','{"experienceCapabilities",nullptr,experienceCapabilities,nullptr,nullptr,nullptr,napi_default,nullptr},{"setProgressV2",nullptr,setProgressV2');
});
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const experienceCapabilities:()=>string;\n');
fs.writeFileSync(path.join(out,'t438-manifest.json'),JSON.stringify({schema:'T438_GENERATION_V1',base:'0228a7d',profile,menu:'pages/Index',explicitPage:'pages/GpuSandWaterExperience',device:'NOT_RUN'},null,2));
console.log(out);
