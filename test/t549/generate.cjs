const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t529/generate.cjs'),out],{stdio:'inherit'});
const cpp=path.join(out,'entry/src/main/cpp');
// Frozen T529 production source; concurrent T548 work must not change this runner.
const frozen='e429ea7',prefix='entry/src/main/cpp/gpu_resident/g9spf/';
const files=cp.execFileSync('git',['ls-tree','-r','--name-only',frozen,'--',prefix],{encoding:'utf8'}).trim().split('\n');
for(const name of fs.readdirSync(path.join(cpp,'gpu_resident/g9spf')))fs.unlinkSync(path.join(cpp,'gpu_resident/g9spf',name));
for(const name of files)fs.writeFileSync(path.join(cpp,'gpu_resident/g9spf',path.basename(name)),cp.execFileSync('git',['show',frozen+':'+name]));

fs.copyFileSync(path.join(__dirname,'harness.cpp'),path.join(cpp,'t549_harness.cpp'));
fs.appendFileSync(path.join(cpp,'CMakeLists.txt'),'\nadd_executable(g9_differential t549_harness.cpp)\ntarget_compile_features(g9_differential PRIVATE cxx_std_17)\ntarget_include_directories(g9_differential PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")\ntarget_link_libraries(g9_differential PRIVATE grs_foundation libEGL.so libGLESv3.so libace_ndk.z.so)\n');
const profile=path.join(out,'build-profile.json5'),d=JSON.parse(fs.readFileSync(profile));delete d.app.signingConfigs;for(const p of d.app.products)delete p.signingConfig;fs.writeFileSync(profile,JSON.stringify(d,null,2));
