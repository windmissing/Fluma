'use strict';
const fs = require('node:fs');
const path = require('node:path');
const cp = require('node:child_process');
const crypto = require('node:crypto');
const {applyFirstFrameControl} = require('./first_frame_control.cjs');
const {applySessionControl} = require('./session_control.cjs');
const root = path.resolve(__dirname, '../..');
const local = path.join(root, '.codex/local-test');
const [route, directory, control] = process.argv.slice(2);
if (!['M1', 'M2'].includes(route) || !directory ||
    (control !== undefined && control !== '--first-frame-control' && !(route==='M1'&&control==='--session-control')) ||
    process.argv.length > 5)
  throw Error('Usage: node tools/t701/prepare.cjs M1|M2 <new .codex/local-test directory> [--first-frame-control | --session-control (M1)]');
const sessionControl = control === '--session-control';
const firstFrameControl = control === '--first-frame-control' || sessionControl;
const output = path.resolve(directory);
if (!output.startsWith(local + path.sep) || fs.existsSync(output))
  throw Error('T701_FRESH_ISOLATED_DIRECTORY_REQUIRED');
process.chdir(root);
const run = (exe, args) => cp.execFileSync(exe, args, {stdio: 'inherit'});
const git = (...args) => cp.execFileSync('git', args, {encoding: 'utf8'}).trim();
const sha = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const read = file => fs.readFileSync(file, 'utf8').replaceAll('\r\n', '\n');
const writeJson = (file, data) => fs.writeFileSync(file, JSON.stringify(data, null, 2) + '\n');
const parent = path.join(output, 'parent');
const m2 = route === 'M2';
const folder = m2 ? 'm2route' : 'm1';
const baseline = git('rev-parse', '620b114c');
const inputCommit = git('rev-parse', 'HEAD');
const checkoutBytes = {};
fs.mkdirSync(output, {recursive: true});
if (m2) {
  run(process.execPath, [path.join(root, 'tools/m2/prepare_cursor_build.cjs'), parent]);
} else {
  fs.mkdirSync(parent);
  const archive = path.join(output, 'parent.tar');
  run('git', ['archive', '-o', archive, baseline, 'AppScope', 'entry', 'hvigor',
    'docs', 'tools', 'build-profile.json5', 'hvigorfile.ts', 'oh-package.json5',
    'oh-package-lock.json5','test/t566_generate_composite.cjs','test/t576_generate_pipeline.cjs']);
  run('tar', ['-xf', archive, '-C', parent]);
  fs.unlinkSync(archive);
  // T647's shipped table binds hashes of the Windows checkout's raw protocol
  // bytes. Reproduce that checkout format only for those four exact inputs.
  for (const name of ['architecture-T554-g9-dm-manifest-v2.json',
    'architecture-T554-g9-dm-dispatch-v2.json','architecture-T647-acceptance-v1.json',
    'architecture-T552-g9-seed-manifest-v2.json']) {
    const rel = 'docs/' + name;
    const frozen = path.join(parent,rel);
    const checkedOut = path.join(root,rel);
    if (read(frozen) !== read(checkedOut)) throw Error('T701_PROTOCOL_PARENT_DRIFT ' + rel);
    fs.copyFileSync(checkedOut,frozen);
    checkoutBytes[rel] = sha(fs.readFileSync(frozen));
  }
  const g11 = JSON.parse(read(path.join(parent,'docs/architecture-T660-g11-manifest-v2.json')));
  for (const input of [...g11.inputs,g11.visualSeedRecipe]) {
    const frozen = path.join(parent,input.path);
    if (!fs.existsSync(frozen)) {
      const approved = fs.readFileSync(path.join(root,input.path));
      if (approved.length !== input.bytes || sha(approved) !== input.sha256)
        throw Error('T701_G11_PINNED_DEPENDENCY_CHANGED ' + input.path);
      fs.writeFileSync(frozen,approved);
      checkoutBytes[input.path] = input.sha256;
      continue;
    }
    const raw = fs.readFileSync(frozen);
    if (sha(raw) === input.sha256) continue;
    const windows = Buffer.from(read(frozen).replaceAll('\n','\r\n'));
    if (windows.length !== input.bytes || sha(windows) !== input.sha256)
      throw Error('T701_G11_PROTOCOL_PARENT_DRIFT ' + input.path);
    fs.writeFileSync(frozen,windows);
    checkoutBytes[input.path] = input.sha256;
  }
  const g12 = path.join(parent,'entry/src/main/cpp/gpu_resident/g12water/kernels.cmake');
  const cmake = one(read(g12),'../../../test/t576_generate_pipeline.cjs',
    '../../../../../test/t576_generate_pipeline.cjs');
  fs.writeFileSync(g12,one(cmake,'if(EXISTS "${G12WATER_GEN}")',
    'find_program(T701_G12_NODE NAMES node node.exe REQUIRED)\nif(EXISTS "${G12WATER_GEN}")')
    .replace('"C:/Program Files/nodejs/node.exe"','"${T701_G12_NODE}"'));
  checkoutBytes['entry/src/main/cpp/gpu_resident/g12water/kernels.cmake'] = sha(fs.readFileSync(g12));
  const profile = path.join(parent, 'build-profile.json5');
  const config = JSON.parse(read(profile));
  delete config.app.signingConfigs;
  for (const product of config.app.products) delete product.signingConfig;
  writeJson(profile, config);
}
const sdkSettings = path.join(root, 'local.properties');
if (fs.existsSync(sdkSettings)) fs.copyFileSync(sdkSettings, path.join(parent, 'local.properties'));
const parentNative = path.join(parent, 'entry/src/main/cpp/gpu_resident');
const shaderSource = path.join(parentNative, folder, 'full_kernel.cpp');
let shaderBytes = fs.readFileSync(shaderSource);
if (!m2) {
  const sdkHome = process.env.OHOS_SDK_HOME ||
    read(sdkSettings).match(/^sdk\.dir=(.*)$/m)?.[1].replaceAll('\\:', ':');
  if (!sdkHome) throw Error('T701_OHOS_SDK_HOME_REQUIRED');
  const sdk = path.join(sdkHome,'26.0.0/native');
  const cmakeBin = path.join(sdk,'build-tools/cmake/bin');
  const configure = path.join(output,'parent-cmake');
  run(path.join(cmakeBin,'cmake.exe'),['-G','Ninja','-S',path.join(parent,'entry/src/main/cpp'),
    '-B',configure,'-DOHOS_ARCH=arm64-v8a','-DCMAKE_BUILD_TYPE=Debug',
    '-DCMAKE_MAKE_PROGRAM='+path.join(cmakeBin,'ninja.exe'),
    '-DCMAKE_TOOLCHAIN_FILE='+path.join(sdk,'build/cmake/ohos.toolchain.cmake')]);
  shaderBytes = fs.readFileSync(path.join(configure,'gpu_resident/full_m1_kernels.h'));
}
const parentShader = fs.readFileSync(shaderSource);
// C++ translation normalizes physical CRLF before evaluating raw literals.
// Bind the identities to the compiled GLSL bytes, including on Windows.
const kernels = Array.from(shaderBytes.toString().replaceAll('\r\n','\n')
  .matchAll(/R"([A-Za-z0-9_]*)\(([\s\S]*?)\)\1"/g), match => match[2]);
if (kernels.length !== 2) throw Error('T701_FULL_KERNEL_PAIR');
const canonical = kernels.map(sha);
const parentDigest = m2 ? sha(fs.readFileSync(path.join(parent,'t663-sources.json'))) : baseline;
const sourceFiles = ['quantum_candidate.cpp', 'quantum_candidate.h',
  'source_transport.cpp', 'source_transport.h', 'full_transport.cpp'];
const factory = path.join(root,'entry/src/main/cpp/gpu_resident/program_cache');
const factoryFiles = ['program_factory.h','artifact_store.h','program_observer.h'];
const evidence = [];

function one(body, anchor, replacement) {
  const index = body.indexOf(anchor);
  if (index === -1 || body.indexOf(anchor, index + anchor.length) !== -1)
    throw Error('T701_PARENT_DRIFT ' + anchor);
  return body.slice(0, index) + replacement + body.slice(index + anchor.length);
}
function between(body, start, end) {
  const begin = body.indexOf(start);
  const finish = body.indexOf(end, begin);
  if (begin < 0 || finish < 0) throw Error('T701_PARENT_SPAN ' + start);
  return body.slice(begin, finish);
}
function cacheBuild(role, source, shaderSha, program, error, generation, failure) {
  return `if(!programcache::Build(${role},${source},${shaderSha},manifestSha,${program},${error},submitObserver,${generation})){${failure}}`;
}
function instrumentPrepare(body, program) {
  const name = m2 ? 'create468::Prepare' : 'PrepareProgram';
  const call = between(body, name + '(' + program, ')!=GL_NO_ERROR') + ')';
  return one(body, call, `programcache::ObservePrepare(${program},[&]{return ${call};})`);
}
function ownership(body, generation) {
  body = one(body, 'if(program)glDeleteProgram(program);',
    `if(program){programcache::Forget(program,context,${generation});glDeleteProgram(program);}`);
  return one(body, '::ContextLost(){',
    `::ContextLost(){programcache::Forget(program,context,${generation});`);
}
function quantum(body, cached) {
  const start = 'TransportCode QuantumCandidate::Create(uint32_t k,uint32_t kw){';
  const original = between(body, start, '\nTransportCode QuantumCandidate::Enqueue');
  const build = between(original, m2 ? ' if(!create468::Build(1,' : ' auto sh=glCreateShader',
    '\n SubmitScope resources');
  const plan = original.slice(start.length, original.indexOf(build));
  const alloc = between(original, ' SubmitScope resources', 'if(!uniformLocations.Prepare(program))');
  const check = 'if(!uniformLocations.Prepare(program)){Release();return TransportCode::SHADER_FAILED;}';
  const reflect = cached ?
    'if(!(cacheGeneration ? programcache::Reflect(uniformLocations,program) : uniformLocations.Prepare(program))){Release();return TransportCode::SHADER_FAILED;}' : check;
  const select = cached ? ` if(generation) {
 ${cacheBuild(1,'QuantumKernel()','QuantumSha()','program','firstGlError','generation','Release();return TransportCode::SHADER_FAILED;')}
 } else {${build}
 }` : build;
  const replacement = `TransportCode QuantumCandidate::PlanCreate(uint32_t k,uint32_t kw){${plan}
 return TransportCode::OK;
}
TransportCode QuantumCandidate::BuildIntermediate(uint64_t generation){
 cacheGeneration=generation;
${select}
 return TransportCode::OK;
}
TransportCode QuantumCandidate::InitBaseResources(bool reflect){
${alloc}
 if(reflect){${reflect}}
 resources.End(0);
 BuildPassTable();
 return TransportCode::OK;
}
TransportCode QuantumCandidate::Create(uint32_t k,uint32_t kw){
 auto code=PlanCreate(k,kw);
 if(code!=TransportCode::OK)return code;
 code=BuildIntermediate(0);
 if(code!=TransportCode::OK)return code;
 return InitBaseResources(true);
}`;
  const result = one(body, original, replacement);
  return cached ? ownership(result,'cacheGeneration') : result;
}
function source(body, cached) {
  if (!cached) return body;
  const original = between(body, m2 ? ' if(!create468::Build(0,' : ' auto sh=glCreateShader',
    '\n SubmitScope resources');
  const generation = 't701::Generation(identity.context)';
  const failure = 'return fail(TransportCode::SHADER_FAILED);';
  // This translation unit also serves standalone SourceCoreTransport.
  body = one(body, 'const char* expectedSha,const char* shader){',
    'const char* expectedSha,const char* shader,bool fullScope,void* scope,bool(*valid)(void*)){');
  body = one(body, original, ` const auto scopeCurrent=[&]{return Current()&&(!valid||valid(scope));};
 const auto revoke=[&]{
  if(Current())Release();else ContextLost();
  return TransportCode::STATE_LOST;
 };
 if(fullScope){
  if(!scopeCurrent())return revoke();
  ${cacheBuild(0,'shader','t701::kSourceSha','program','stats.firstGlError',generation,failure)}
  if(!scopeCurrent())return revoke();
 } else {${original}
 }`);
  body = one(body,'uniformLocations.Prepare(program)','programcache::Reflect(uniformLocations,program)');
  body = instrumentPrepare(body, 'program');
  body = one(body, 'ready=true;return TransportCode::OK;', `
 if(fullScope){
  if(!scopeCurrent())return revoke();
  if(programcache::Prepared(program)!=GL_NO_ERROR)return fail(TransportCode::GL_FAILED);
  if(!scopeCurrent())return revoke();
 }
 ready=true;return TransportCode::OK;`);
  return ownership(body, generation);
}
function full(body, cached) {
  const init = 'c=quantum.Create(K,Kw);if(c!=TransportCode::OK){Release();return c;}';
  const retire = 'glDeleteProgram(quantum.program);quantum.program=0;';
  const original = between(body, m2 ? 'if(!create468::Build(2,' : 'GLuint sh=glCreateShader',
    m2 ? '\n create468::Enter(create468::Stage::ALLOCATE)' : '\n SubmitScope resources');
  const failure = 'Release();return TransportCode::SHADER_FAILED;';
  const build = cached ? cacheBuild(2,'QuantumCommitKernel()','t701::kFinalSha',
    'quantum.program','stats.firstGlError','generation',failure) : original;
  if (cached) body = one(body,'id,FullM1ShaderSha(),FullSourceKernel())',
    'id,FullM1ShaderSha(),FullSourceKernel(),true,preparation.context,preparation.current)');
  body = one(body, init + '\n ' + retire + original, `
 const auto generation=t701::Generation(id.context);
 const auto current=[&]{
  return quantum.Current()&&(!preparation.current||preparation.current(preparation.context));
 };
 const auto revoke=[&]{
  if(quantum.Current())Release();else ContextLost();
  return TransportCode::STATE_LOST;
 };
 c=quantum.PlanCreate(K,Kw);
 if(c!=TransportCode::OK){Release();return c;}
 if(!current())return revoke();
 if constexpr (!t701::kTwo){
  c=quantum.BuildIntermediate(generation);
  if(c==TransportCode::OK&&!current())return revoke();
  if(c==TransportCode::OK)c=quantum.InitBaseResources(true);
  if(c!=TransportCode::OK){Release();return c;}
  if(!current())return revoke();
  ${cached?'if(programcache::Prepared(quantum.program)!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}':''}
  ${cached?'programcache::Forget(quantum.program,quantum.context,generation);':''}
  ${retire}
 }else{
  quantum.cacheGeneration=generation;
  ${m2?'if(create468::active)create468::active->object=2;':''}
  c=quantum.InitBaseResources(false);
  if(c!=TransportCode::OK){Release();return c;}
 }
 if(!current())return revoke();
 ${build}
 if(!current())return revoke();`);
  body = one(body,'quantum.uniformLocations.Prepare(quantum.program)',
    't701::ReflectFinal(quantum.uniformLocations,quantum.program)');
  if (cached) {
    body = instrumentPrepare(body, 'quantum.program');
    body = one(body,' // T341:',' if(programcache::Prepared(quantum.program,false)!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}\n // T341:');
  }
  const end = 'if(prepared!=TransportCode::OK){stats.firstGlError=error;Release();return prepared;}';
  return one(body,end,` ${cached?'programcache::Warmup(quantum.program,start,start+warmupWallNs,error);':''}
 ${end}
 if(!current())return revoke();
 ${cached?`if(preparation.twoBatches||t701::ExportFence(current,start))
  programcache::ExportPrepared(quantum.program);
 if(!current())return revoke();`:''}`);
}
function bridge(body, cached) {
  const functions = fs.readFileSync(path.join(root,'test/t475/napi.inc'),'utf8');
  const marker = '{"setSource",nullptr,sourceMode';
  const binding = name => `{"${name}",nullptr,${name},nullptr,nullptr,nullptr,napi_default,nullptr},`;
  if (m2) body = one(body,'    create468::QueryContext();',
    `    if constexpr(t701::kTwo){
     auto& recorder=createScope.recorder;
     recorder.store.Change(recorder.serial,[](create468::Attempt& row){
      row.programs[1].name="NOT_USED_BY_VARIANT";
     });
    }
    create468::QueryContext();`);
  if (cached) body = one(body,'napi_value setProgressV2(',functions+'\nnapi_value setProgressV2(');
  body = one(body,'napi_value setProgressV2(',`napi_value fullComputeVariant(napi_env e,napi_callback_info){
 return text(e,t701::kDeclaration);
}
napi_value setProgressV2(`);
  return '#include "m1/t701_variant.h"\n' +
    (cached?'#include "program_cache/program_factory.h"\n':'') +
    one(body,marker,binding('fullComputeVariant')+
        (cached ? binding('configureProgramCache')+binding('programCacheState') : '')+marker);
}

for (const variant of ['THREE','TWO']) for (const cache of ['OFF','ON']) {
  const arm = variant + '_' + cache;
  if (firstFrameControl && arm !== (m2 ? 'TWO_ON' : 'THREE_OFF')) continue;
  const cached = cache === 'ON';
  const dest = path.join(output, arm);
  fs.cpSync(parent, dest, {recursive:true, filter:file=>path.basename(file)!=='oh_modules'});
  const modules = path.join(root,'oh_modules');
  if(fs.existsSync(modules)) fs.symlinkSync(modules,path.join(dest,'oh_modules'),'junction');
  const main = path.join(dest,'entry/src/main');
  const native = path.join(main,'cpp/gpu_resident');
  const changes = {};
  function edit(relative, transform) {
    const file = path.join(main,relative);
    const before = fs.readFileSync(file);
    const after = transform(read(file));
    fs.writeFileSync(file,after);
    changes[relative] = {before:changes[relative]?.before ?? sha(before),after:sha(after)};
  }
  const declaration = {route,variant,cache,
    roles:variant==='THREE'?[0,1,2]:[0,2],
    role1:variant==='THREE'?'INTERMEDIATE_Q':'NOT_USED_BY_VARIANT',
    independentQ:'SOURCE_BUILD', sourceSha:canonical[0], finalSha:canonical[1]};
  if (firstFrameControl)
    declaration.firstFrameControl = m2 ? 'T714_M2_DIRECT_ENTRY' : 'T714_M1_CLOCK_V2';
  if (sessionControl)
    declaration.sessionControl = 'T717_M1_SESSION_EVIDENCE';
  const identity = `#pragma once
namespace grs::t701 {
inline constexpr bool kTwo = ${variant==='TWO'};
inline constexpr bool kCache = ${cached};
inline constexpr char kSourceSha[] = "${canonical[0]}";
inline constexpr char kFinalSha[] = "${canonical[1]}";
inline constexpr char kDeclaration[] = R"T701(${JSON.stringify(declaration)})T701";
}
`;
  for (const dir of new Set(['m1',folder]))
    fs.writeFileSync(path.join(native,dir,'t701_variant.h'),identity);
  if (!fs.existsSync(path.join(native,'program_cache'))) fs.mkdirSync(path.join(native,'program_cache'));
  for (const file of factoryFiles) fs.copyFileSync(path.join(factory,file),path.join(native,'program_cache',file));
  fs.copyFileSync(path.join(__dirname,'full_compute.h'),path.join(native,folder,'t701_full_compute.h'));
  const prefix = 'cpp/gpu_resident/'+folder+'/';
  for (const name of sourceFiles) {
    const transform = {'quantum_candidate.cpp':quantum,'source_transport.cpp':source,
      'source_transport.h':body=>one(body,'const char* expectedSha,const char* shader);',
        'const char* expectedSha,const char* shader,bool fullScope=false,void* scope=nullptr,bool(*valid)(void*)=nullptr);'),
      'full_transport.cpp':full,'quantum_candidate.h':body=>one(body,' friend class FullM1Transport;',
      ' friend class FullM1Transport;\n TransportCode PlanCreate(uint32_t,uint32_t);\n TransportCode InitBaseResources(bool);\n TransportCode BuildIntermediate(uint64_t);\n uint64_t cacheGeneration=0;')}[name];
    if (name.startsWith('source_transport.') && !cached) continue;
    edit(prefix+name,body=>(name.endsWith('.cpp')?'#include "t701_full_compute.h"\n':'')+transform(body,cached));
  }
  edit('cpp/gpu_resident/app_bridge.cpp',body=>bridge(body,cached));
  edit('cpp/types/libgrsm1/index.d.ts',body=>body+
    '\nexport const fullComputeVariant:()=>string;\n'+(cached?
    'export const configureProgramCache:(filesDir:string, packageSha?:string)=>string;\nexport const programCacheState:()=>string;\n':''));
  if (cached) edit('ets/entryability/EntryAbility.ets',body=>one(body,
    '    AppStorage.setOrCreate(\'gpuResidentM1\',',
    "    grsm1.configureProgramCache(this.context.filesDir,String(want.parameters?.gpusha??''));\n    AppStorage.setOrCreate('gpuResidentM1',"));
  // CMake receives the same fixed two enums as every affected native TU.
  edit('cpp/CMakeLists.txt',body=>body+`
set(GRS_FULL_CREATE_VARIANT "${variant}" CACHE STRING "T701 fixed creation variant")
set(GRS_FULL_COMPUTE_CACHE "${cache}" CACHE STRING "T701 fixed compute cache policy")
if(NOT GRS_FULL_CREATE_VARIANT STREQUAL "${variant}" OR NOT GRS_FULL_COMPUTE_CACHE STREQUAL "${cache}")
  message(FATAL_ERROR "T701_VARIANT_MISMATCH")
endif()
${!m2&&cached?'target_sources(grsm1 PRIVATE gpu_resident/m2/create_diagnostics_abi.cpp)':''}
`);
  if (firstFrameControl) applyFirstFrameControl(edit,m2);
  if (sessionControl) {
    fs.copyFileSync(path.join(__dirname,'first_frame_session.h'),path.join(native,'t717_session.h'));
    applySessionControl(edit);
  }
  if (sha(fs.readFileSync(path.join(native,folder,'full_kernel.cpp'))) !== sha(parentShader))
    throw Error('T701_SHADER_SOURCE_CHANGED');
  const armEvidence = {...declaration,parentDigest, changes,
    headerSHA256:sha(identity), fullKernelSHA256:sha(parentShader),
    factorySHA256:Object.fromEntries(factoryFiles.map(file=>[file,sha(fs.readFileSync(path.join(factory,file)))]))};
  writeJson(path.join(dest,'t701-arm.json'),armEvidence);
  evidence.push(armEvidence);
}
writeJson(path.join(output,'t701-generation.json'),{route,inputCommit,parentDigest,
  parent:m2?'T663 -> T468 -> T452 actual m2route':baseline,
  checkoutBytes, recipeSHA256:sha(fs.readFileSync(__filename)),
  firstFrameControlSHA256:firstFrameControl ? sha(fs.readFileSync(require.resolve('./first_frame_control.cjs'))) : undefined,
  sessionControlSHA256:sessionControl ? sha(fs.readFileSync(require.resolve('./session_control.cjs'))) : undefined,
  sessionHeaderSHA256:sessionControl ? sha(fs.readFileSync(path.join(__dirname,'first_frame_session.h'))) : undefined,
  arms:evidence});
console.log(output);
