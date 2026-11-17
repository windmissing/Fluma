'use strict';
// T555: generate entry/src/main/cpp/gpu_resident/g9dm/schema_generated.h from
// the T554 machine tables (manifest v2 + dispatch v2). Mirrors the g9spf
// schema shape with DM v2 identity/resources/2404-pass table + DM48 WORK map.
const fs = require('fs');
const path = require('path');
const root = process.cwd();
const man = JSON.parse(fs.readFileSync(path.join(root, 'docs/architecture-T554-g9-dm-manifest-v2.json'), 'utf8'));
const disp = JSON.parse(fs.readFileSync(path.join(root, 'docs/architecture-T554-g9-dm-dispatch-v2.json'), 'utf8'));

const roleMap = { ROOT: 0, DISCOVERY: 1, AUTHORITATIVE: 2, REJECTED_PREFIX_CHECK: 3, REPLAY: 4 };
const ops = disp.opcodeByName;

// Parent state/work bank ranges (g9spf parent layout — T554 keeps the parent
// state/work layout untouched and only appends the DM48 segment at the tail).
const parentLayout = {
  192: {
    state: [[0,1024],[1024,512],[1536,512],[2048,16384],[18432,131072],[149504,131072],[280576,49152],[329728,262144]],
    work: [[0,16512],[16512,32768],[49280,1048576],[1097856,65536],[1163392,1536],[1164928,512],[1165440,1536],[1166976,4096],[1171072,1024],[1172096,1536],[1173632,512],[1174144,9216],[1183360,32768],[1216128,65536],[1281664,5120]],
  },
  512: {
    state: [[0,1024],[1024,512],[1536,512],[2048,16384],[18432,131072],[149504,131072],[280576,131072],[411648,262144]],
    work: [[0,16512],[16512,32768],[49280,1048576],[1097856,65536],[1163392,4096],[1167488,512],[1168000,1536],[1169536,4096],[1173632,2048],[1175680,1536],[1177216,512],[1177728,9216],[1186944,32768],[1219712,65536],[1285248,5120]],
  },
};

const out = [];
out.push('#pragma once');
out.push('// T555 (problem T554): G9_SPF_DM_Q20_V2 schema, generated verbatim from');
out.push('// docs/architecture-T554-g9-dm-manifest-v2.json + -dispatch-v2.json.');
out.push('// Regenerate with: node test/t555_generate_dm_schema.cjs');
out.push('#include <array>');
out.push('#include <cstddef>');
out.push('#include <cstdint>');
out.push('namespace grs::g9dm {');
out.push(`inline constexpr uint32_t kProfileCode=${man.profileCode}u,kSeal=${man.identity.seal}u;`);
out.push(`inline constexpr uint32_t kStateMagic=${man.identity.stateMagic}u;`);
out.push(`inline constexpr uint32_t kEventMagic=${man.identity.eventMagic}u;`);
out.push(`inline constexpr uint32_t kPathMagic=${man.identity.pathMagic}u;`);
out.push(`inline constexpr uint32_t kInputMagic=${man.identity.inputMagic}u;`);
out.push(`inline constexpr uint32_t kCompletionMagic=${man.identity.completionMagic}u;`);
out.push(`inline constexpr uint32_t kColdMagic=${man.identity.coldMagic}u;`);
out.push(`inline constexpr uint32_t kRevision=${man.identity.revision}u;`);
out.push(`inline constexpr uint32_t kConfigMagic=${man.config.magic}u,kContainerRevision=${man.config.revision}u,kParameterRevision=${man.config.parameterRevision}u;`);
out.push(`inline constexpr const char* kRoute="${man.route}";`);
out.push(`inline constexpr const char* kColdCapability="${man.cold.capability}";`);
out.push(`inline constexpr const char* kColdPage="${man.cold.page}";`);
out.push('');
out.push('enum class Op:uint32_t {');
out.push(Object.keys(ops).map(n => `${n}=${ops[n]}`).join(','));
out.push('};');
out.push('');
out.push('struct Pass {Op op;uint32_t role,j;int32_t micro;};');
out.push(`inline constexpr std::array<Pass,${disp.quantum.length}> kQuantumPasses={{`);
for (const p of disp.quantum) out.push(`{Op::${p.op},${roleMap[p.role]},${p.Jslot},${p.micro}},`);
out.push('};');
out.push(`inline constexpr std::array<Pass,${disp.source.length}> kSourcePasses={{`);
for (const p of disp.source) out.push(`{Op::${p.op},${roleMap[p.role]},${p.Jslot},${p.micro}},`);
out.push('};');
out.push('');
out.push('struct Range{size_t begin,bytes;constexpr size_t End()const{return begin+bytes;}};');
out.push('struct Profile{uint32_t k,eventCapacity,pathCapacity;size_t stateBytes,workBytes,arenaBytes,residentBytes,fullColdBytes,rootColdBytes;std::array<Range,8> state;std::array<Range,15> work;};');
out.push('inline constexpr std::array<Profile,2> kProfiles={{');
for (const r of man.resources) {
  const layout = parentLayout[r.K];
  const stateRanges = layout.state.map(([b, by]) => `{${b},${by}}`).join(',');
  const workRanges = layout.work.map(([b, by]) => `{${b},${by}}`).join(',');
  out.push(`{${r.K},${r.eventRows},${r.pathRows},${r.stateBytes},${r.workBytes},${r.arenaBytes},${r.residentPayloadBytes},${r.FULL.bytes},${r.ROOT.bytes},{{{${stateRanges}}}},{{{${workRanges}}}}},`);
}
out.push('};');
out.push('');
// DM48 per-column WORK segment map (48 words per column, from appendedWork).
const aw = man.resources[0].appendedWork;
out.push(`inline constexpr size_t kDM48ByteStart=${aw.byteStart},kDM48Bytes=${aw.bytes},kDM48Stride=${aw.strideBytes},kDM48Columns=${aw.columns};`);
out.push('namespace dm48 {');
aw.fields.forEach((f, i) => out.push(`inline constexpr size_t ${f}=${i};`));
out.push('}');
out.push('');
// Header word map — parent header 106..113 carries the eight DM parameter words.
out.push('namespace header {');
out.push('inline constexpr size_t depthGravityGainQ16=106,depthShearGainQ16=107,regularizationDepthQ=108,omegaBaseQ16=109,omegaGravityGainQ16=110,omegaShearGainQ16=111,omegaMaxQ16=112,mobilizationPolicy=113;');
out.push('}');
out.push('');
// Research parameter values (G9_DM_RESEARCH_R0, explicit unapproved construction).
const rp = man.researchSpec.research.parameters;
out.push(`inline constexpr uint32_t kResearchDepthGravityGainQ16=${rp.depthGravityGainQ16.value},kResearchDepthShearGainQ16=${rp.depthShearGainQ16.value},kResearchRegularizationDepthQ=${rp.regularizationDepthQ.value};`);
out.push(`inline constexpr uint32_t kResearchOmegaBaseQ16=${rp.omegaBaseQ16.value},kResearchOmegaGravityGainQ16=${rp.omegaGravityGainQ16.value},kResearchOmegaShearGainQ16=${rp.omegaShearGainQ16.value},kResearchOmegaMaxQ16=${rp.omegaMaxQ16.value};`);
out.push(`inline constexpr uint32_t kMobilizationPolicyCode=${man.researchSpec.research.policyCode ?? 2};`);
out.push(`inline constexpr const char* kResearchSpecId="${man.researchSpec.id}";`);
out.push('}');

fs.mkdirSync(path.join(root, 'entry/src/main/cpp/gpu_resident/g9dm'), { recursive: true });
fs.writeFileSync(path.join(root, 'entry/src/main/cpp/gpu_resident/g9dm/schema_generated.h'), out.join('\n') + '\n');
console.log('g9dm/schema_generated.h written:', out.length, 'lines,', disp.quantum.length, 'quantum passes');
