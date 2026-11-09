const fs=require('node:fs'),cp=require('node:child_process');
const out=process.argv.includes('--controls-only')?'.codex/local-test/t409-gate':'.codex/local-test/t409-app';
let script=fs.readFileSync('test/t398/build_host.cjs','utf8')
 .replace("const out='.codex/local-test/t398-host';","const out='"+out+"';")
 .replaceAll('test/t398/app_host.cpp','test/t409/app_host.cpp');
if(process.argv.includes('--controls-only'))script=script.replace("' --quick'","' --controls-only'");
const file=out+'-build.cjs';fs.writeFileSync(file,script);
cp.execFileSync(process.execPath,[file,...(process.argv.includes('--controls-only')?['--quick']:[])],{stdio:'inherit'});
