layout(local_size_x=64) in;
layout(std430,binding=0) readonly buffer Cases { uint cases[]; };
layout(std430,binding=1) writeonly buffer Results { uint result[]; };
void main(){
    uint i=gl_GlobalInvocationID.x;
    if(i>=6u)return;
    uint at=i*36u;
    uint a[16],b[16],m[16],r[16];
    for(uint f=0u;f<16u;++f){a[f]=cases[at+f];b[f]=cases[at+16u+f];}
    bool ok=m2MergeRows(a,b,cases[at+32u],cases[at+33u],cases[at+34u],cases[at+35u],m,r);
    result[i*33u]=uint(ok);
    if(ok)for(uint f=0u;f<16u;++f){result[i*33u+1u+f]=m[f];result[i*33u+17u+f]=r[f];}
}
