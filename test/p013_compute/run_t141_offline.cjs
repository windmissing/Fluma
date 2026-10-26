'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),cp=require('node:child_process');
const ROOT=path.resolve('.'),OUT=path.join(ROOT,'test/reports/t141'),TMP=path.join(ROOT,'.codex/local-test/t141');
fs.mkdirSync(OUT,{recursive:true});fs.mkdirSync(TMP,{recursive:true});
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
function call(bin,args,opts={}){return cp.execFileSync(bin,args,{cwd:ROOT,encoding:'utf8',timeout:600000,...opts});}
const pack=call('node',['test/p013_compute/tools/pack_t140_fixtures.cjs']);
const variant=call('node',['test/p013_compute/tools/generate_hot_test_variant.cjs']);
const patch=call('node',['test/p013_compute/run_variant_patch.cjs']);
if(!patch.includes('passed":12')||!patch.includes('selector calls 7 PASS'))throw new Error('HOT_VARIANT_REGRESSION');
const build=call('cmd',['/c','test\\p013_compute\\build_t141.cmd']);
if(!build.includes('BUILD_OK'))throw new Error('NATIVE_BUILD_NOT_CONFIRMED');
fs.writeFileSync(path.join(OUT,'build.txt'),build);
const exe=path.join(ROOT,'.codex/local-test/t80-matrix/t141_pair.exe');
const raw=path.join(OUT,'offline.json');if(fs.existsSync(raw))fs.unlinkSync(raw);
const env={...process.env,PATH:'C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;'+process.env.PATH};
const run=call(exe,[raw,path.join(TMP,'t140-192.bin'),path.join(TMP,'t140-512.bin')],{env});
fs.writeFileSync(path.join(OUT,'run.txt'),run);
const data=JSON.parse(fs.readFileSync(raw));if(!data.allOfflineChecks||data.runs.length!==2)throw new Error('OFFLINE_CHECK_FAILED');
for(const row of data.runs){if(!row.rangeOracle||!row.ownerProtocol||row.K!==(row.Nmax===192?256:512))
  throw new Error(`RANGE_OR_OWNER_${row.Nmax}`);
 if(row.gpuExecution==='OK'&&(row.pairOracle!=='AGREE'||row.dispatchCount!==(row.Nmax===192?11:12)))
  throw new Error(`GPU_CHAIN_${row.Nmax}`);}
const names=['pair_wire_v1.h','pair_sha256_v1.h','pair_prepare_v1.h','pair_prepare_v1.cpp',
  'pair_executor_v1.h','pair_executor_v1.cpp','pair_owner_v1.h','pair_owner_v1.cpp',
  'glsl/pair_build_v1.comp','glsl/pair_query_v1.comp','glsl/pair_reduce_v1.comp',
  'pair_driver_v1.cpp','pair_binding_v1.cpp','ets/T141NativePairBinding.ets',
  'CMakeLists.txt','build_t141.cmd','tools/generate_hot_test_variant.cjs',
  'tools/pack_t140_fixtures.cjs','run_variant_patch.cjs','run_t141_offline.cjs'];
const files=names.map(name=>{const b=fs.readFileSync(path.join(ROOT,'test/p013_compute',name));
  return {name,byteLength:b.length,sha256:sha(b)};});
const manifest={revision:'T141_OFFLINE_MANIFEST_V1',buildFlags:'/fp:strict /std:c++17',
  algorithm:'GPU_SEGMENTED_ID_PAIR_V1',wire:'P011_GPU_PAIR_WIRE_V1',
  t140ManifestSha256:sha(fs.readFileSync('test/reports/t140/p140_manifest.json')),
  t140FixturePoolSha:{'192':'0d3e9b6de5c22ba6f5c256aff46ab605524cd222b486c977df2b5b6451f4d720',
    '512':'d7239a09e99e80697c9a571bf2aef22d6c7eb40248266a89b7c95fb5a53b5d2f'},
  gpuExecution:data.runs.map(x=>({Nmax:x.Nmax,status:x.gpuExecution,dispatchCount:x.dispatchCount})),
  shaderCompileStatus:data.runs.every(x=>x.gpuExecution==='OK')?'GL_EXECUTED':'NOT_RUN_GLES31_UNAVAILABLE',
  nativeExecutable:{byteLength:fs.statSync(exe).size,sha256:sha(fs.readFileSync(exe))},
  harmonyNativeModule:{byteLength:fs.statSync(path.join(TMP,'libp011_gpu_pair_prototype.so')).size,
    sha256:sha(fs.readFileSync(path.join(TMP,'libp011_gpu_pair_prototype.so')))},files};
fs.writeFileSync(path.join(OUT,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(pack.trim());console.log(variant.trim());console.log(patch.split('\n').at(-2));
console.log(`T141 offline PASS; GPU ${manifest.shaderCompileStatus}`);
