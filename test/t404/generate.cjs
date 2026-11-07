const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t375/generate.cjs'),out],{stdio:'inherit'});
const cmake=path.join(out,'entry/src/main/cpp/CMakeLists.txt');
fs.appendFileSync(cmake,'\ntarget_compile_definitions(grsm1 PRIVATE GRS_PROGRESS_V2_EXPERIMENT)\n');
