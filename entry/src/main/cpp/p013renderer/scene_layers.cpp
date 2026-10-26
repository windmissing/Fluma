#include "scene_layers.h"
#include <algorithm>
namespace p013 {
namespace {
const char* vertex=R"S(#version 300 es
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)S";
const char* fragment=R"S(#version 300 es
precision highp float;precision highp int;
uniform sampler2D u_data,u_legacy;uniform vec2 u_resolution;uniform vec4 u_box,u_canvasBox;
uniform float u_capacity,u_density,u_contrast,u_phase,u_scale;uniform uint u_seed;
uniform int u_layer,u_tier,u_ablation;uniform bool u_dry,u_invert;out vec4 color;
vec3 lin(vec3 c){return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));}
vec4 over(vec4 s,vec4 d){return s+(1.-s.a)*d;}
vec4 rgba(vec3 c,float a){return vec4(lin(c)*a,a);}
vec4 D(float x,int row){float r=clamp(x/u_box.z*128.,0.,127.);int i=min(int(floor(r)),126);return mix(texelFetch(u_data,ivec2(i,row),0),texelFetch(u_data,ivec2(i+1,row),0),r-float(i));}
float hash(ivec2 p,uint seed){uint h=uint(p.x)*0x27d4eb2du ^ uint(p.y)*0x165667b1u ^ seed;h*=0x9E3779B1u;return float((h>>8u)&0xffffffu)/16777215.*2.-1.;}
float noise(vec2 p,uint seed){ivec2 i=ivec2(floor(p));vec2 t=fract(p);t=t*t*(3.-2.*t);return mix(mix(hash(i,seed),hash(i+ivec2(1,0),seed),t.x),mix(hash(i+ivec2(0,1),seed),hash(i+ivec2(1),seed),t.x),t.y);}
float mot(vec2 p){if(u_ablation==5||u_ablation==6){ivec2 i=ivec2(floor(p));i=ivec2((i.x%64+64)%64,(i.y%64+64)%64);vec2 n=texelFetch(u_legacy,i,0).rg;return 1.+1.3*n.r+n.g;}uint s=u_seed ^ 0xE3C0FFEEu;return 1.+1.63*(.565*noise(p/16.,s+1u)+.435*noise(p/48.,s+2u));}
float alphaFog(float tau){float t=floor(clamp(tau,0.,4.)*16.+.5)/16.;return 1.-exp(-t);}
void main(){
 vec2 screen=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y),q=screen-u_box.xy;color=vec4(0.);
 bool inside=all(greaterThanEqual(q,vec2(0.)))&&all(lessThan(q,u_box.zw));
 if(u_layer==0){color=vec4(lin(mix(vec3(10.,10.,20.),vec3(26.,26.,46.),screen.y/u_resolution.y)/255.),1.);if(inside)color=vec4(lin(vec3(16.,16.,36.)/255.),1.);return;}
 // border is allowed half a stroke outside the box and must mirror exactly once.
 if(u_layer==6){if(u_ablation==7)return;vec2 d=abs(screen-(u_box.xy+.5*u_box.zw))-.5*u_box.zw;float distance=abs(max(d.x,d.y));if(distance<=1.5*u_scale)color=rgba(vec3(1.),.35);return;}
 if(!inside)return;if(u_invert)q=u_box.zw-q;
 vec4 d=D(q.x,0);vec4 optics=D(q.x,3);vec2 p=q/u_box.zw*u_canvasBox.zw;
 float surface=(1.-d.g/u_capacity)*u_canvasBox.w,iface=(1.-d.r/u_capacity)*u_canvasBox.w;
 bool water=p.y>=surface&&p.y<iface;
 vec2 samplePoint=p;
 if(u_ablation==5){samplePoint.x=(floor(p.x/(u_canvasBox.z/128.))+.5)*(u_canvasBox.z/128.);samplePoint.y=ceil(surface)+floor((p.y-ceil(surface))/8.)*8.+4.;}
 float m=mot(vec2(samplePoint.x-u_phase,samplePoint.y)+(u_ablation==5?u_canvasBox.xy:vec2(0.)));
 if(u_layer==1){
  if(u_dry||u_ablation==7||!water)return;
  color=vec4(lin(mix(vec3(192.,202.,196.),vec3(168.,186.,182.),p.y/u_canvasBox.w)/255.),1.);
  if(u_tier>=2&&u_ablation!=2){float optical=1.35*max(0.,1.-abs(iface-samplePoint.y)/28.);
   float stretched=clamp(1.+(m-1.)*2.2*u_contrast,.2,2.);float tau=max(0.,optical+.9*optics.r+.7*optics.g+max(0.,.55+1.8*(stretched-1.)));
   float a=min(.78,alphaFog(u_density*tau));if(u_ablation==5&&a>.004)a=floor((min(11.,floor(a/(.78/12.)))+.5)*(.78/12.)*1000.+.5)/1000.;if(a>.004)color=over(rgba(vec3(232.,240.,236.)/255.,a),color);}
 }
 if(u_layer==4){if(!u_dry&&u_ablation!=7&&abs(p.y-surface)<=2.5)color=rgba(vec3(1.),.12);}
 if(u_layer==5){
  if(u_dry||u_tier<2||u_ablation==2||u_ablation==7)return;
  float top=iface-28.,bottom=iface+14.;if(p.y<top||p.y>=bottom)return;
  float sampleY=p.y,ramp=min(1.,(p.y-top)/(bottom-top)*2.5);
  if(u_ablation==5){float seg=floor((p.y-top)/(bottom-top)*8.);sampleY=top+(seg+.5)*(bottom-top)/8.;ramp=min(1.,(seg+.5)/2.5);m=mot(vec2(samplePoint.x-u_phase+u_canvasBox.x,sampleY+u_canvasBox.y));}
  float t=clamp((iface-sampleY+28.)/56.,0.,1.);float weight=t*t*(3.-2.*t)*ramp;
  if(weight<=.01)return;float tau=max(0.,(1.35+.9*optics.r+.7*optics.g)*weight*m);
  float a=min(.55,alphaFog(u_density*tau));if(u_ablation==5&&a>.004)a=floor((min(11.,floor(a/(.55/12.)))+.5)*(.55/12.)*1000.+.5)/1000.;if(a>.004)color=rgba(vec3(232.,240.,236.)/255.,a);
 }
}
)S";
const char* presentFrag=R"S(#version 300 es
precision highp float;uniform sampler2D u_frame;out vec4 color;
void main(){color=texelFetch(u_frame,ivec2(gl_FragCoord.xy),0);}
)S";
std::array<float,16384> LegacyNoise(){
 std::array<float,16384> pixels{};
 for(int layer=0;layer<2;layer++){
  std::array<double,4096> white{},out{};uint32_t state=20261022u+uint32_t(layer);
  for(auto& v:white){state=state*1664525u+1013904223u;v=double((state>>8u)&0xffffffu)/16777215.*2.-1.;}
  int radius=layer?28:8;
  for(int y=0;y<64;y++)for(int x=0;x<64;x++){double sum=0;int count=0;for(int dy=-radius;dy<=radius;dy+=2)for(int dx=-radius;dx<=radius;dx+=2){sum+=white[((y+dy+64)%64)*64+(x+dx+64)%64];count++;}out[y*64+x]=sum/count;}
  double mean=0,maxAbs=1e-9;for(double v:out)mean+=v;mean/=4096.;for(auto& v:out){v-=mean;maxAbs=std::max(maxAbs,std::abs(v));}for(size_t i=0;i<4096;i++)pixels[i*4+layer]=float(out[i]/maxAbs);
 }
 return pixels;
}
GLuint Compile(GLenum type,const char* text){auto s=glCreateShader(type);glShaderSource(s,1,&text,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
GLuint Program(const char* frag){auto v=Compile(GL_VERTEX_SHADER,vertex),f=Compile(GL_FRAGMENT_SHADER,frag);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}auto p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;}
}
Status SceneLayers::Ensure(uint64_t context){
 if(epoch==context&&program&&present&&data&&legacy&&vao)return Status::OK;Reset(epoch==context);epoch=context;std::string source=std::string(vertex)+fragment+presentFrag;sourceHash=MaterialSha256(reinterpret_cast<const uint8_t*>(source.data()),source.size());program=Program(fragment);present=Program(presentFrag);if(!program||!present)return Status::SHADER_COMPILE_FAILED;
 glGenVertexArrays(1,&vao);glGenTextures(1,&data);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,data);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F,128,4);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 auto noise=LegacyNoise();glGenTextures(1,&legacy);glActiveTexture(GL_TEXTURE3);glBindTexture(GL_TEXTURE_2D,legacy);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F,64,64);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,64,64,GL_RGBA,GL_FLOAT,noise.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_RESOURCE_FAILED;
}
Status SceneLayers::Upload(const float* bytes){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,data);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,128,4,GL_RGBA,GL_FLOAT,bytes);return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_RESOURCE_FAILED;}
Status SceneLayers::Draw(int layer,const ScenePacket& p,const SceneConfiguration& c){
 glUseProgram(program);glBindVertexArray(vao);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,data);
 auto loc=[&](const char* n){return glGetUniformLocation(program,n);};const auto& f=p.frame;
 glUniform1i(loc("u_data"),0);glActiveTexture(GL_TEXTURE3);glBindTexture(GL_TEXTURE_2D,legacy);glUniform1i(loc("u_legacy"),3);glUniform1i(loc("u_layer"),layer);glUniform1i(loc("u_tier"),int(c.values[1]));glUniform1i(loc("u_ablation"),int(c.values[2]));glUniform1ui(loc("u_seed"),uint32_t(c.values[50]));
 glUniform1i(loc("u_dry"),p.medium=="DRY_AIR_V1");glUniform1i(loc("u_invert"),f.visualInverted);
 glUniform2f(loc("u_resolution"),f.surfaceWidthPx,f.surfaceHeightPx);glUniform4f(loc("u_box"),f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx);glUniform4f(loc("u_canvasBox"),c.values[6],c.values[7],c.values[8],c.values[9]);
 glUniform1f(loc("u_capacity"),f.capacity);glUniform1f(loc("u_density"),c.values[11]);glUniform1f(loc("u_contrast"),c.values[12]);glUniform1f(loc("u_phase"),p.motion[9]);glUniform1f(loc("u_scale"),c.values[10]);
 glDrawArrays(GL_TRIANGLES,0,3);return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_DRAW_FAILED;
}
Status SceneLayers::Present(GLuint texture,int w,int h){
 glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,w,h);glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_CULL_FACE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glUseProgram(present);glBindVertexArray(vao);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glUniform1i(glGetUniformLocation(present,"u_frame"),0);glDrawArrays(GL_TRIANGLES,0,3);return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_DRAW_FAILED;
}
void SceneLayers::Reset(bool current){if(current){if(program)glDeleteProgram(program);if(present)glDeleteProgram(present);if(vao)glDeleteVertexArrays(1,&vao);if(data)glDeleteTextures(1,&data);if(legacy)glDeleteTextures(1,&legacy);}program=present=vao=data=legacy=0;epoch=0;sourceHash.clear();}
}
