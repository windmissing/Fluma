const fs=require('node:fs'),cp=require('node:child_process');
const script=fs.readFileSync('test/t372/build_host.cmd','utf8')
 .replace('test/t372/prepare_host.py','test/t406/prepare_host.py');
const out='.codex/local-test/t406-host.cmd';
fs.writeFileSync(out,script);
cp.execFileSync('cmd.exe',['/c',out.replaceAll('/','\\'),process.argv[2]],{stdio:'inherit'});
