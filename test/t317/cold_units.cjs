// Extend the unchanged T306 checks with versioned T317 pages. Run this file
// directly; output belongs to this task, never overwrites T306 evidence.
const fs=require('fs'),Module=require('module');
let source=fs.readFileSync('test/run_t306_timeline_units.cjs','utf8');
source=source.replace("test/reports/t306/units.json","test/reports/t317/cold.json");
source=source.replace("const report={revision:",`
await test('T317 all 39 stages preserve dictionary and raw codes',async()=>{
 const out=await collect(39,p=>{p.stageRevision='T317_STAGE39_V2';for(const row of p.rows){row.stage=Number(row.serial);row.code=4294967295;}});
 assert.equal(out.state,'COMPLETE');assert.equal(out.stageRevision,'T317_STAGE39_V2');assert.equal(out.stages.length,40);assert.equal(out.stages[24],'SubmitEnter');assert.equal(out.stages[39],'PollExit');
});
await test('T317 stage40 refused',async()=>assert.equal((await collect(1,p=>{p.stageRevision='T317_STAGE39_V2';p.rows[0].stage=40;})).state,'INCOMPLETE'));
await test('dictionary switch during export refused',async()=>assert.equal((await collect(5,(p,n)=>{if(n===1)p.stageRevision='T317_STAGE39_V2';})).state,'INCOMPLETE'));
const report={revision:`);
const m=new Module(__filename);m.paths=module.paths;m._compile(source,__filename);
