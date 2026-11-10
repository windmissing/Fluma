layout(local_size_x=64) in;
layout(std430,binding=0) coherent buffer Cases { uint cases[]; };
layout(std430,binding=1) writeonly buffer Results { uint result[]; };
layout(std430,binding=2) coherent buffer Workspace { uint W[]; };
void m2LoadSand(uint slot,out uint row[16]) {
    for(uint f=0u;f<16u;++f)row[f]=cases[slot*16u+f];
}
layout(std430,binding=3) coherent buffer SourceRows { uint E[]; };
