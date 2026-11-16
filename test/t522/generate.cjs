const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t507/generate.cjs'),out,'m2'],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),cpp=path.join(main,'cpp'),dest=path.join(cpp,'gpu_resident/m3a');
fs.cpSync('entry/src/main/cpp/gpu_resident/m3a',dest,{recursive:true});
// Build an unsigned HAP, then use the established T257 signing/verifyApp step.
// DevEco's encrypted-password validator rejects the legacy root profile during
// assembleHap; never rewrite or print that shared signing material here.
const profilePath=path.join(out,'build-profile.json5'),profile=JSON.parse(fs.readFileSync(profilePath,'utf8'));
delete profile.app.signingConfigs;for(const product of profile.app.products)delete product.signingConfig;
fs.writeFileSync(profilePath,JSON.stringify(profile,null,2)+'\n');
const cmake=path.join(cpp,'gpu_resident/CMakeLists.txt');
if(!fs.readFileSync(cmake,'utf8').includes('/m3a/kernels.cmake'))fs.appendFileSync(cmake,'\ninclude("${CMAKE_CURRENT_SOURCE_DIR}/m3a/kernels.cmake")\n');
const files={};for(const name of fs.readdirSync(dest))files[name]=crypto.createHash('sha256').update(fs.readFileSync(path.join(dest,name))).digest('hex');
fs.writeFileSync(path.join(out,'t522-manifest.json'),JSON.stringify({revision:'T522_M3_V4_IN_PROGRESS',route:'m3a',product:'IN_PROGRESS',defaultRoute:'m2',files,device:'NOT_RUN'},null,2)+'\n');
