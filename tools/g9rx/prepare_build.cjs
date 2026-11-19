'use strict';
// Unsigned isolated product preparation only: no execution, signing or device.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||path.join(root,'.codex/local-test/g9rx-build'));
if(!out.startsWith(path.join(root,'.codex/local-test')+path.sep)||fs.existsSync(out))throw Error('FRESH_ISOLATED_OUTPUT_REQUIRED');
function copy(a,b){const stat=fs.statSync(a);if(stat.isDirectory()){fs.mkdirSync(b,{recursive:true});for(const n of fs.readdirSync(a))if(!['build','.hvigor','oh_modules','.cxx','generated'].includes(n))copy(path.join(a,n),path.join(b,n));}else{fs.mkdirSync(path.dirname(b),{recursive:true});fs.copyFileSync(a,b);}}
for(const n of ['AppScope','hvigor','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5','local.properties'])copy(path.join(root,n),path.join(out,n));
for(const n of ['src/main','build-profile.json5','hvigorfile.ts','oh-package.json5'])copy(path.join(root,'entry',n),path.join(out,'entry',n));
copy(path.join(root,'tools/g9rx'),path.join(out,'tools/g9rx'));
for(const n of ['architecture-T641-wire-v1.json','architecture-T641-runtime-v1.json','architecture-T641-storage-cold-v1.json','architecture-T641-seed-config-v1.json','architecture-T647-acceptance-v1.json','design-T637-manifest-v1.json','design-G9-dynamic-mobilization-manifest-v2.json','architecture-T552-g9-seed-manifest-v2.json'])copy(path.join(root,'docs',n),path.join(out,'docs',n));
fs.symlinkSync(path.join(root,'oh_modules'),path.join(out,'oh_modules'),'junction');
function read(rel){return JSON.parse(fs.readFileSync(path.join(out,rel),'utf8').replace(/^\uFEFF/,''));}function save(rel,data){fs.writeFileSync(path.join(out,rel),JSON.stringify(data,null,2)+'\n');}
const profile=read('build-profile.json5');delete profile.app.signingConfigs;for(const p of profile.app.products)delete p.signingConfig;save('build-profile.json5',profile);
fs.writeFileSync(path.join(out,'entry/src/main/cpp/CMakeLists.txt'),'cmake_minimum_required(VERSION 3.16)\nproject(g9rx_isolated)\nadd_subdirectory(gpu_resident/g9rx)\n');
const pages=read('entry/src/main/resources/base/profile/main_pages.json');if(!pages.src.includes('pages/GpuResidentG9Rx'))pages.src.push('pages/GpuResidentG9Rx');save('entry/src/main/resources/base/profile/main_pages.json',pages);
const mod=read('entry/src/main/module.json5');mod.module.abilities.find(a=>a.name===mod.module.mainElement).srcEntry='./ets/entryability/G9RxResearchAbility.ets';save('entry/src/main/module.json5',mod);
const pkg=read('entry/oh-package.json5');pkg.dependencies['libgrs_g9rx.so']='file:./src/main/cpp/types/libgrs_g9rx';save('entry/oh-package.json5',pkg);
console.log(out);

const ep=read('entry/build-profile.json5');ep.buildOption.sourceOption??={};ep.buildOption.sourceOption.workers??=[];ep.buildOption.sourceOption.workers.push('./src/main/ets/workers/G9RxColdWorker.ets');save('entry/build-profile.json5',ep);
