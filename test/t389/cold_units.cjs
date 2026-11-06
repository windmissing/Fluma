const fs=require('node:fs');
const Module=require('node:module');
const file=require('node:path').resolve('test/run_t306_timeline_units.cjs');
let source=fs.readFileSync(file,'utf8');
source=source.replace('test/reports/t306/units.json','.codex/local-test/t389-cold-units.json');
source=source.replace('const report={revision:',`
await test('T389 stages 51–57 preserve raw timing and cold/heat identity',async()=>{
 const out=await collect(7,p=>{
  p.stageRevision='T389_STAGE57_V1';
  for(const row of p.rows){
   row.stage=50+Number(row.serial);
   row.root=row.stage===51?'1':'0';
   row.probeStart='18014398509481984';
   row.probeEnd='18014398509481990';
  }
 });
 assert.equal(out.state,'COMPLETE');
 assert.equal(out.stages[51],'StagingFlush');
 assert.equal(out.stages[57],'ColdBatchFlush');
 assert.equal(JSON.parse(out.before).rows[0].root,'1');
 assert.equal(JSON.parse(out.before).rows[1].root,'0');
 assert.equal(JSON.parse(out.before).rows[0].probeStart,'18014398509481984');
});
const report={revision:`);
const m=new Module(file,module);
m.filename=file;
m.paths=module.paths;
m._compile(source,file);
