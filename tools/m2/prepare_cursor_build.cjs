// T663 actual M2 route product build preparation. No test/device execution.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]);if(!out.startsWith(path.join(root,'.codex/local-test')+path.sep)||fs.existsSync(out))throw Error('FRESH_M2_APP_REQUIRED');
cp.execFileSync(process.execPath,[path.join(root,'test/t468/generate.cjs'),out],{cwd:root,stdio:'inherit'});
const p=path.join(out,'build-profile.json5'),profile=JSON.parse(fs.readFileSync(p,'utf8'));delete profile.app.signingConfigs;for(const x of profile.app.products)delete x.signingConfig;fs.writeFileSync(p,JSON.stringify(profile,null,2));
const files={};function scan(dir){for(const n of fs.readdirSync(dir)){const x=path.join(dir,n);if(fs.statSync(x).isDirectory())scan(x);else files[path.relative(out,x).replaceAll('\\','/')]=crypto.createHash('sha256').update(fs.readFileSync(x)).digest('hex');}}
scan(path.join(out,'entry/src/main'));fs.writeFileSync(path.join(out,'t663-sources.json'),JSON.stringify({revision:'T663_REAL_M2_API_CURSOR_V1',generator:'test/t468/generate.cjs',files},null,2)+'\n');
console.log(out);
