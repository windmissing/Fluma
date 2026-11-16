// Generated V4 source event field masks.
const uint M3_MERGE=15u;
const uvec4 M3_MERGE_MASK=uvec4(4160749567u,4294967295u,4294967295u,255u);
const uint M3_BIRTH=16u;
const uvec4 M3_BIRTH_MASK=uvec4(4160684031u,65535u,65535u,255u);
const uint M3_DIRECT=17u;
const uvec4 M3_DIRECT_MASK=uvec4(4160651263u,0u,0u,255u);
const uint M3_END=18u;
const uvec4 M3_END_MASK=uvec4(4269555711u,0u,0u,255u);
const uint M3_ETA_COLUMN=20u;
const uvec4 M3_ETA_COLUMN_MASK=uvec4(3456270335u,2047u,0u,255u);
const uint M3_SEED_BEGIN=21u;
const uvec4 M3_SEED_BEGIN_MASK=uvec4(3456532479u,1536u,0u,255u);
const uint M3_TRIAL_END=22u;
const uvec4 M3_TRIAL_END_MASK=uvec4(3464265727u,63488u,0u,255u);
const uint M3_TRIAL_BEGIN=23u;
const uvec4 M3_TRIAL_BEGIN_MASK=uvec4(3456139263u,6144u,0u,255u);
uvec4 m3SourceMask(uint kind){
if(kind==M3_MERGE)return M3_MERGE_MASK;
if(kind==M3_BIRTH)return M3_BIRTH_MASK;
if(kind==M3_DIRECT)return M3_DIRECT_MASK;
if(kind==M3_END)return M3_END_MASK;
if(kind==M3_ETA_COLUMN)return M3_ETA_COLUMN_MASK;
if(kind==M3_SEED_BEGIN)return M3_SEED_BEGIN_MASK;
if(kind==M3_TRIAL_END)return M3_TRIAL_END_MASK;
if(kind==M3_TRIAL_BEGIN)return M3_TRIAL_BEGIN_MASK;
return uvec4(0u);
}
