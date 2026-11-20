'use strict';
// Unsigned isolated product preparation only: no execution, signing or device.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||path.join(root,'.codex/local-test/g9dm-t648-build'));
if(!out.startsWith(path.join(root,'.codex/local-test')+path.sep)||fs.existsSync(out))throw Error('FRESH_ISOLATED_OUTPUT_REQUIRED');
const boot=process.argv[3]==='--boot-only';
if(process.argv[3]&&!boot)throw Error('EXPECTED_OPTIONAL_BOOT_ONLY');
function copy(a,b){const stat=fs.statSync(a);if(stat.isDirectory()){fs.mkdirSync(b,{recursive:true});for(const n of fs.readdirSync(a))if(!['build','.hvigor','oh_modules','.cxx','generated'].includes(n))copy(path.join(a,n),path.join(b,n));}else{fs.mkdirSync(path.dirname(b),{recursive:true});fs.copyFileSync(a,b);}}
for(const n of ['AppScope','hvigor','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5','local.properties'])copy(path.join(root,n),path.join(out,n));
for(const n of ['src/main','build-profile.json5','hvigorfile.ts','oh-package.json5'])copy(path.join(root,'entry',n),path.join(out,'entry',n));
copy(path.join(root,'tools/g9dm'),path.join(out,'tools/g9dm'));
copy(path.join(root,'tools/glsl/qualified_values.py'),path.join(out,'tools/glsl/qualified_values.py'));
if(boot) for(const asset of ['tools/glsl/g9_boot.py','tools/g13/split_shader.py','docs/architecture-T733-closure-v1.json'])copy(path.join(root,asset),path.join(out,asset));
for(const n of ['architecture-T554-g9-dm-manifest-v2.json','architecture-T554-g9-dm-dispatch-v2.json','architecture-T647-acceptance-v1.json','architecture-T552-g9-seed-manifest-v2.json'])copy(path.join(root,'docs',n),path.join(out,'docs',n));
fs.symlinkSync(path.join(root,'oh_modules'),path.join(out,'oh_modules'),'junction');
function read(rel){return JSON.parse(fs.readFileSync(path.join(out,rel),'utf8').replace(/^\uFEFF/,''));}function save(rel,data){fs.writeFileSync(path.join(out,rel),JSON.stringify(data,null,2)+'\n');}
const profile=read('build-profile.json5');delete profile.app.signingConfigs;for(const p of profile.app.products)delete p.signingConfig;save('build-profile.json5',profile);
fs.writeFileSync(path.join(out,'entry/src/main/cpp/CMakeLists.txt'),'cmake_minimum_required(VERSION 3.16)\nproject(dm_t647_compile_only)\n'+(boot?'set(GRS_G9_BOOT_ONLY ON)\n':'')+'add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../tools/g9dm/native" dm_native)\n');
console.log(out);
