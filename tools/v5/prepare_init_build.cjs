// T653 actual generated App/ABC preparation; no product or test execution.
'use strict';
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]);
if(!out.startsWith(path.join(root,'.codex/local-test')+path.sep))throw Error('ISOLATED_OUTPUT_REQUIRED');
if(fs.existsSync(out))throw Error('FRESH_GENERATED_APP_REQUIRED');
cp.execFileSync(process.execPath,[path.join(root,'test/t517/generate.cjs'),out],{cwd:root,stdio:'inherit'});
function copy(rel){fs.cpSync(path.join(root,rel),path.join(out,rel),{recursive:true});}
for(const rel of ['entry/src/main/cpp/gpu_resident/program_cache/program_factory.h','entry/src/main/cpp/gpu_resident/program_cache/program_observer.h','entry/src/main/cpp/gpu_resident/program_cache/graphics_program_factory.h','entry/src/main/cpp/gpu_resident/napi_words_contract.h','entry/src/main/cpp/gpu_resident/napi_words_contract.cpp','entry/src/main/cpp/gpu_resident/napi_provider_profile.h','entry/src/main/ets/physics/GpuM5Init.ets','entry/src/main/ets/physics/GpuInputAttempt.ets','entry/src/main/ets/physics/DiagnosticColdFiles.ets','entry/src/main/ets/physics/GpuUtf8Budget.ets','entry/src/main/ets/physics/GpuBridgeTimelineCold.ets','tools/v5/verify_init_manifest.py'])copy(rel);
const cmake=path.join(out,'entry/src/main/cpp/CMakeLists.txt');let text=fs.readFileSync(cmake,'utf8');if(!text.includes('T653_INIT_MANIFEST'))text+='\n# T653_INIT_MANIFEST: reject stale generated App/helper sources.\nfind_package(Python3 REQUIRED COMPONENTS Interpreter)\nexecute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../tools/v5/verify_init_manifest.py" "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." RESULT_VARIABLE M5_MANIFEST_RESULT)\nif(NOT M5_MANIFEST_RESULT EQUAL 0)\n message(FATAL_ERROR "M5_INIT_MANIFEST_STALE")\nendif()\n';fs.writeFileSync(cmake,text);
const files={};function visit(p){for(const n of fs.readdirSync(p)){const f=path.join(p,n);if(fs.statSync(f).isDirectory())visit(f);else if(!['m5_source_manifest.h'].includes(n)){const rel=path.relative(out,f).split(path.sep).join('/');files[rel]=crypto.createHash('sha256').update(fs.readFileSync(f)).digest('hex');}}}
visit(path.join(out,'entry/src/main/cpp'));visit(path.join(out,'entry/src/main/ets'));
const sha=crypto.createHash('sha256').update(JSON.stringify(files)).digest('hex');
fs.writeFileSync(path.join(out,'m5-init-source-manifest.json'),JSON.stringify({schema:'T653_GENERATED_APP_SOURCES_V1',sourceManifestSHA:sha,excludes:['self JSON and source SHA header to avoid recursive identity'],files},null,2)+'\n');
fs.writeFileSync(path.join(out,'entry/src/main/cpp/gpu_resident/m3b/m5_source_manifest.h'),'#pragma once\n#define GRS_M5_SOURCE_MANIFEST_SHA "'+sha+'"\n');console.log('T653 sourceManifestSHA='+sha);
