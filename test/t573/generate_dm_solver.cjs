'use strict';
// T573: splice the DM v2 solver body into g9dm/pipeline.comp (canonical
// source). Idempotent; replaces the three alias arms so DM_REQUEST/DM_APPLY
// run the real §4 solver and PREDICTOR/MIDPOINT run the §6.2 stages.
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '../..');
const pipeline = path.join(root, 'entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp');
let s = fs.readFileSync(pipeline, 'utf8').replaceAll('\r\n', '\n');
if (s.includes('bool dmRequest()')) {
  console.log('dm solver already present');
  process.exit(0);
}
const body = fs.readFileSync(path.join(__dirname, 'dm_solver_body.inc'), 'utf8').replaceAll('\r\n', '\n');

// 1. Insert the solver body right before the main dispatch chain comment or
//    the first 'bool g9StartThreshold'. Anchor: the start-threshold helper
//    that precedes the old activation request.
const anchorBody = 'bool g9StartThreshold(out uvec4 threshold){return g9RneProduct(g9U32(W[uConfig]),g9U32(C[23u]),65536u,threshold);}';
if (s.split(anchorBody).length !== 2) throw Error('T573_BODY_ANCHOR');
s = s.replace(anchorBody, body + '\n' + anchorBody);

// 2. Rewire the three dispatch arms to the real DM functions.
const arms = [
  ['else if(uOp==G9_OP_DM_REQUEST){if(g9Cfl()&&g9SnapshotMicro())g9ActivationRequest();}',
   'else if(uOp==G9_OP_DM_REQUEST){if(g9Cfl()&&g9SnapshotMicro())dmRequest();}'],
  ['else if(uOp==G9_OP_DM_APPLY)g9ActivationApply();',
   'else if(uOp==G9_OP_DM_APPLY){for(uint c=0u;c<128u;++c)if(!dmApplyColumn(c))return;}'],
  ['else if(uOp==G9_OP_PREDICTOR_FACE)g9FaceFlux();',
   'else if(uOp==G9_OP_PREDICTOR_FACE){g9FaceFlux();if(!dmPredictor())return;}'],
  ['else if(uOp==G9_OP_MIDPOINT_BUILD)g9FaceFlux();',
   'else if(uOp==G9_OP_MIDPOINT_BUILD){if(!dmMidpoint())return;}'],
  ['else if(uOp==G9_OP_CORRECTOR_FACE)g9FaceFlux();',
   'else if(uOp==G9_OP_CORRECTOR_FACE){g9FaceFlux();}'],
];
for (const [a, b] of arms) {
  if (s.split(a).length !== 2) throw Error('T573_ARM_ANCHOR ' + a.slice(0, 50));
  s = s.replace(a, b);
}
fs.writeFileSync(pipeline, s);
console.log('g9dm pipeline: DM solver + midpoint wired');
