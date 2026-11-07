const fs=require('node:fs'),path=require('node:path'),Module=require('node:module');
const file=path.resolve('test/run_t306_timeline_units.cjs');
let source=fs.readFileSync(file,'utf8');
source=source.replace('test/reports/t306/units.json','.codex/local-test/t398-cold-units.json');
source=source.replace('const report={revision:',`
await test('T398 bounded export preserves V1 remaining budget and generation',async()=>{
 const out=await collect(2,p=>{
  p.stageRevision='T398_STAGE59_V1';
  for(const row of p.rows){
   row.stage=57+Number(row.serial);row.root='17';row.carry=8;
   row.probeStart='18014398509481984';row.probeEnd='18014398509482000';
   row.probeA='16';row.probeB='0';row.probeC='13';
  }
 });
 assert.equal(out.state,'COMPLETE');
 assert.equal(out.stages[58],'IsolationCall');
 assert.equal(out.stages[59],'IsolationRevoke');
 assert.equal(JSON.parse(out.before).rows[0].probeStart,'18014398509481984');
 assert.equal(JSON.parse(out.before).rows[1].probeC,'13');
});
const report={revision:`);
const m=new Module(file,module);m.filename=file;m.paths=module.paths;m._compile(source,file);
