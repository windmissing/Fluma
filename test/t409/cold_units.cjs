const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),Module=require('node:module'),crypto=require('node:crypto'),cp=require('node:child_process');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const m=new Module(__filename);m.require=id=>id==='@ohos.util'?{TextEncoder:class{encodeInto(t){return new TextEncoder().encode(t);}}}:require(id);
m._compile(ts.transpileModule(fs.readFileSync('entry/src/main/ets/physics/GpuEvidenceCaptureCold.ets','utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020,esModuleInterop:true}}).outputText,__filename);
const {GpuEvidenceCaptureCold}=m.exports;
const folder=process.argv[2]||'.codex/local-test/t409-pages';
const pages=fs.readdirSync(folder).filter(f=>/^page\d+.json$/.test(f)).sort((a,b)=>Number(a.match(/\d+/)[0])-Number(b.match(/\d+/)[0])).map(f=>fs.readFileSync(path.join(folder,f),'utf8'));
const base=JSON.parse(pages[0]);const root='.codex/local-test/t409-archives';fs.mkdirSync(root,{recursive:true});
async function collect(label,change=()=>{}){
 const dir=path.join(root,label);fs.mkdirSync(dir,{recursive:true});
 const port={evidenceCapturePage(id,cursor){const p=JSON.parse(pages[cursor]);change(p,cursor);return JSON.stringify(p);}};
 const store={async save(name,text){fs.writeFileSync(path.join(dir,name),text);return {name,bytes:Buffer.byteLength(text),sha256:crypto.createHash('sha256').update(text).digest('hex')};}};
 return {dir,out:await new GpuEvidenceCaptureCold().collect(port,store,base.captureId,base.packageSha256,123,async()=>{})};
}
(async()=>{
 const positive=await collect('positive');assert.equal(positive.out.state,'COMPLETE');
 const manifest=path.join(positive.dir,fs.readdirSync(positive.dir).filter(f=>f.endsWith('-manifest.json')).at(-1));
 cp.execFileSync('python',['-X','utf8','test/t409/check_archive.py',manifest],{stdio:'inherit'});
 const cases=[['missingRows',(p,i)=>{if(i===1)p.rows.splice(0,1);}],['mixedIdentity',(p,i)=>{if(i===1)p.pageSession='999';}],
 ['dropped',(p)=>{p.incomplete=true;}],['oversizedPage',p=>{p.oversize='x'.repeat(16384);}],
 ['badRequired',p=>{if(p.type==='EVENTS')for(const r of p.rows)if(r.kind==='END')r.kind='SAMPLED_AWAY';}],
 ['cursor',p=>{p.next++;}],['budget',p=>{if(p.type==='SUMMARY')p.summary.maxBytes=1;}]];
 for(const [label,change]of cases)assert.equal((await collect(label,change)).out.state,'INCOMPLETE',label);
 const file=path.join(positive.dir,positive.out.files[0].name);const original=fs.readFileSync(file);fs.unlinkSync(file);
 const missing=cp.spawnSync('python',['-X','utf8','test/t409/check_archive.py',manifest]);assert.notEqual(missing.status,0);fs.writeFileSync(file,original);
 fs.appendFileSync(file,' ');const sha=cp.spawnSync('python',['-X','utf8','test/t409/check_archive.py',manifest]);assert.notEqual(sha.status,0);fs.writeFileSync(file,original);
 console.log(JSON.stringify({revision:'T409_COLD_UNITS_V1',positive:1,rejected:cases.length+2,bytes:positive.out.totalBytes,manifest}));
})().catch(e=>{console.error(e);process.exitCode=1;});
