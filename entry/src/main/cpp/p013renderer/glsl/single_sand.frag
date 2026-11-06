#version 300 es
precision highp float;
precision highp int;
uniform int layer;
uniform vec2 viewport;
uniform vec4 box;
uniform float capacity, scale, depth, grainGain, surfaceGain, coreGain, rimGain;
uniform vec3 albedo, background, lightDir;
uniform bool inverted;
uniform highp sampler2D profile, grain, frameTexture;
in vec2 disc;
flat in vec2 center;
flat in uint state;
out vec4 color;
vec2 bed(float x){
  float r=clamp(x/box.z*128.,0.,127.);
  int i=min(int(floor(r)),126);
  return vec2(mix(texelFetch(profile,ivec2(i,0),0).r,texelFetch(profile,ivec2(i+1,0),0).r,r-float(i)),
              mix(texelFetch(profile,ivec2(i,1),0).r,texelFetch(profile,ivec2(i+1,1),0).r,r-float(i)));
}
vec3 linearRgb(vec3 c){
  return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));
}
void main(){
  if(layer==4){color=texelFetch(frameTexture,ivec2(gl_FragCoord.xy),0);return;}
  vec2 screen=vec2(gl_FragCoord.x,viewport.y-gl_FragCoord.y);
  vec2 q=screen-box.xy;
  bool inside=all(greaterThanEqual(q,vec2(0.)))&&all(lessThan(q,box.zw));
  if(layer==0){
    color=vec4(inside?background:linearRgb(mix(vec3(10.,10.,20.),vec3(26.,26.,46.),screen.y/viewport.y)/255.),1.);
    return;
  }
  if(layer==3){
    vec2 d=abs(q-.5*box.zw)-.5*box.zw;
    if(abs(max(d.x,d.y))>1.5*scale)discard;
    color=vec4(.35);
    return;
  }
  if(!inside)discard;
  if(layer==2){
    float rho=length(disc);
    if(rho>1.||(state==1u&&(1.-center.y/box.w)*capacity<bed(center.x).x))discard;
    float t=clamp((rho-.60)/(.95-.60),0.,1.);
    color=vec4(albedo*(1.+coreGain*(1.-2.*rho*rho)-rimGain*t*t*(3.-2.*t)),1.);
    return;
  }
  if(inverted)q=box.zw-q;
  vec2 surface=bed(q.x);
  float h=(1.-q.y/box.w)*capacity;
  if(h<0.||h>=surface.x)discard;
  ivec2 i=ivec2(floor(q/scale));
  i=(i%256+256)%256;
  float b=floor(texelFetch(grain,i,0).r*255.+.5);
  float n=(b-128.)/(b<=128.?128.:127.);
  float face=max(0.,dot(normalize(vec3(-surface.y,0.,1.)),lightDir))-max(0.,lightDir.z);
  float d=(surface.x-h)*box.w/capacity;
  color=vec4(albedo*(1.+grainGain*n+surfaceGain*exp(-d/(depth*scale))*face),1.);
}
