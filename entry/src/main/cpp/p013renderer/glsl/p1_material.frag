#version 300 es
precision highp float;
precision highp int;
precision highp sampler2DArray;
uniform sampler2D u_tdata;
uniform sampler2DArray u_grain,u_strata;
uniform vec2 u_resolution;
uniform vec4 u_box,u_canvasBox;
uniform float u_capacity;
uniform bool u_visualInverted,u_lightEnabled,u_grainEnabled,u_strataEnabled,u_sparkEnabled;
uniform vec3 u_palette[2],u_grainBase[3];
uniform vec3 u_offsets,u_bandMin,u_bandMax;
uniform vec3 u_strengths;
uniform float u_warpRange;
uniform vec3 u_lightDir;
uniform vec4 u_light; // La,Ld,Lmin,Lmax
uniform vec2 u_lightDepth; // Lref,ellLightMaterial
out vec4 outColor;
/* P013_MATERIAL_FUNCTIONS_V1 */
vec4 D(float x,int row){
 float r=clamp(x/(u_box.z/128.0),0.0,127.0);int i=min(int(floor(r)),126);
 return mix(texelFetch(u_tdata,ivec2(i,row),0),texelFetch(u_tdata,ivec2(i+1,row),0),r-float(i));
}
int positiveMod(int x,int n){return (x%n+n)%n;}
void main(){
 vec2 p=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y);
 vec2 q=p-u_box.xy;outColor=vec4(0.0);
 if(any(lessThan(q,vec2(0.0)))||any(greaterThanEqual(q,u_box.zw)))return;
 if(u_visualInverted)q=u_box.zw-q;
 vec4 d=D(q.x,0),r1=D(q.x,1),r2=D(q.x,2);
 float h=(1.0-q.y/u_box.w)*u_capacity;
 if(h<0.0||h>=d.r)return;
 int k=h<d.b?0:h<d.a?1:2;
 vec2 canvas=u_canvasBox.xy+q/u_box.zw*u_canvasBox.zw;
 float gain=u_sparkEnabled?r2.g:1.0;
 vec3 palette=k==0?u_palette[0]:k==1?u_palette[1]:mix(u_palette[1],u_palette[0],r1.g);
 vec3 baseByte=floor(clamp(palette*255.0*gain,vec3(0.0),vec3(248.0))+0.5);
 // Fixed seed/layer bytes are never re-baked by ablation or light switches.
 ivec2 grainXY=ivec2(positiveMod(int(floor(canvas.x-floor(u_offsets[k]+0.5))),256),positiveMod(int(floor(canvas.y)),256));
 vec3 grainByte=texelFetch(u_grain,ivec3(grainXY,k),0).rgb*255.0;
 float warp=k==0?r1.b:k==1?r1.a:r2.r;
 ivec2 strataXY=ivec2(positiveMod(int(floor(warp/u_warpRange*64.0+0.5)),64),positiveMod(int(floor(canvas.y-floor(u_bandMin[k]))),256));
 vec3 strataByte=texelFetch(u_strata,ivec3(strataXY,k),0).rgb*255.0;
 float c3=128.0;
 float bandHeight=u_bandMax[k]-u_bandMin[k];
 if(bandHeight>0.5 && canvas.y>=u_bandMin[k] && canvas.y<u_bandMax[k]){
   int block=int(floor((canvas.y-u_bandMin[k])/(bandHeight/2.0)));c3=block==0?206.0:159.0;
 }
 vec3 strengths=vec3(u_grainEnabled?u_strengths.x:0.0,u_strataEnabled?u_strengths.y:0.0,u_strataEnabled?u_strengths.z:0.0);
 vec3 n=normalize(vec3(-r1.r,0.0,1.0));
 float face=clamp(u_light.x+u_light.y*max(0.0,dot(n,normalize(u_lightDir))),u_light.z,u_light.w);
 float zPx=(d.r-h)*u_box.w/u_capacity;
 float ellPx=u_lightDepth.y*u_box.w/u_capacity;
 float lm=u_lightEnabled?1.0+exp(-zPx/ellPx)*(face-u_lightDepth.x):1.0;
 outColor=vec4(preparedMaterial(baseByte,grainByte,u_grainBase[k]*255.0,strataByte,c3,strengths,lm,u_lightEnabled),1.0);
}
