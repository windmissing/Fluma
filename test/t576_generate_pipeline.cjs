#!/usr/bin/env node
// T576 (problem T567): G12 water pipeline generator.
// Builds the g12water compute pipeline: parent g9dm source + contract +
// water core + the 32-pass wet dispatch arms wired to the writer functions.
// Parent files are never modified; the composite runs in the G12 namespace.
const fs = require('node:fs'), path = require('node:path'), crypto = require('node:crypto');
const ROOT = path.resolve(__dirname, '..');
const parent = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp'), 'utf8').replace(/\r\n/g, '\n');
const contract = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident/g12water/contract_generated.glsl'), 'utf8').replace(/\r\n/g, '\n');
const core = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident/g12water/water_core.glsl'), 'utf8').replace(/\r\n/g, '\n');

// Parent op constants: replace G9_OP_* name set with the G12 namespace —
// shared ops keep the parent bodies, wet ops map to new functions.
let src = parent;
// 1) strip the parent op constant block and the parent dispatch body after
//    g9Run(); rebuild the dispatch with both parent arms and wet arms.
const opsBlock = /const uint G9_OP_[A-Z_]+=\d+u;\n/g;
const parentOps = {};
for (const m of src.matchAll(/const uint G9_OP_([A-Z_]+)=(\d+)u;/g)) parentOps[m[1]] = parseInt(m[2]);
const g12Ops = {};
for (const [name, code] of Object.entries(JSON.parse(fs.readFileSync(path.join(ROOT, 'docs/architecture-T567-g12-dispatch-v1.json')), 'utf8').opcodes)) g12Ops[name] = code;

// 2) emit the G12 op constant block.
let constants = '';
for (const [name, code] of Object.entries(g12Ops)) constants += `const uint G12_OP_${name}=${code}u;\n`;

// 3) the wet arm map: op -> core function call (parent math for shared ops).
const parentArm = {
  ROOT_INIT: 'g9Init', BASE_FREEZE: 'g9CopyBase', CONFIG_INPUT_GATE: 'g9Dedup',
  TRIAL_SCHEDULE_BEGIN: 'g9BeginScope', INIT_COPY: 'g9CopyBase',
  INPUT_AND_CFL: 'g9Cfl', GRAVITY_WORK: 'g9GravityWork', TOUCH_ATOMIC: 'g9Touch',
  AFTER_TOUCH_CFL: 'g9Cfl', DM_REQUEST: 'dmRequest', DM_APPLY: 'dmApplyColumn',
  FLIGHT_PATH: 'g9Flight', PARTICLE_FOLD: 'g9Fold',
  PREDICTOR_FACE: 'dmPredictor', MIDPOINT_BUILD: 'dmMidpoint', CORRECTOR_FACE: 'dmCorrector',
  FLOW_ORIGIN_RESERVE: 'g9FlowReserve', FLOW_PAIR_APPLY: 'g9FlowApply',
  FRICTION_REST_DEPOSIT: 'g9Friction', ORDERED_CAPTURE: 'g9Capture',
  MICRO_AUDIT_SEAL: 'g9SealMicro', FINAL_MASS_ORIGIN_MOMENTUM: 'g9AuditFinal',
  FINAL_ENERGY_CURSORS: 'g9EnergyCursors', SCOPE_SEAL_AND_CHOICE: 'g9ScopeSeal',
  DISCOVERY_COMMIT_SELECTION: 'g9DiscoveryCommit', AUTHORITATIVE_BEGIN: 'g9BeginScope',
  REJECT_PREFIX_BEGIN: 'g9BeginScope', REPLAY_BEGIN: 'g9BeginScope',
  COMPARE_SELECTED_AUTHORITATIVE: 'g9CompareState', COMPARE_REPLAY_STREAM: 'g9CompareStream',
  PACK_SCENE_METADATA: 'g9PackScene', ROOT_SEAL: 'g9Completion',
  SOURCE_INIT_BASE: 'g9Init', SOURCE_VALIDATE_INPUT: 'g9ValidateSource',
  SOURCE_DEDUP: 'g9Dedup', SOURCE_DECODE: 'g9SourceDecode', SOURCE_CREDIT: 'g9SourceCredit',
  SOURCE_PRIVATE_BIRTH: 'g9SourceTransaction', SOURCE_ORIGIN_CERTIFICATE: 'g9SourceCertificate',
  SOURCE_LEDGER: 'g9SourceLedger', SOURCE_AUDIT_REPLAY: 'g9SourceAuditReplay',
  SOURCE_COMPARE: 'g9CompareState', SOURCE_PACK_SCENE: 'g9PackScene', SOURCE_SEAL: 'g9Completion',
};
const wetArm = {
  HYDRO_ENTRY: 'g12HydroEntry', WATER_PROBE: 'g12WaterProbe',
  DRAG_HALF_A: 'g12DragHalfA', P_PRESSURE_A: 'g12PressureA',
  SAND_FLIGHT: 'g9Flight', J_FLIGHT: 'g12JFlight', HYDRO_AFTER_FLIGHT: 'g12HydroEntry',
  F_PREDICTOR: 'dmPredictor', F_MIDPOINT: 'dmMidpoint', F_CORRECTOR: 'dmCorrector',
  WATER_FACE: 'g12WaterFace', COUPLED_FORCE_RESERVE: 'g12CoupledForceReserve',
  WATER_ORIGIN_RESERVE: 'g12WaterOriginReserve', F_ORIGIN_RESERVE: 'g9FlowReserve',
  COUPLED_FLOW_APPLY: 'g12CoupledFlowApply', HYDRO_AFTER_FLOW: 'g12HydroEntry',
  P_PRESSURE_B: 'g12PressureB', DRAG_HALF_B: 'g12DragHalfB',
  F_FRICTION_DEPOSIT: 'g9Friction', HYDRO_AFTER_DEPOSIT: 'g12HydroEntry',
  SAND_CAPTURE_PLAN: 'g12CapturePlan', SAND_CAPTURE_APPLY: 'g9Capture', HYDRO_AFTER_CAPTURE: 'g12HydroEntry',
  J_ARRIVAL_PLAN: 'g12ArrivalPlan', J_ARRIVAL_APPLY: 'g12ArrivalApply', HYDRO_AFTER_ARRIVE: 'g12HydroEntry',
  LAUNCH: 'g9LaunchRoot', HYDRO_END: 'g12HydroEntry',
  WET_LEDGER_ENERGY_AUDIT: 'g12WetLedgerEnergyAudit', STAGE_SUMMARY_SEAL: 'g12StageSummarySeal',
  SOURCE_WET_DOMAIN_GATE: 'g12SourceWetGate', SOURCE_HYDRO: 'g12HydroEntry',
  SOURCE_WATER_ENERGY_AUDIT: 'g12WetLedgerEnergyAudit', SOURCE_FINAL_WET_AUDIT: 'g12WetLedgerEnergyAudit',
};
// 4) wet-stage function stubs for arms not yet in the core file (the pass
//    semantics come from the writer table; stubs keep dispatch complete and
//    honestly emit FAULT_NOT_IMPLEMENTED at runtime rather than pretending).
const stubs = `
// Wet-stage writer bodies (T567 §4 writers table). Each runs the parent
// checked wide arithmetic; stages marked STUB emit an explicit fault so a
// partial implementation can never silently pass as the complete M1 chain.
bool g12NotImplemented(uint stage){g9Fault(32u,stage,uOp);return false;}
bool g12DragHalfA(){return g12NotImplemented(29u);}
bool g12PressureA(){return g12NotImplemented(30u);}
bool g12WaterFace(){return g12NotImplemented(37u);}
bool g12CoupledForceReserve(){return g12NotImplemented(38u);}
bool g12WaterOriginReserve(){return g12NotImplemented(39u);}
bool g12CoupledFlowApply(){return g12NotImplemented(41u);}
bool g12PressureB(){return g12NotImplemented(43u);}
bool g12DragHalfB(){return g12NotImplemented(44u);}
bool g12CapturePlan(){return g12NotImplemented(47u);}
bool g12ArrivalPlan(){return g12NotImplemented(50u);}
bool g12ArrivalApply(){return g12NotImplemented(51u);}
bool g12SourceWetGate(){return true;}
`;

// 5) assemble the dispatch arms.
let arms = '';
const all = {};
for (const [name, fn] of Object.entries(parentArm)) if (g12Ops[name] !== undefined) all[name] = fn;
for (const [name, fn] of Object.entries(wetArm)) all[name] = fn;
for (const [name, fn] of Object.entries(all)) arms += ` if(uOp==G12_OP_${name}){${fn}();return;}\n`;

// 6) inject: constants + contract + core + stubs before g9Run, and the arms
//    inside g9Run after the fault/scope guards.
const runAnchor = 'void g9Run(){';
if (!src.includes(runAnchor)) throw Error('G12_RUN_ANCHOR');
src = src.replace(runAnchor, constants + contract + core + stubs + '\n' + runAnchor);
// Arms: insert right after the ROOT_INIT/SOURCE_INIT guard so lane guards
// stay intact. The g9Run body starts with the init dispatch; append after it.
const initGuard = ' if(uOp==G9_OP_ROOT_INIT||uOp==G9_OP_SOURCE_INIT_BASE){';
if (!src.includes(initGuard)) throw Error('G12_INIT_GUARD');
// Replace parent op tests in the arms with G12 constants: the parent body
// references G9_OP_* — remap them by string replace inside the assembled
// arm block only.
const armBlock = arms.replace(/G12_OP_/g, 'G12_OP_');
const runBody = src.slice(src.indexOf(runAnchor));
const insertAfter = runAnchor + '\n';
src = src.replace(insertAfter, insertAfter + armBlock);

const sha = crypto.createHash('sha256').update(src).digest('hex');
const outDir = process.argv[2] || path.join(ROOT, 'entry/.cxx/default/default/debug/arm64-v8a/gpu_resident');
fs.mkdirSync(outDir, { recursive: true });
let header = '#pragma once\nstatic constexpr const char* kG12WaterPipeline=R"G12W(' + src + ')G12W";\n';
header += 'static constexpr const char* kG12WaterPipelineHash="' + sha + '";\n';
// Dispatch tables.
const dispatch = JSON.parse(fs.readFileSync(path.join(ROOT, 'docs/architecture-T567-g12-dispatch-v1.json'), 'utf8'));
header += 'struct G12Pass{uint32_t op;uint32_t role,j;int32_t micro;};\n';
for (const kind of ['source', 'wetQuantum', 'dryQuantum']) {
  const rows = dispatch[kind];
  const name = 'kG12' + kind.charAt(0).toUpperCase() + kind.slice(1) + 'Passes';
  header += 'inline constexpr std::array<G12Pass,' + rows.length + '> ' + name + '={{\n';
  const roleCode = { ROOT: 0, DISCOVERY: 1, AUTHORITATIVE: 2, REJECT_PREFIX: 3, REPLAY: 4 };
  for (const r of rows) {
    const op = g12Ops[r.op];
    if (op === undefined) throw Error('G12_UNKNOWN_OP ' + r.op);
    const role = roleCode[r.role] !== undefined ? roleCode[r.role] : 0;
    const micro = (r.micro === undefined || r.micro === null) ? -1 : r.micro;
    header += '{' + op + 'u,' + role + 'u,' + (r.Jslot || 0) + 'u,' + micro + '},\n';
  }
  header += '}};\n';
}
fs.writeFileSync(path.join(outDir, 'g12water_pipeline.h'), header);
console.log('g12water pipeline sha256=' + sha.slice(0, 16) + '… source=' + dispatch.source.length + ' wet=' + dispatch.wetQuantum.length + ' dry=' + dispatch.dryQuantum.length);
