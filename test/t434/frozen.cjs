const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const commit='0228a7d';
function freeze(out){
 cp.execFileSync(process.execPath,[require.resolve('../t375/generate.cjs'),out],{stdio:'inherit'});
 const roots=['entry/src/main/cpp/gpu_resident','entry/src/main/cpp/CMakeLists.txt','entry/src/main/cpp/types/libgrsm1','entry/src/main/ets/pages/GpuResidentM1.ets','entry/src/main/ets/physics/GpuM1DemoController.ets','entry/src/main/ets/entryability/EntryAbility.ets','entry/src/main/resources/base/profile/main_pages.json'];
 const files=cp.execFileSync('git',['ls-tree','-r','--name-only',commit,'--',...roots],{encoding:'utf8'}).trim().split('\n');
 const hashes={};for(const rel of files){const b=cp.execFileSync('git',['show',commit+':'+rel],{maxBuffer:32*1024*1024});const p=path.join(out,rel);fs.mkdirSync(path.dirname(p),{recursive:true});fs.writeFileSync(p,b);hashes[rel]=crypto.createHash('sha256').update(b).digest('hex');}
 fs.writeFileSync(path.join(out,'frozen-source.json'),JSON.stringify({revision:'T421_FROZEN_SOURCE_V1',commit,files:hashes},null,2));
}
module.exports={freeze};
