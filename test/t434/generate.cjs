const fs=require('node:fs'),path=require('node:path');
const out=path.resolve(process.argv[2]);require('./frozen.cjs').freeze(out);
const cpp=path.join(out,'entry/src/main/cpp'),base=path.join(cpp,'gpu_resident');
for(const name of ['full_transport.h','full_transport.cpp','quantum_candidate.h','quantum_candidate.cpp'])fs.copyFileSync('entry/src/main/cpp/gpu_resident/m1/'+name,path.join(base,'m1',name));
fs.appendFileSync(path.join(cpp,'CMakeLists.txt'),'\n# T434 isolated comparison profile; no production default change.\noption(T434_EXPERIMENTAL_V2 "T434 comparison profile" ON)\nif(T434_EXPERIMENTAL_V2)\n target_compile_definitions(grsm1 PRIVATE GRS_PROGRESS_V2_EXPERIMENT)\nendif()\n');
const bridge=path.join(base,'app_bridge.cpp');let s=fs.readFileSync(bridge,'utf8').replaceAll('\r\n','\n');
function replace(a,b){if(s.split(a).length!==2)throw Error('T434_ANCHOR '+a);s=s.replace(a,b);}
replace('bool progressV2=false,nextProgressV2=false,v2ResourcesReady=false;',`bool progressV2=false,nextProgressV2=false,v2ResourcesReady=false;
 bool prefixNext=false;
 std::array<m1::CopyAccounting,128> copyCache{};size_t copyCount=0,copyStart=0;uint64_t copyDropped=0;
 void CacheCopiesLocked(const AppTransport& gpu){
  if(!gpu.CopySampleCount())return;const auto& s=gpu.CopySample(gpu.CopySampleCount()-1);
  if(copyCount){auto& last=copyCache[(copyStart+copyCount-1)%128];if(Compare64(last.root.nonce,s.root.nonce)==0&&Compare64(last.identity.run,s.identity.run)==0){last=s;return;}}
  const size_t at=(copyStart+copyCount)%128;if(copyCount==128){copyStart=(copyStart+1)%128;++copyDropped;}else ++copyCount;copyCache[at]=s;
 }
`);
replace('bool activeTailProbe=false,activeColdTwoBatches=false;','bool activeTailProbe=false,activeColdTwoBatches=false,activePrefix=false;');
replace('progressV2=nextProgressV2;','progressV2=nextProgressV2;activePrefix=prefixNext;copyStart=copyCount=0;copyDropped=0;');
replace('return scope.app.CurrentLocked(scope.epoch,scope.window);\n    }};','return scope.app.CurrentLocked(scope.epoch,scope.window);\n    },activePrefix};');
s=s.replaceAll('Mark(BridgeStage::PumpEnd,seen,last,0,int(code));','{std::lock_guard<std::mutex> l(mutex);CacheCopiesLocked(*gpu);}Mark(BridgeStage::PumpEnd,seen,last,0,int(code));');
replace('status=s;if(s.find', 'if(gpu)CacheCopiesLocked(*gpu);status=s;if(s.find');
// Cold serialization never invokes GL and reads only the owner-maintained bounded cache.
replace('napi_value setColdTwoBatches(',`napi_value setLedgerPrefix(napi_env e,napi_callback_info i){size_t n=1;napi_value v;bool on=false;if(napi_get_cb_info(e,i,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||napi_get_value_bool(e,v,&on)!=napi_ok)return text(e,"INVALID_LEDGER_ROUTE");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.prefixNext=on;return text(e,on?"LEDGER_PREFIX_NEXT_RUN":"LEDGER_FULL_NEXT_RUN");}
napi_value copyAccounting(napi_env e,napi_callback_info){auto& a=app();std::lock_guard<std::mutex> l(a.mutex);std::ostringstream o;
 o<<"{\\\"schema\\\":\\\"T434_COPY_ACCOUNTING_V1\\\",\\\"revision\\\":1,\\\"stateCopyMeaning\\\":\\\"GL_COPY_BUFFER_ONLY_SHADER_STORES_EXCLUDED\\\",\\\"dropped\\\":"<<a.copyDropped<<",\\\"rows\\\":[";
 for(size_t i=0;i<a.copyCount;++i){if(i)o<<',';const auto& s=a.copyCache[(a.copyStart+i)%128];o<<'{';
 auto u=[&](const char* key,U64 v){o<<'"'<<key<<"\\\":\\\""<<((uint64_t(v.hi)<<32)|v.lo)<<"\\\",";};
 u("run",s.identity.run);u("owner",s.identity.owner);u("context",s.identity.context);u("surface",s.identity.surface);u("geometry",s.identity.geometry);u("config",s.identity.config);u("nonce",s.root.nonce);u("control",s.root.control);u("frame",s.root.frame);
 auto h=[&](const char* k,const std::array<uint32_t,8>& a){o<<'"'<<k<<"\\\":[";for(size_t j=0;j<8;++j){if(j)o<<',';o<<a[j];}o<<"],";};h("shaderShaWords",s.identity.shaderSha);h("angleShaWords",s.identity.angleSha);h("layoutShaWords",s.identity.layoutSha);
 o<<"\\\"kind\\\":"<<s.root.kind<<",\\\"route\\\":"<<s.route<<",\\\"entries\\\":"<<s.entries<<",\\\"ledgerCopyBytes\\\":"<<s.ledgerCopyBytes<<",\\\"eventPrefixCopyBytes\\\":"<<s.eventPrefixCopyBytes<<",\\\"stateCopyBytes\\\":"<<s.stateCopyBytes<<",\\\"completionMappedBytes\\\":"<<s.completionMappedBytes<<",\\\"coldMappedBytes\\\":"<<s.coldMappedBytes<<'}';}
 o<<"]}";return text(e,o.str());}
napi_value setColdTwoBatches(`);
replace('{"setSource",nullptr,sourceMode', '{"setLedgerPrefix",nullptr,setLedgerPrefix,nullptr,nullptr,nullptr,napi_default,nullptr},{"copyAccounting",nullptr,copyAccounting,nullptr,nullptr,nullptr,napi_default,nullptr},{"setSource",nullptr,sourceMode');
s=s.replaceAll('CacheCopiesLocked(*gpu);','T434_CACHE(*gpu);');
s=s.replace('namespace {', '#ifdef GRS_APP_M1\n#define T434_CACHE(gpu) CacheCopiesLocked(gpu)\n#else\n#define T434_CACHE(gpu) ((void)0)\n#endif\nnamespace {');
fs.writeFileSync(bridge,s);
const types=path.join(cpp,'types/libgrsm1/index.d.ts');s=fs.readFileSync(types,'utf8');s+='\nexport const setLedgerPrefix:(enabled:boolean)=>string;\nexport const copyAccounting:()=>string;\n';fs.writeFileSync(types,s);
const page=path.join(out,'entry/src/main/ets/pages/GpuResidentM1.ets');s=fs.readFileSync(page,'utf8').replace("@State private progressV2:boolean=false;","@State private progressV2:boolean=false;@State private ledgerPrefix:boolean=false;");
s=s.replace("Button('导出原生时间线')",`Button(this.ledgerPrefix?'下轮来源账：前缀':'下轮来源账：完整').onClick(()=>{this.ledgerPrefix=!this.ledgerPrefix;this.coldStatus=m1.setLedgerPrefix(this.ledgerPrefix);});Button('导出复制计数').onClick(async()=>{const context=getContext(this) as common.UIAbilityContext;await new DiagnosticColdFiles(context.filesDir).save('t434-copy-accounting.json',m1.copyAccounting());});Button('导出原生时间线')`);fs.writeFileSync(page,s);
fs.writeFileSync(path.join(out,'t434-manifest.json'),JSON.stringify({schema:'T434_GENERATION_V1',base:'0228a7d',routeDefault:'FULL',select:'setLedgerPrefix then newRun; never hot',device:'NOT_RUN'},null,2));
console.log(out);
