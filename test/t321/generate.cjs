'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,['test/t309/generate.cjs',out],{cwd:root,stdio:'pipe'});
// T309 prefixes its own imports; T321 production page already has these cold
// export dependencies. Keep one identical import, preserve all instrumentation.
const file=path.join(out,'entry/src/main/ets/pages/ClearWaterDemo.ets');let source=fs.readFileSync(file,'utf8');
for(const line of ["import { common } from '@kit.AbilityKit';","import { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';"]){let seen=false;source=source.split('\n').filter(x=>{if(x.trim()!==line)return true;if(seen)return false;seen=true;return true;}).join('\n');}
fs.writeFileSync(file,source);
fs.writeFileSync(path.join(out,'t321-manifest.json'),JSON.stringify({revision:'T321_SOURCE_CLOCK_RECEIPTS_V1',parent:JSON.parse(fs.readFileSync(path.join(out,'t309-manifest.json'))),scope:'C_F64_REFERENCE_ONLY',recentRows:512,boundaryRows:64,rawClockWindowPolicy:'UNCHANGED_1_OVER_30',rateWater:1,sourceReceiptTraceSha:crypto.createHash('sha256').update(fs.readFileSync(path.join(out,'entry/src/main/ets/physics/clearwater/P011ClearWaterSourceTrace.ets'))).digest('hex')},null,2));
console.log(out);
