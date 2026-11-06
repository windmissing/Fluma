const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t351/generate.cjs'),out],{stdio:'inherit'});
// Preserve both production routing and the existing - launch parameter grsm1.
for(const rel of ['entry/src/main/ets/entryability/EntryAbility.ets','entry/src/main/resources/base/profile/main_pages.json']){
 fs.copyFileSync(rel,path.join(out,rel));
}
