const fs=require('node:fs'),cp=require('node:child_process'),crypto=require('node:crypto');
const dir='.codex/local-test/t434-host';fs.mkdirSync('test/reports/t434',{recursive:true});
const env={...process.env,PATH:'C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;'+process.env.PATH};
const rows=[];for(const [k,w] of [[192,64],[192,128],[512,64],[512,128]]){
 const hashes=[];for(const route of ['full','prefix']){
 const stem=`${route}-${k}-${w}`,bin=`${dir}/${stem}.bin`,log=`${dir}/${stem}.txt`;
 const out=fs.openSync(log,'w');
 const r=cp.spawnSync(dir+'/full.exe',[String(k),String(w),route,bin,...(k===192&&w===64?[]:['smoke'])],{env,stdio:['ignore',out,out],timeout:600000});
 fs.closeSync(out);if(r.status!==0)throw Error(stem+' '+r.status+' '+r.error);
 const text=fs.readFileSync(log,'utf8');const report=JSON.parse(text.trim().split(/\r?\n/).at(-1));if(report.failed!==0)throw Error(stem);
 const hash=crypto.createHash('sha256').update(fs.readFileSync(bin)).digest('hex');hashes.push(hash);rows.push({k,w,route,...report,sha256:hash,log});console.log(stem,report.passed,hash);
 }if(hashes[0]!==hashes[1])throw Error('RAW_ACCEPTED_DIFFERENCE '+k+'/'+w);
}
fs.writeFileSync('test/reports/t434/gl-differential.json',JSON.stringify({schema:'T434_DEVELOPER_GL_DIFF_V1',device:'NOT_RUN',engine:'ANGLE SwiftShader GLES 3.1',rows,comparison:'raw accepted state+event64+completion256 byte equality; source/quantum/lease/fault tests; n boundary staging+nonzero suffix'},null,2));
