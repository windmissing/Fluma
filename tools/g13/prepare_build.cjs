'use strict';
// Product build preparation only. Does not run tests, sign, install or deploy.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||path.join(root,'.codex/local-test/g13-build'));
if(!out.startsWith(path.join(root,'.codex/local-test')+path.sep)||fs.existsSync(out))throw Error('FRESH_ISOLATED_OUTPUT_REQUIRED');
function copy(a,b){const st=fs.statSync(a);if(st.isDirectory()){fs.mkdirSync(b,{recursive:true});for(const n of fs.readdirSync(a))if(!['build','.hvigor','oh_modules','.cxx'].includes(n))copy(path.join(a,n),path.join(b,n));}else{fs.mkdirSync(path.dirname(b),{recursive:true});fs.copyFileSync(a,b);}}
cp.execFileSync('python',[path.join(__dirname,'generate_assets.py')],{cwd:root,stdio:'inherit'});
for(const n of ['AppScope','hvigor','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5','local.properties'])copy(path.join(root,n),path.join(out,n));
for(const n of ['src/main','build-profile.json5','hvigorfile.ts','oh-package.json5'])copy(path.join(root,'entry',n),path.join(out,'entry',n));
copy(path.join(root,'tools/g13'),path.join(out,'tools/g13'));for(const n of ['engineering','layout','dispatch','math'])copy(path.join(root,`docs/architecture-T607-g13-${n}-v1.json`),path.join(out,`docs/architecture-T607-g13-${n}-v1.json`));
for(const item of JSON.parse(fs.readFileSync(path.join(root,'docs/architecture-T607-g13-engineering-v1.json'),'utf8')).inputs)copy(path.join(root,item.path),path.join(out,item.path));
fs.symlinkSync(path.join(root,'oh_modules'),path.join(out,'oh_modules'),'junction');
const profile=JSON.parse(fs.readFileSync(path.join(out,'build-profile.json5'),'utf8'));delete profile.app.signingConfigs;for(const p of profile.app.products)delete p.signingConfig;fs.writeFileSync(path.join(out,'build-profile.json5'),JSON.stringify(profile,null,2)+'\n');
fs.writeFileSync(path.join(out,'entry/src/main/cpp/CMakeLists.txt'),'cmake_minimum_required(VERSION 3.16)\nproject(g13_isolated)\nadd_subdirectory(gpu_resident/g13flow)\n');
const pages=JSON.parse(fs.readFileSync(path.join(out,'entry/src/main/resources/base/profile/main_pages.json'),'utf8'));if(!pages.src.includes('pages/GpuResidentG13'))pages.src.push('pages/GpuResidentG13');fs.writeFileSync(path.join(out,'entry/src/main/resources/base/profile/main_pages.json'),JSON.stringify(pages,null,2)+'\n');
const modulePath=path.join(out,'entry/src/main/module.json5'),moduleProfile=JSON.parse(fs.readFileSync(modulePath,'utf8'));
const ability=moduleProfile.module.abilities.find(a=>a.name===moduleProfile.module.mainElement);if(!ability)throw Error('G13_ENTRY_ABILITY_NOT_FOUND');ability.srcEntry='./ets/entryability/G13ResearchAbility.ets';
fs.writeFileSync(modulePath,JSON.stringify(moduleProfile,null,2)+'\n');
const pkg=JSON.parse(fs.readFileSync(path.join(out,'entry/oh-package.json5'),'utf8'));pkg.dependencies['libgrs_g13.so']='file:./src/main/cpp/types/libgrs_g13';fs.writeFileSync(path.join(out,'entry/oh-package.json5'),JSON.stringify(pkg,null,2)+'\n');
// Canonical EntryAbility/default routes are untouched. This isolated artifact
// uses its own research OFF ability and never loads missing old route DSOs.
console.log(out);

const entryProfilePath=path.join(out,'entry/build-profile.json5');const entryProfile=JSON.parse(fs.readFileSync(entryProfilePath,'utf8'));entryProfile.buildOption.sourceOption??={};entryProfile.buildOption.sourceOption.workers??=[];const g13Worker='./src/main/ets/workers/G13ColdWorker.ets';if(!entryProfile.buildOption.sourceOption.workers.includes(g13Worker))entryProfile.buildOption.sourceOption.workers.push(g13Worker);fs.writeFileSync(entryProfilePath,JSON.stringify(entryProfile,null,2)+'\n');
