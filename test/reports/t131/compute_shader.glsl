#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(binding=0, std430) buffer InBuf { uint data[]; } inBuf;
layout(binding=1, std430) buffer OutBuf { uint data[]; } outBuf;
void main(){
  uint i = gl_GlobalInvocationID.x;
  if (i < uint(inBuf.data.length())) outBuf.data[i] = inBuf.data[i] * 3u + 1u;
}