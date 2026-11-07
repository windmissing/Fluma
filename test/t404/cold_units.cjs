const fs=require('fs'),Module=require('module');
fs.mkdirSync('test/reports/t404',{recursive:true});
let source=fs.readFileSync('test/run_t306_timeline_units.cjs','utf8').replace('test/reports/t306/units.json','test/reports/t404/cold.json');
source=source.replace('const report={revision:',`
await test('T404 V2 stages60-65 preserve distinct identities',async()=>{
 const out=await collect(6,p=>{p.stageRevision='T404_STAGE65_V1';for(const r of p.rows){r.stage=59+Number(r.serial);}});
 assert.equal(out.state,'COMPLETE');assert.equal(out.stages.length,66);
 assert.deepEqual(out.stages.slice(60),['PagePulse','PlanGrant','QuotaBalance','JobIntent','ProgressTimeout','SourceBoundary']);
});
await test('T404 stage66 rejected',async()=>assert.equal((await collect(1,p=>{p.stageRevision='T404_STAGE65_V1';p.rows[0].stage=66;})).state,'INCOMPLETE'));
const report={revision:`);
const m=new Module(__filename);m.paths=module.paths;m._compile(source,__filename);
