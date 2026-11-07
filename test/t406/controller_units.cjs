const fs=require('fs'),Module=require('module');
let source=fs.readFileSync('test/t394/units.cjs','utf8');
source=source.replace(" ctl.beginT394(arm+'_A',",` assert.equal(ctl.setT406Plan('bad'),'INVALID_FROZEN_PLAN');
 assert.equal(ctl.setT406Plan('a'.repeat(64)+'|'+'b'.repeat(64)+'|278,503,44,44;878,503,44,44;578,798,44,44'),'FROZEN_PLAN_READY');
 ctl.beginT394(arm+'_A',`);
source=source.replace("   readbacks++;", "   assert.equal(packet.calibration,false);readbacks++;");
const m=new Module(__filename);m.paths=module.paths;m._compile(source,__filename);
