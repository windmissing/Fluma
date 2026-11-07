const fs=require('node:fs'),cp=require('node:child_process');
const only=process.argv.includes('--permit-only');
const target=only?'.codex/local-test/t404-gate':'.codex/local-test/t404-app';
let script=fs.readFileSync('test/t398/build_host.cjs','utf8')
 .replace("const out='.codex/local-test/t398-host';","const out='.codex/local-test/t404-app';")
 .replaceAll('test/t398/app_host.cpp','test/t404/app_host.cpp');
if(only)script=script.replaceAll('.codex/local-test/t404-app',target).replace("' --quick'","' --permit-only'");
const file=target+'-build.cjs';
fs.writeFileSync(file,script);
cp.execFileSync(process.execPath,[file,...(only?['--quick']:[])],{stdio:'inherit'});
