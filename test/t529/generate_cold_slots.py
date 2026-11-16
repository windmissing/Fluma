from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];GPU=ROOT/'entry/src/main/cpp/gpu_resident'
s=(GPU/'m3b/cold_trace.h').read_text(encoding='utf-8').replace('m3b','g9spf').replace('T527_V5_BOUNDED_COLD_V1','G9_SPF_COLD_V1')
s=s.replace('SelectProfile(b[8],b[9])','SelectProfile(b[7])').replace('SelectProfile(s.block[8],s.block[9])','SelectProfile(s.block[7])').replace('SelectProfile(read.block[8],read.block[9])','SelectProfile(read.block[7])')
(GPU/'g9spf/cold_trace.h').write_text(s,encoding='utf-8')
