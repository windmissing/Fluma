const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t452/generate.cjs'),out],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident');
function edit(p,f){const file=path.join(main,p);fs.writeFileSync(file,f(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T468_ANCHOR '+a);return s.replace(a,b);}
const rel=n=>'cpp/gpu_resident/m2route/'+n;
fs.copyFileSync('entry/src/main/cpp/gpu_resident/m2/create_diagnostics.h',path.join(base,'m2/create_diagnostics.h'));
fs.copyFileSync(path.join(__dirname,'uniform_locations_v3.h'),path.join(base,'m2route/uniform_locations_v3.h'));
const sha=n=>crypto.createHash('sha256').update(fs.readFileSync(path.join(out,n+'-v3.glsl'))).digest('hex');
fs.writeFileSync(path.join(base,'m2route/create_shader_identity.h'),`#pragma once\nnamespace grs::m2route {inline constexpr char kSourceActualSha[]="${sha('source')}";inline constexpr char kFinalActualSha[]="${sha('quantum')}";}\n`);
for(const h of ['source_transport.h','quantum_candidate.h'])edit(rel(h),s=>one(s,'#include "../uniform_locations.h"','#include "uniform_locations_v3.h"'));
for(const [name,object,source,hash,error,program] of [
 ['source_transport.cpp',0,'shader','kSourceActualSha','stats.firstGlError','program'],
 ['quantum_candidate.cpp',1,'QuantumKernel()','QuantumSha()','firstGlError','program'],
 ['full_transport.cpp',2,'QuantumCommitKernel()','kFinalActualSha','stats.firstGlError','quantum.program']]){
 edit(rel(name),s=>{
  s='#include "create_shader_identity.h"\n#include "../m2/create_diagnostics.h"\n'+s;
  const begin=s.indexOf(name==='full_transport.cpp'?'GLuint sh=glCreateShader':'auto sh=glCreateShader');
  const end=s.indexOf('\n SubmitScope resources',begin);
  if(begin<0||end<0)throw Error('T468_COMPILE_CHAIN '+name);
  s=s.slice(0,begin)+`if(!create468::Build(${object},${source},${hash},${program},${error},submitObserver)){${name==='source_transport.cpp'?'return fail(TransportCode::SHADER_FAILED);':'Release();return TransportCode::SHADER_FAILED;'}}`+s.slice(end);
  // A consumed original GL error is restored at the original decision sites.
  if(name==='source_transport.cpp'){
   s=one(s,'auto e=glGetError();if(e&&stats.firstGlError==0)','auto e=create468::Consume(glGetError());if(e&&stats.firstGlError==0)');
   s=one(s,'glGenBuffers(1,&b);glBindBuffer', 'create468::Enter(create468::Stage::ALLOCATE,0,n*4);glGenBuffers(1,&b);glBindBuffer');
   s=one(s,'stats.allocatedBytes+=n*4;return b!=0&&Good();','stats.allocatedBytes+=n*4;auto error=glGetError();create468::Exit(b!=0&&error==0,error);return b!=0&&Good();');
  }else if(name==='quantum_candidate.cpp'){
   s=one(s,'auto alloc=[&](GLuint& b,uint32_t n){glGenBuffers(1,&b);','auto alloc=[&](GLuint& b,uint32_t n){create468::Enter(create468::Stage::ALLOCATE,0,uint64_t(n)*4);glGenBuffers(1,&b);');
   s=one(s,'allocated+=uint64_t(n)*4;};','allocated+=uint64_t(n)*4;const auto error=glGetError();create468::Exit(b!=0&&error==0,error);};');
   s=one(s,'if(glGetError()!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}','if(create468::Consume(glGetError())!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}');
  }else{
   s=one(s,' if(m2Capability!=grs::m2::ProbeCode::OK)', ' create468::ProbeResult(uint32_t(m2Capability),m2Limits);\n if(m2Capability!=grs::m2::ProbeCode::OK)');
   s=one(s,'SubmitScope resources(submitObserver,SubmitStage::ResourceEnter,SubmitStage::ResourceExit);','create468::Enter(create468::Stage::ALLOCATE);SubmitScope resources(submitObserver,SubmitStage::ResourceEnter,SubmitStage::ResourceExit);');
   s=one(s,'if(glGetError()!=GL_NO_ERROR){Release();return TransportCode::MEMORY_PROFILE_UNSUPPORTED;}', 'const auto allocationError=create468::Consume(glGetError());create468::Exit(allocationError==0,allocationError);if(allocationError!=GL_NO_ERROR){Release();return TransportCode::MEMORY_PROFILE_UNSUPPORTED;}');
  }
  s=s.replaceAll('PrepareProgram(', 'create468::Prepare(');
  return s;
 });
}
edit('cpp/gpu_resident/m2/capability.h',s=>one('#include \"create_diagnostics.h\"\n'+s,'limits.glError=glGetError();','limits.glError=grs::create468::Consume(glGetError());'));
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s='#include "m2/create_diagnostics.h"\n'+s;
 s=one(s,'    auto code=gpu->Create(capacity,waterCapacity,id,m2route::QuantumParameters{},16u*1048576u,cold);',`    create468::Scope createScope(seen,reinterpret_cast<uintptr_t>(held),BridgeTid(),reinterpret_cast<uintptr_t>(eglGetCurrentContext()),id,capacity,waterCapacity);
    create468::QueryContext();
    auto code=gpu->Create(capacity,waterCapacity,id,m2route::QuantumParameters{},16u*1048576u,cold);createScope.Finish(uint32_t(code));`);
 const api=fs.readFileSync(path.join(__dirname,'napi.inc'),'utf8');
 s=one(s,'napi_value setProgressV2(',api+'\nnapi_value setProgressV2(');
 return one(s,'{"setSource",nullptr,sourceMode','{"m2CreateDiagnostics",nullptr,m2CreateDiagnostics,nullptr,nullptr,nullptr,napi_default,nullptr},{"m2CreateLog",nullptr,m2CreateLog,nullptr,nullptr,nullptr,napi_default,nullptr},{"setSource",nullptr,sourceMode');
});
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const m2CreateDiagnostics:(serial?:string)=>string;\nexport const m2CreateLog:(serial:string,object:number)=>string;\n');
fs.copyFileSync(path.join(__dirname,'M2CreateColdFiles.ets'),path.join(main,'ets/physics/M2CreateColdFiles.ets'));
fs.copyFileSync(path.join(__dirname,'M2CreatePackage.ets'),path.join(main,'ets/physics/M2CreatePackage.ets'));
edit('ets/entryability/EntryAbility.ets',s=>one(s,"import grsm1 from 'libgrsm1.so';", "import grsm1 from 'libgrsm1.so';\nimport { initializeM2Package } from '../physics/M2CreatePackage';").replace("    AppStorage.setOrCreate('gpuResidentM1',", "    initializeM2Package(want.parameters?.gpusha,want.parameters?.gpuelfsha);\n    AppStorage.setOrCreate('gpuResidentM1',"));
edit('ets/pages/GpuSandWaterExperience.ets',s=>{
 s="import { M2CreateColdFiles } from '../physics/M2CreateColdFiles';\nimport { m2PackageDeclaration } from '../physics/M2CreatePackage';\n"+s;
 s=one(s,'interface M2TraceMeta', 'interface M2CreateSnapshot {revision:string;bootSerial:string;previousSerial:string;}\ninterface M2CreateLog {state:string;bootSerial:string;object:number;hex:string;}\ninterface M2TraceMeta');
 s=one(s,'  build() {',fs.readFileSync(path.join(__dirname,'page.inc'),'utf8')+'\n  build() {');
 return one(s,"      Text(this.operation)","      Button('导出 M2 创建原始诊断').onClick(()=>this.exportCreateDiagnostic())\n      Button('导出上一创建诊断').onClick(()=>this.exportCreateDiagnostic(true))\n      Text(this.operation)");
});
fs.writeFileSync(path.join(out,'t468-manifest.json'),JSON.stringify({schema:'T462_M2_CREATE_DIAGNOSTIC_V1',base:'911c9ce',sourceSha:sha('source'),finalSha:sha('quantum'),kernelWireChanged:false,device:'NOT_RUN'},null,2));
