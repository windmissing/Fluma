from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
out=ROOT/'.codex/local-test/t517-lineage.glsl'
out.write_text('''#version 310 es
precision highp int;
layout(local_size_x=64)in;
layout(std430,binding=0)readonly buffer Base{uint B[];};
layout(std430,binding=1)buffer Candidate{uint C[];};
layout(std430,binding=2)buffer Work{uint W[];};
layout(std430,binding=3)buffer Events{uint E[];};
uniform uint uK,uColumn,uPass;
'''+(GPU/'integer_primitives.glsl').read_text(encoding='utf-8-sig')+'\n'+
 (GPU/'m3b/lineage.glsl').read_text(encoding='utf-8')+'\n'+(GPU/'m3b/lift_numeric.glsl').read_text(encoding='utf-8')+'''
void main(){if(gl_LocalInvocationIndex!=0u)return;
 if(uPass==0u){
  W[0]=uint(v5Append(0u,uvec3(1u,0u,7u),100u));
  W[1]=uint(v5Append(0u,uvec3(1u,0u,7u),50u));
  W[2]=uint(v5Append(0u,uvec3(2u,0u,8u),80u));
  C[uColumn+256u]=230u;W[3]=uint(v5AuditLayers(0u));
  for(uint rank=0u;rank<64u;++rank)v5Append(1u,uvec3(rank%2u+1u,0u,rank%2u+7u),1u);
  W[4]=uint(!v5CanAppend(1u,uvec3(1u,0u,7u)));
  W[5]=v5BedPrefix(0u,1u,90u); // receiver top origin2: 80, then layer-full
  W[6]=uint(v5Pop(0u,80u));W[7]=C[uV5Depth];
 }else if(uPass==1u){
  C[0]=100u;C[uK]=524288u;C[5u*uK]=1u;C[7u*uK]=7u;C[8u*uK]=1u;C[10u*uK]=1u;
  W[8]=uint(v5BirthCertificate(0u,1u,uvec2(5u,0u),1u,0u,150u));
  C[1]=50u;C[uK+1u]=524288u;C[5u*uK+1u]=2u;C[7u*uK+1u]=7u;C[8u*uK+1u]=1u;C[10u*uK+1u]=2u;
  W[9]=uint(v5BirthCertificate(1u,0u,uvec2(6u,0u),2u,0u,0u));
  W[10]=uint(v5MergeCertificate(0u,1u,100u,50u));
  uint returned;W[11]=uint(v5ReturnTag(0u,150u,40u,returned));W[12]=returned;W[13]=C[uV5Aux];
  W[14]=uint(v5ReturnTag(0u,110u,110u,returned));W[15]=returned;
 }else if(uPass==2u){
  uint next,request;int vx,vy;
  W[16]=uint(v5Energy(65536u,0u,2u,65536u,65536u,next));W[17]=next;
  W[18]=uint(v5Energy(0u,65536u,2u,1u,1u,next));W[19]=next;
  W[20]=uint(v5Request(65536u,268435456u,1048576u,62914560u,request));W[21]=request;
  W[22]=uint(v5Request(0u,268435456u,1048576u,62914560u,request));W[23]=request;
  W[24]=uint(v5LiftVelocity(67108864u,524288,908093,vx,vy));W[25]=uint(vx);W[26]=uint(vy);
  W[27]=uint(!v5Energy(65537u,0u,2u,1u,1u,next));
 }
}
''',encoding='utf-8')
