const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]),route=process.argv[3];if(!['m1','m2'].includes(route))throw Error('ROUTE');
cp.execFileSync(process.execPath,[require.resolve('../t501/generate.cjs'),out,route],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident');
function edit(name,fn){const p=path.join(main,name);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
edit('cpp/gpu_resident/program_cache/artifact_store.h',s=>{
 const start=s.indexOf('  std::string Stats501(){'),end=s.indexOf('  size_t Count() {',start);if(start<0||end<start||!s.includes('Diagnostics502()'))throw Error('T502_STORE_BASE');
 return s.slice(0,start)+'  std::string Stats501(){return Diagnostics502();}\n'+s.slice(end);
});
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 const at=s.indexOf('napi_value programCacheState(');if(at<0)throw Error('T502_NAPI');
 s=s.slice(0,at)+'napi_value cacheStorageState(napi_env env,napi_callback_info){return text(env,programcache::Store().Diagnostics502());}\n'+s.slice(at);
 const marker='{"programCacheState",nullptr,programCacheState';if(s.split(marker).length!==2)throw Error('T502_EXPORT');
 return s.replace(marker,'{"cacheStorageState",nullptr,cacheStorageState,nullptr,nullptr,nullptr,napi_default,nullptr},'+marker);
});
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const cacheStorageState:()=>string;\n');
const manifest={revision:'T502_CACHE_TEMP_OVERLAY_V1',route,files:{}};
for(const n of ['app_bridge.cpp','program_cache/artifact_store.h','program_cache/program_factory.h','resource_ledger.h','resource_gl.h'])manifest.files[n]=crypto.createHash('sha256').update(fs.readFileSync(path.join(base,n))).digest('hex');
fs.writeFileSync(path.join(out,'t502-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
