#!/usr/bin/env node
// T566 (problem T563): composite G11 pipeline generator.
// Builds the combined parent+visual pipeline: the frozen parent g9spf or
// g9dm pipeline source with the g11vis visual stage appended and the VIS
// dispatch arms wired, plus the composite source passes table from the
// T563 dispatch JSONs. Never modifies the parent files on disk.
const fs = require('node:fs'), path = require('node:path'), crypto = require('node:crypto');
const ROOT = path.resolve(__dirname, '..');
require('node:child_process').execFileSync(process.execPath,[path.join(ROOT,'tools/g11/generate_contract.cjs'),'--check']);
const manifest=JSON.parse(fs.readFileSync(path.join(ROOT,'docs/architecture-T563-g11-manifest-v1.json'),'utf8'));
for(const receipt of [...manifest.inputs,manifest.visualSeedRecipe]){const raw=fs.readFileSync(path.join(ROOT,receipt.path));if(raw.length!==receipt.bytes||crypto.createHash('sha256').update(raw).digest('hex')!==receipt.sha256)throw Error('G11_APPROVED_INPUT_MISMATCH '+receipt.path);}

let VIS = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl'), 'utf8').replace(/\r\n/g, '\n');

for(const name of ['material_plan.glsl','marker_map.glsl']) VIS=VIS.replace('/* g11vis/'+name+' */',fs.readFileSync(path.join(ROOT,'entry/src/main/cpp/gpu_resident/g11vis',name),'utf8').replace(/\r\n/g,'\n'));
function exact(src,anchor,replacement){if(src.split(anchor).length!==2)throw Error('G11_WRITER_ANCHOR '+anchor.slice(0,80));return src.replace(anchor,replacement);}
function parentSource(which) {
  const file = which === 'dm' ? 'g9dm/pipeline.comp' : 'g9spf/pipeline.comp';
  let src = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident', file), 'utf8').replace(/\r\n/g, '\n');
  if(which==='r1') src=exact(src,'uniform uint uOp,uRole,uJslot,uMicro,uPass,uRequiredPasses,','uniform uint uRootKind,uRequiredAudits;\nuniform uint uOp,uRole,uJslot,uMicro,uPass,uRequiredPasses,');
  src=src.replaceAll('uRequiredPasses==12u','uRootKind==1u').replaceAll('uRequiredPasses==2024u','uRootKind==2u').replaceAll('uRequiredPasses==2404u','uRootKind==2u');
  src=src.replace('void g9Completion(){','void g9Completion(){\n if((g9Ctl(17u)&uRequiredAudits)!=uRequiredAudits)g9Fault(30u,0u,uRequiredAudits);');
  if(/uRequiredPasses\s*==\s*(12|2024|2404)u/.test(src))throw Error('G11_IMPLICIT_ROOT_KIND_REMAINS');
  // Observe only successful real writers; discovery has no pending PlanHeader.
  const prototypes='bool g11TapCut(uint,uint,uint);\nbool g11TapDonor(uint,uint,uint,uint);\nbool g11TapDeposit(uint,uint,uint,uint,uint);\nbool g11TapPacketEvent(uint,uint[64]);\nvoid g11CancelTouchColumn(uint);\nvoid g11CompletionExtension();\n';
  src=exact(src,'bool g9WriteEvent(uint at,uint row[64]){',prototypes+'bool g9WriteEvent(uint at,uint row[64]){');
  const eventBegin=src.indexOf('bool g9WriteEvent('),eventEnd=src.indexOf('\nbool g9Seal()',eventBegin);
  if(eventEnd<0)throw Error('G11_EVENT_END');
  const event=src.slice(eventBegin,eventEnd);if(event.split('return true;').length!==3)throw Error('G11_EVENT_SUCCESS_COUNT');
  src=src.slice(0,eventBegin)+event.replaceAll('return true;','return g11TapPacketEvent(at,row);')+src.slice(eventEnd);
  src=exact(src,'if(!g9MapTake(false,c,rank,amount)||!g9MapAdd(true,c,key,amount))return false;left-=amount;',
    'if(!g9MapTake(false,c,rank,amount)||!g9MapAdd(true,c,key,amount))return false;if(!g11TapCut(c,rank,amount))return false;left-=amount;');
  src=exact(src,'if(!g9MapAdd(false,c,g9Key(true,c,i),C[at+3u]))return false;',
    'if(!g9MapAdd(false,c,g9Key(true,c,i),C[at+3u]))return false;uint nr=g9MapCount(false,c)-1u;if(!g11TapDeposit(c,i,nr,C[at+3u],C[g9MapAddress(false,c,nr)+3u]-C[at+3u]))return false;');
  src=exact(src,'if(!g9ProvisionalAdd(receiver,uvec3(W[at],W[at+1u],W[at+2u]),share)){g9Reject(11u,receiver,i);return false;}W[at+3u]-=share;',
    'if(!g9ProvisionalAdd(receiver,uvec3(W[at],W[at+1u],W[at+2u]),share)){g9Reject(11u,receiver,i);return false;}if(!g11TapDonor(donor,side,i,share))return false;W[at+3u]-=share;');
  src=exact(src,'void g9RestoreTouchColumn(uint c){','void g9RestoreTouchColumn(uint c){g11CancelTouchColumn(c);');
  src=exact(src,'memoryBarrierBuffer();W[uCompletion+3u]=G9_SEAL;','g11CompletionExtension();');
  // The visual stage needs the parent helpers (g9Fault etc.); inject right
  // before void g9Run() so all helpers are declared. The stage's own VIS ops
  // hook into the dispatch after this mark.
  const anchor = 'void g9Run(){';
  if (!src.includes(anchor)) throw Error('G11_PARENT_RUN_ANCHOR_MISSING');
  src = src.replace(anchor, VIS + '\n// T566 VIS dispatch arms (composite only; parent ops untouched).\n' + anchor);
  return src;
}

function buildArms(which) {
  // VIS opcode values live in a dedicated high namespace so they never
  // collide with either parent's opcode space.
  const ops = {
    VIS_ROOT_COPY: 512, VIS_PLAN_BEGIN: 513, VIS_PLAN_APPLY: 514,
    VIS_ROOT_AUDIT: 515, VIS_GLYPH: 516,
  };
  let arms = 'const uint G11_OP_VIS_ROOT_COPY=' + ops.VIS_ROOT_COPY + 'u;\n';
  arms += 'const uint G11_OP_VIS_PLAN_BEGIN=' + ops.VIS_PLAN_BEGIN + 'u;\n';
  arms += 'const uint G11_OP_VIS_PLAN_APPLY=' + ops.VIS_PLAN_APPLY + 'u;\n';
  arms += 'const uint G11_OP_VIS_ROOT_AUDIT=' + ops.VIS_ROOT_AUDIT + 'u;\n';
  arms += 'const uint G11_OP_VIS_GLYPH=' + ops.VIS_GLYPH + 'u;\n';
  return { ops, arms };
}

function build(which) {
  let src = parentSource(which);
  const { ops, arms } = buildArms(which);
  const anchor = 'void g9Run(){';
  // Declare VIS op constants + arms before g9Run.
  src = src.replace(anchor, arms + anchor);
  // Wire the arms inside g9Run right after the ROOT_INIT/SOURCE_INIT guard so
  // every lane participates in barriers.
  const runBody = ' uint lane=gl_LocalInvocationIndex;\n';
  if (!src.includes(runBody)) throw Error('G11_RUN_BODY_ANCHOR');
  const visArm = runBody +
    ' if(uOp==G11_COLD_INIT){g11ColdInitialize();return;}\n' +
    ' // T639: VIS rows advance the same actual pass counter before uniform gates.\n' +
    ' if(uOp>=512u&&uOp<=516u){if(lane==0u){if(g9Ctl(16u)!=uPass)g9Fault(25u,0u,uPass);g9Set(16u,g9Ctl(16u)+1u);}memoryBarrierBuffer();barrier();uJ=(uRole==2u||uRole==4u)?g9Ctl(11u):uJslot;if(g9Ctl(0u)!=0u)return;if(uOp==513u||uOp==514u){if(uRootKind==2u&&((uRole!=2u&&uRole!=4u)||!g9ScopeEnabled()||(uMicro!=0xffffffffu&&(uMicro>=uJ||g9Ctl(14u)!=0u))))return;}}\n' +
    ' if(uOp==G11_OP_VIS_ROOT_COPY){if(!visRootCopy())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_PLAN_BEGIN){if(!visPlanBegin())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_PLAN_APPLY){if(!visPlanApply())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_ROOT_AUDIT){if(!visRootAudit())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_GLYPH){if(!visGlyphBuild())return;return;}\n';
  const runAt=src.indexOf(anchor);if(runAt<0)throw Error('RUN_ANCHOR');const bodyAt=src.indexOf(runBody,runAt);if(bodyAt<0)throw Error('RUN_BODY_AFTER_ANCHOR');src=src.slice(0,bodyAt)+visArm+src.slice(bodyAt+runBody.length);
  const sha = crypto.createHash('sha256').update(src).digest('hex');
  return { src, sha, ops };
}

function passTable(which) {
  const file = which === 'dm' ? 'docs/architecture-T563-g11-dm-dispatch-v1.json' : 'docs/architecture-T563-g11-r1-dispatch-v1.json';
  const d = JSON.parse(fs.readFileSync(path.join(ROOT, file), 'utf8'));
  const visOps = { VIS_ROOT_COPY: 512, VIS_PLAN_BEGIN: 513, VIS_PLAN_APPLY: 514, VIS_ROOT_AUDIT: 515, VIS_GLYPH: 516 };
  // Parent opcode value maps: R1 uses 0..41 (g9spf), DM uses 0..43 (g9dm).
  // The dispatch rows reference parent ops by name; translate with the same
  // numeric namespace the parent pipeline declares.
  const r1Ops = { SOURCE_INIT_BASE: 30, SOURCE_VALIDATE_INPUT: 31, SOURCE_DEDUP: 32, SOURCE_DECODE: 33, SOURCE_CREDIT: 34, SOURCE_PRIVATE_BIRTH: 35, SOURCE_ORIGIN_CERTIFICATE: 36, SOURCE_LEDGER: 37, SOURCE_AUDIT_REPLAY: 38, SOURCE_COMPARE: 39, SOURCE_PACK_SCENE: 40, SOURCE_SEAL: 41, ROOT_INIT: 0, BASE_FREEZE: 1, CONFIG_INPUT_GATE: 2, TRIAL_SCHEDULE_BEGIN: 3, INIT_COPY: 4, INPUT_AND_CFL: 5, GRAVITY_WORK: 6, TOUCH_ATOMIC: 7, AFTER_TOUCH_CFL: 8, ACTIVATE_REQUEST: 9, ACTIVATE_APPLY: 10, FLIGHT_PATH: 11, PARTICLE_FOLD: 12, FACE_FLUX_MOBILITY: 13, FLOW_ORIGIN_RESERVE: 14, FLOW_PAIR_APPLY: 15, FRICTION_REST_DEPOSIT: 16, ORDERED_CAPTURE: 17, MICRO_AUDIT_SEAL: 18, FINAL_MASS_ORIGIN_MOMENTUM: 19, FINAL_ENERGY_CURSORS: 20, SCOPE_SEAL_AND_CHOICE: 21, DISCOVERY_COMMIT_SELECTION: 22, AUTHORITATIVE_BEGIN: 23, REJECT_PREFIX_BEGIN: 24, REPLAY_BEGIN: 25, COMPARE_SELECTED_AUTHORITATIVE: 26, COMPARE_REPLAY_STREAM: 27, PACK_SCENE_METADATA: 28, ROOT_SEAL: 29 };
  const dmOps = { SOURCE_INIT_BASE: 0, SOURCE_VALIDATE_INPUT: 1, SOURCE_DEDUP: 2, SOURCE_DECODE: 3, SOURCE_CREDIT: 4, SOURCE_PRIVATE_BIRTH: 5, SOURCE_ORIGIN_CERTIFICATE: 6, SOURCE_LEDGER: 7, SOURCE_AUDIT_REPLAY: 8, SOURCE_COMPARE: 9, SOURCE_PACK_SCENE: 10, SOURCE_SEAL: 11, ROOT_INIT: 12, BASE_FREEZE: 13, CONFIG_INPUT_GATE: 14, TRIAL_SCHEDULE_BEGIN: 15, INIT_COPY: 16, INPUT_AND_CFL: 17, GRAVITY_WORK: 18, TOUCH_ATOMIC: 19, AFTER_TOUCH_CFL: 20, DM_REQUEST: 21, DM_APPLY: 22, FLIGHT_PATH: 23, PARTICLE_FOLD: 24, PREDICTOR_FACE: 25, MIDPOINT_BUILD: 26, CORRECTOR_FACE: 27, FLOW_ORIGIN_RESERVE: 28, FLOW_PAIR_APPLY: 29, FRICTION_REST_DEPOSIT: 30, ORDERED_CAPTURE: 31, MICRO_AUDIT_SEAL: 32, FINAL_MASS_ORIGIN_MOMENTUM: 33, FINAL_ENERGY_CURSORS: 34, SCOPE_SEAL_AND_CHOICE: 35, DISCOVERY_COMMIT_SELECTION: 36, AUTHORITATIVE_BEGIN: 37, REJECT_PREFIX_BEGIN: 38, REPLAY_BEGIN: 39, COMPARE_SELECTED_AUTHORITATIVE: 40, COMPARE_REPLAY_STREAM: 41, PACK_SCENE_METADATA: 42, ROOT_SEAL: 43 };
  const map = which === 'dm' ? dmOps : r1Ops;
  const roleCode = { ROOT: 0, DISCOVERY: 1, AUTHORITATIVE: 2, REJECT_PREFIX: 3, REPLAY: 4, SOURCE: 1 };
  const phaseCode = { TOUCH: 1, NATURAL: 2, FLOW: 3, DEPOSIT: 4, CAPTURE: 5, SOURCE: 6 };
  function rows(list) {
    const out = [];
    for (const r of list) {
      const isVis = r.op && r.op.startsWith('VIS_');
      const op = isVis ? visOps[r.op] : map[r.op];
      if (op === undefined) throw Error('G11_UNKNOWN_OP ' + r.op);
      const role = roleCode[r.role] !== undefined ? roleCode[r.role] : (r.role === 'REPLAY' ? 4 : 0);
      const micro = (r.micro === undefined || r.micro === null) ? -1 : r.micro;
      const phase = r.batchPhase ? phaseCode[r.batchPhase] : 0;
      out.push({ op, role, j: r.Jslot || 0, micro, vis: isVis ? 1 : 0, phase });
    }
    return out;
  }
  return { source: rows(d.source), quantum: rows(d.quantum) };
}

function emit() {
  const outDir = process.argv[2] || path.join(ROOT, 'entry/.cxx/default/default/debug/arm64-v8a/gpu_resident');
  fs.mkdirSync(outDir, { recursive: true });
  let header = '#pragma once\n#include <array>\n#include <cstdint>\nstruct G11Pass{uint32_t op;uint32_t role,j;int32_t micro;uint32_t vis,phase;};\n';
  for (const which of ['r1', 'dm']) {
    const { src, sha, ops } = build(which);
    const tag = which === 'dm' ? 'Dm' : 'R1';
    const raw = 'kG11' + tag + 'Pipeline';
    header += 'static constexpr const char* ' + raw + '=R"G11V(' + src + ')G11V";\n';
    header += 'static constexpr const char* ' + raw + 'Hash="' + sha + '";\n';
    const table = passTable(which);
    if(table.source.length!==19||table.quantum.length!==(which==='dm'?2923:2543))throw Error('G11_FROZEN_PASS_COUNT');
    const tname = 'kG11' + tag + 'Passes';
    for (const kind of ['source', 'quantum']) {
      const rows = table[kind];
      const varName = 'kG11' + tag + (kind === 'source' ? 'Source' : 'Quantum') + 'Passes';
      header += 'inline constexpr std::array<G11Pass,' + rows.length + '> ' + varName + '={{\n';
      for (const r of rows) header += '{' + r.op + 'u,' + r.role + 'u,' + r.j + 'u,' + r.micro + ',' + r.vis + 'u,' + r.phase + 'u},\n';
      header += '}};\n';
    }
    console.log('g11vis composite ' + which.toUpperCase() + ': pipeline sha256=' + sha.slice(0, 16) + '… source=' + table.source.length + ' quantum=' + table.quantum.length);
  }
  fs.writeFileSync(path.join(outDir, 'g11vis_pipeline.h'), header);
  for(const which of ['r1','dm']){fs.writeFileSync(path.join(outDir,'g11vis_'+which+'.comp'),build(which).src);}
  const inputs=['entry/src/main/cpp/gpu_resident/g9spf/pipeline.comp','entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp','entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl','entry/src/main/cpp/gpu_resident/g11vis/material_plan.glsl','entry/src/main/cpp/gpu_resident/g11vis/marker_map.glsl','tools/g11/generate_contract.cjs','docs/architecture-T552-g9-seed-manifest-v2.json','entry/src/main/cpp/gpu_resident/g11vis/parent_seed_generated.h','docs/architecture-T563-g11-r1-dispatch-v1.json','docs/architecture-T563-g11-dm-dispatch-v1.json','docs/architecture-T563-g11-manifest-v1.json','entry/src/main/cpp/gpu_resident/g11vis/generated_contract.h','test/t566_generate_composite.cjs',...manifest.inputs.map(x=>x.path),manifest.visualSeedRecipe.path];
  fs.writeFileSync(path.join(outDir,'g11vis_dependencies.cmake'),'set(G11_INPUTS\n'+[...new Set(inputs)].map(p=>'\"'+path.join(ROOT,p).replaceAll('\\','/')+'\"').join('\n')+'\n)\n');
  fs.writeFileSync(path.join(outDir,'g11vis_inputs.json'),JSON.stringify(Object.fromEntries(inputs.map(p=>[p,crypto.createHash('sha256').update(fs.readFileSync(path.join(ROOT,p))).digest('hex')])),null,2));
  console.log('written: ' + path.join(outDir, 'g11vis_pipeline.h'));
}
emit();
