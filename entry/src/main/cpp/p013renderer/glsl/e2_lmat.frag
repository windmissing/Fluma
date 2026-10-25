#version 300 es
precision highp float;
precision highp int;
uniform sampler2D u_tdata;
uniform vec2 u_resolution;
uniform vec4 u_box;
uniform float u_capacity;
uniform bool u_visualInverted;
uniform vec3 u_lightDir;
uniform float u_la,u_ld,u_lmin,u_lmax,u_lref,u_ellLightPx;
out vec4 outColor;
vec4 D(float x) {
 float r=clamp(x/(u_box.z/128.0),0.0,127.0);
 int i=min(int(floor(r)),126);
 return mix(texelFetch(u_tdata,ivec2(i,0),0),texelFetch(u_tdata,ivec2(i+1,0),0),r-float(i));
}
void main() {
 vec2 p=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y);
 vec2 q=p-u_box.xy;
 outColor=vec4(0.0);
 if(any(lessThan(q,vec2(0.0)))||any(greaterThanEqual(q,u_box.zw)))return;
 if(u_visualInverted)q=u_box.zw-q;
 vec4 d=D(q.x);
 float h=(1.0-q.y/u_box.w)*u_capacity;
 if(h<0.0||h>=d.r)return;
 float dx=u_box.z/128.0;
 float pb=(D(q.x+dx).r-D(q.x-dx).r)*(u_box.w/u_capacity)/(2.0*dx);
 vec3 n=normalize(vec3(-pb,0.0,1.0));
 float lf=clamp(u_la+u_ld*max(0.0,dot(n,u_lightDir)),u_lmin,u_lmax);
 float zPx=(d.r-h)*(u_box.w/u_capacity);
 float lm=1.0+exp(-zPx/u_ellLightPx)*(lf-u_lref);
 outColor=vec4(vec3(clamp(0.5*lm,0.0,1.0)),1.0);
}
