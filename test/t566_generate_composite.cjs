#!/usr/bin/env node
// T566 (problem T563): composite G11 pipeline generator.
// Builds the combined parent+visual pipeline: the frozen parent g9spf or
// g9dm pipeline source with the g11vis visual stage appended and the VIS
// dispatch arms wired, plus the composite source passes table from the
// T563 dispatch JSONs. Never modifies the parent files on disk.
const fs = require('node:fs'), path = require('node:path'), crypto = require('node:crypto');
const ROOT = path.resolve(__dirname, '..');
const VIS = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl'), 'utf8').replace(/\r\n/g, '\n');

function parentSource(which) {
  const file = which === 'dm' ? 'g9dm/pipeline.comp' : 'g9spf/pipeline.comp';
  let src = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident', file), 'utf8').replace(/\r\n/g, '\n');
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
    ' // T566 composite VIS ops: independent switch; every lane passes barriers.\n' +
    ' if(uOp==G11_OP_VIS_ROOT_COPY){if(!visRootCopy())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_PLAN_BEGIN){if(!visPlanBegin())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_PLAN_APPLY){if(!visPlanApply())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_ROOT_AUDIT){if(!visRootAudit())return;return;}\n' +
    ' if(uOp==G11_OP_VIS_GLYPH){if(!visGlyphBuild())return;return;}\n';
  src = src.replace(runBody, visArm);
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
  console.log('written: ' + path.join(outDir, 'g11vis_pipeline.h'));
}
emit();
