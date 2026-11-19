'use strict';
// Actual isolated G11 product. No signing, deployment or execution.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||path.join(root,'.codex/local-test/g11-build'));
if(!out.startsWith(path.join(root,'.codex/local-test')+path.sep)||fs.existsSync(out))throw Error('FRESH_ISOLATED_OUTPUT_REQUIRED');
function copy(rel){function rec(a,b){const s=fs.statSync(a);if(s.isDirectory()){fs.mkdirSync(b,{recursive:true});for(const n of fs.readdirSync(a))if(!['build','.hvigor','oh_modules','.cxx'].includes(n))rec(path.join(a,n),path.join(b,n));}else{fs.mkdirSync(path.dirname(b),{recursive:true});fs.copyFileSync(a,b);}}rec(path.join(root,rel),path.join(out,rel));}
cp.execFileSync(process.execPath,['tools/g11/generate_contract.cjs','--check'],{cwd:root,stdio:'inherit'});
for(const rel of ['AppScope','hvigor','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5','local.properties','entry/src/main','entry/build-profile.json5','entry/hvigorfile.ts','entry/oh-package.json5','tools/g11','test/t566_generate_composite.cjs','docs/architecture-T552-g9-seed-manifest-v2.json','docs/architecture-T660-contract-v1.json','docs/architecture-T660-g11-manifest-v2.json','docs/architecture-T563-g11-r1-dispatch-v1.json','docs/architecture-T563-g11-dm-dispatch-v1.json'])copy(rel);
const manifest=JSON.parse(fs.readFileSync(path.join(root,'docs/architecture-T660-g11-manifest-v2.json'),'utf8'));
for(const rel of [...manifest.inputs.map(x=>x.path),manifest.visualSeedRecipe.path])copy(rel);
fs.symlinkSync(path.join(root,'oh_modules'),path.join(out,'oh_modules'),'junction');
function json(rel,change){const p=path.join(out,rel),j=JSON.parse(fs.readFileSync(p,'utf8'));change(j);fs.writeFileSync(p,JSON.stringify(j,null,2)+'\n');}
json('build-profile.json5',j=>{delete j.app.signingConfigs;for(const p of j.app.products)delete p.signingConfig;});
fs.writeFileSync(path.join(out,'entry/src/main/cpp/CMakeLists.txt'),'cmake_minimum_required(VERSION 3.16)\nproject(g11_isolated)\nadd_subdirectory(gpu_resident/g11vis)\n');
json('entry/src/main/resources/base/profile/main_pages.json',j=>{if(!j.src.includes('pages/GpuResidentG11'))j.src.push('pages/GpuResidentG11');});
json('entry/src/main/module.json5',j=>{const a=j.module.abilities.find(a=>a.name===j.module.mainElement);if(!a)throw Error('NO_ENTRY_ABILITY');a.srcEntry='./ets/entryability/G11ResearchAbility.ets';});
json('entry/oh-package.json5',j=>{j.dependencies['libgrs_g11.so']='file:./src/main/cpp/types/libgrs_g11';});
json('entry/build-profile.json5',j=>{j.buildOption.sourceOption??={};j.buildOption.sourceOption.workers??=[];const w='./src/main/ets/workers/G11ColdWorker.ets';if(!j.buildOption.sourceOption.workers.includes(w))j.buildOption.sourceOption.workers.push(w);});
console.log(out);
