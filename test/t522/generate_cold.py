"""Explicit V4 adaptation of the delivered T507 two-slot ownership protocol.

Its queue/cancellation/read lease logic is reused; route, bounds, validation and
payload schema are independently versioned. Existing M2 readers are untouched.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
s=(GPU/'cold_trace.h').read_text(encoding='utf-8-sig')
s=s.replace('#include "cold_protocol.h"','#include "cold_contract.h"')
s=s.replace('namespace grs {namespace cold503 {','''namespace grs {namespace m3a {namespace coldtrace {
using cold503::Scope;using cold503::ScopeJson;
using Writer=cold::Writer;
inline constexpr size_t MaxTraceBytes=cold::kMaximumTraceBytes,TraceBudget=cold::kTwoSlotTraceBytes,PageWords=cold::kPageWords;
inline constexpr uint64_t TtlNs=60000000000ULL;''')
s=s.replace('std::strcmp(actual.route.data(),"m2")','std::strcmp(actual.route.data(),"m3a")')
s=s.replace('c[1]!=3','c[1]!=4').replace('return state==s.expectedState;','''
  if(state!=s.expectedState||state<2848||c[49]!=15||c[173]!=1||c[174]!=7||c[188]!=1)return false;
  QuotaSnapshotV3 quota;std::memcpy(quota.data(),c+256+state-2848,1024);
  return InspectQuotaSnapshotV3(quota).result==ControlResult::OK;''')
s=s.replace('T503_M2_COLD_TRACE_V1','GRS_M3_BOUNDED_COLD_V1').replace('T445_M2_TRACE_V3','GRS_M3_TRACE_V4_QUOTA3_CONTROL128').replace('GRS_GPU_ROOT_Q20_M2_V3','GRS_GPU_ROOT_Q20_M3_V4')
s=s.replace('i<173;++i','i<189;++i')
s=s.replace('w.Number(read.stateWords);', '''w.Number(read.stateWords);
 w.Raw(",\\\"physicalStateWords\\\":");w.Number(read.stateWords-2848);
 w.Raw(",\\\"quotaWordOffset\\\":");w.Number(256+read.stateWords-2848);
 w.Raw(",\\\"controlWordOffset\\\":");w.Number(512+read.stateWords-2848);
 w.Raw(",\\\"inputWordOffset\\\":");w.Number(2560+read.stateWords-2848);
 w.Raw(",\\\"eventsWordOffset\\\":");w.Number(256+read.stateWords);''')
assert s.rstrip().endswith('}}')
s=s.rstrip()[:-2]+'}}}\n'
from full_cold import extend
s=extend(s)
s='\n'.join(line.rstrip() for line in s.splitlines())+'\n'
(GPU/'m3a/cold_trace.h').write_text('// T522 generated explicit V4 cold protocol; see generate_cold.py.\n'+s,encoding='utf-8')
