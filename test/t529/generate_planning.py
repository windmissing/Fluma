from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];GPU=ROOT/'entry/src/main/cpp/gpu_resident'
s=(GPU/'m3a/planning_control.cpp').read_text(encoding='utf-8').replace('grs::m3a','grs::g9spf').replace('RootPermitV4','Permit').replace('ValidatedBlockedTailV4','ValidatedBlockedControlRecord').replace('ValidatedSuccessTailV4','ValidatedRecord').replace('tail.Permit()','tail.Permission()').replace('p.physicsCommit','p.baseCommit').replace('tail.Words()[29]','tail.Words()[7]')
(GPU/'g9spf/planning_control.cpp').write_text(s,encoding='utf-8')
