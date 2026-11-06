// T324 cold-export dictionary checks. Extends the unchanged T306/T317
// checks with the 41-stage dictionary: SubmitRootBegin=40, SubmitCancel=41.
const fs=require('fs'),Module=require('module');
let source=fs.readFileSync('test/run_t306_timeline_units.cjs','utf8');
source=source.replace("test/reports/t306/units.json","test/reports/t324/cold.json");
source=source.replace("const report={revision:",`
await test('T324 41-stage dictionary accepted, 40/41 named, 42 refused',async()=>{
 const out=await collect(41,p=>{p.stageRevision='T324_STAGE41_V1';for(const row of p.rows){row.stage=Number(row.serial);row.code=4294967295;}});
 assert.equal(out.state,'COMPLETE');assert.equal(out.stageRevision,'T324_STAGE41_V1');assert.equal(out.stages.length,42);assert.equal(out.stages[40],'SubmitRootBegin');assert.equal(out.stages[41],'SubmitCancel');
});
await test('T324 stage42 refused',async()=>assert.equal((await collect(1,p=>{p.stageRevision='T324_STAGE41_V1';p.rows[0].stage=42;})).state,'INCOMPLETE'));
await test('T324 mixed revision switch during export refused',async()=>assert.equal((await collect(5,(p,n)=>{if(n===1)p.stageRevision='T324_STAGE41_V1';})).state,'INCOMPLETE'));
await test('T375 stage42 pair and raw wall preserved in cold pages',async()=>{
 const out=await collect(2,p=>{
  p.stageRevision='T324_STAGE42_V1';
  for(const row of p.rows){row.stage=42;row.code=0;row.carry=Number(row.serial)-1;row.delta=row.carry===0?'0':'123456789';}
 });
 assert.equal(out.state,'COMPLETE');assert.equal(out.stages.length,43);assert.equal(out.stages[42],'SubmitWarmup');
 assert.equal(JSON.parse(out.before).rows[1].delta,'123456789');
});
const report={revision:`);
const m=new Module(__filename);m.paths=module.paths;m._compile(source,__filename);
