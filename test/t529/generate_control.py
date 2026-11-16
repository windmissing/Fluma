"""Reuse the frozen T508 time ledger in the independent G9 namespace.
Only SOURCE packet identity validation changes; its clock mathematics does not.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident';D=GPU/'g9spf'
names=['control_types.h','progress_clock_v3.h','progress_clock_v3.cpp','quota_fields_generated.h','quota_reader_v3.h','quota_reader_v3.cpp','quota_snapshot_v3.cpp']
for name in names:
 s=(GPU/'m3a'/name).read_text(encoding='utf-8-sig').replace('grs::m3a','grs::g9spf').replace('ControlIdentityV4','ControlIdentity').replace('SourcePacketV4','InputPacket').replace('SourceIntentKeyV4','SourceIntentKey')
 if name=='control_types.h':s=s.replace('std::array<uint32_t,544>','std::array<uint32_t,128>')
 if name=='progress_clock_v3.cpp':s=s.replace('packet.words[0]!=1129534023||packet.words[1]!=4||packet.words[2]>2||packet.words[31]!=0','packet.words[0]!=826882375||packet.words[1]!=1||packet.words[40]!=1')
 (D/name).write_text(s,encoding='utf-8')
