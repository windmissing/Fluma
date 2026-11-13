const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]),route=process.argv[3]||'m2';
if(!['m1','m2'].includes(route))throw Error('ROUTE');
cp.execFileSync(process.execPath,[require.resolve(route==='m2'?'../t475/generate.cjs':'../t467/generate.cjs'),out,...(route==='m1'?['experience']:[])],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident');
function one(s,a,b){if(s.split(a).length!==2)throw Error('T485_ANCHOR '+a);return s.replace(a,b);}
function edit(rel,fn){const p=path.join(main,rel);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
const hash=bytes=>crypto.createHash('sha256').update(bytes).digest('hex');
if(route==='m1'){
 const source=fs.readFileSync(require.resolve('../t475/generate.cjs'),'utf8');
 const body=source.slice(source.indexOf('const cache ='),source.indexOf("edit('cpp/gpu_resident/app_bridge.cpp', s => {"))
  .replace("['m1', 'm2route']","['m1']");
 new Function('fs','path','base','main','edit','one','hash',body)(fs,path,base,main,edit,one,hash);
 edit('cpp/gpu_resident/app_bridge.cpp',s=>{
  s='#include "program_cache/program_factory.h"\n'+s;
  s=one(s,'napi_value setProgressV2(',fs.readFileSync('test/t475/napi.inc','utf8')+'\nnapi_value setProgressV2(');
  return one(s,'{"setSource",nullptr,sourceMode','{"programCacheState",nullptr,programCacheState,nullptr,nullptr,nullptr,napi_default,nullptr},{"configureProgramCache",nullptr,configureProgramCache,nullptr,nullptr,nullptr,napi_default,nullptr},{"setSource",nullptr,sourceMode');
 });
 edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const configureProgramCache:(filesDir:string,packageSha?:string)=>string;\nexport const programCacheState:()=>string;\n');
 edit('ets/entryability/EntryAbility.ets',s=>one(s,"    AppStorage.setOrCreate('gpuResidentM1',","    grsm1.configureProgramCache(this.context.filesDir,String(want.parameters?.gpusha??''));\n    AppStorage.setOrCreate('gpuResidentM1',"));
}
for(const name of fs.readdirSync('entry/src/main/cpp/gpu_resident/program_cache'))
 fs.copyFileSync('entry/src/main/cpp/gpu_resident/program_cache/'+name,path.join(base,'program_cache',name));
for(const target of ['m1',...(route==='m2'?['m2route']:[])])for(const name of ['scene.cpp','scene.h']){
 let source=fs.readFileSync('entry/src/main/cpp/gpu_resident/m1/'+name,'utf8').replaceAll('\r\n','\n');
 if(target==='m2route')source=source.replaceAll('namespace m1 {','namespace m2route {')
  .replace('GRS_GPU_SCENE_Q20_M1_V2','GRS_GPU_SCENE_Q20_M2_V3')
  .replace('header[1]!=2||header[31]!=3','header[1]!=3||header[31]!=7')
  .replace('r.requestedToken=Token(lease,f,optics);','r.requestedToken=Token(lease,f,optics)+"|ROOT|"+source.Owner().RootToken();');
 fs.writeFileSync(path.join(base,target,name),source);
}
edit('cpp/gpu_resident/app_bridge.cpp',s=>one(s,'run=std::make_unique<AppRun>(*gpu);bool committed=false;',`
    #ifdef GRS_APP_M1
    struct SceneCandidate{App* app;uint64_t epoch;void* window;};
    SceneCandidate candidate{this,seen,held};
    const auto sceneCurrent=[](void* value){
     const auto& c=*static_cast<SceneCandidate*>(value);
     std::lock_guard<std::mutex> lock(c.app->mutex);
     return c.app->CurrentLocked(c.epoch,c.window)&&c.app->active&&!c.app->requiresReset;
    };
    #ifdef GRS_T456_BOOT
    bootNote(seen,"SCENE_PROGRAM_PREPARE_ENTER");
    #endif
    if(!scene.PreparePrograms(seen,sceneCurrent,&candidate)){
     std::ostringstream failure;failure<<"{\\"state\\":\\"FAULT\\",\\"stage\\":\\"SCENE_PROGRAM_CREATE\\",\\"detail\\":"
      <<programcache::Quote(scene.PrepareFailure())<<",\\"prepareWallNs\\":\\""<<scene.PrepareWallNs()<<"\\"}";
     publish(failure.str());failed=true;retire();continue;
    }
    #ifdef GRS_T456_BOOT
    bootNote(seen,"SCENE_PROGRAM_PREPARE_EXIT",false,false,0,0,0,0,scene.PrepareWallNs());
    #endif
    #endif
    run=std::make_unique<AppRun>(*gpu);bool committed=false;`));
const manifest={revision:'T485_SCENE_CACHE_V1',route,base:'T475+T482+T483+T477 fixes',files:{}};
for(const name of ['program_cache/graphics_program_factory.h','program_cache/program_factory.h','m1/scene.cpp','m1/scene.h','app_bridge.cpp'])
 manifest.files[name]=hash(fs.readFileSync(path.join(base,name)));
if(route==='m2')manifest.files['m2route/scene.cpp']=hash(fs.readFileSync(path.join(base,'m2route/scene.cpp')));
fs.writeFileSync(path.join(out,'t485-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
