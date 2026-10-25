#include "dualfield_pass.h"
#include <cstring>
namespace p013 {
namespace {
const char* fullVertex=R"S(#version 300 es
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)S";
const char* bedFragment=R"S(#version 300 es
precision highp float;precision highp int;
uniform sampler2D u_data;uniform vec2 u_resolution;uniform vec4 u_box;uniform float u_capacity;uniform bool u_invert;
uniform vec3 u_heavy,u_light;out vec4 color;
vec3 linearize(vec3 c){return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));}
void main(){vec2 p=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y)-u_box.xy;color=vec4(0.);
 if(any(lessThan(p,vec2(0.)))||any(greaterThanEqual(p,u_box.zw)))return;
 if(u_invert)p=u_box.zw-p;
 float r=clamp(p.x/u_box.z*128.,0.,127.);int i=min(int(floor(r)),126);
 vec4 d=mix(texelFetch(u_data,ivec2(i,0),0),texelFetch(u_data,ivec2(i+1,0),0),r-float(i));
 float h=(1.-p.y/u_box.w)*u_capacity;if(h>=d.r)return;
 vec3 c=h<d.b?u_heavy:h<d.a?u_light:mix(u_light,u_heavy,.5);color=vec4(linearize(c),1.);
}
)S";
const char* particleVertex=R"S(#version 300 es
precision highp float;precision highp int;
layout(location=0) in vec2 a_xy;layout(location=1) in uvec2 a_meta;
uniform vec2 u_resolution;uniform vec4 u_box;uniform float u_capacity,u_radius;uniform bool u_invert;
out vec2 local;flat out uint phaseState;out vec2 center;
void main(){vec2 corners[6]=vec2[6](vec2(-1,-1),vec2(1,-1),vec2(-1,1),vec2(-1,1),vec2(1,-1),vec2(1,1));
 local=corners[gl_VertexID];vec2 uv=vec2(a_xy.x/128.,1.-a_xy.y/u_capacity);if(u_invert)uv=1.-uv;
 center=u_box.xy+uv*u_box.zw;vec2 p=center+local*u_radius;
 gl_Position=vec4(2.*p.x/u_resolution.x-1.,1.-2.*p.y/u_resolution.y,0.,1.);phaseState=a_meta.x;}
)S";
const char* particleFragment=R"S(#version 300 es
precision highp float;precision highp int;
in vec2 local;flat in uint phaseState;in vec2 center;
uniform sampler2D u_data;uniform vec2 u_resolution;uniform vec4 u_box;uniform float u_capacity;uniform bool u_invert;
uniform vec3 u_heavy,u_light;out vec4 color;
vec3 linearize(vec3 c){return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));}
void main(){vec2 q=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y)-u_box.xy;
 if(dot(local,local)>1.||any(lessThan(q,vec2(0.)))||any(greaterThanEqual(q,u_box.zw)))discard;
 if((phaseState>>8u)==1u){vec2 p=center-u_box.xy;if(u_invert)p=u_box.zw-p;
 float r=clamp(p.x/u_box.z*128.,0.,127.);int i=min(int(floor(r)),126);
 float sand=mix(texelFetch(u_data,ivec2(i,0),0).r,texelFetch(u_data,ivec2(i+1,0),0).r,r-float(i));
 if((1.-p.y/u_box.w)*u_capacity<sand)discard;}
 color=vec4(linearize((phaseState&255u)==0u?u_heavy:u_light),1.);
}
)S";
GLuint Compile(GLenum type,const char* source){GLuint s=glCreateShader(type);glShaderSource(s,1,&source,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
GLuint Program(const char* v,const char* f){GLuint a=Compile(GL_VERTEX_SHADER,v),b=Compile(GL_FRAGMENT_SHADER,f);if(!a||!b){if(a)glDeleteShader(a);if(b)glDeleteShader(b);return 0;}GLuint p=glCreateProgram();glAttachShader(p,a);glAttachShader(p,b);glLinkProgram(p);glDeleteShader(a);glDeleteShader(b);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;}
void Uniforms(GLuint p,const FrameInput& f,GLuint texture,const double* colors){glUseProgram(p);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glUniform1i(glGetUniformLocation(p,"u_data"),0);glUniform2f(glGetUniformLocation(p,"u_resolution"),f.surfaceWidthPx,f.surfaceHeightPx);glUniform4f(glGetUniformLocation(p,"u_box"),f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx);glUniform1f(glGetUniformLocation(p,"u_capacity"),f.capacity);glUniform1i(glGetUniformLocation(p,"u_invert"),f.visualInverted);glUniform3f(glGetUniformLocation(p,"u_heavy"),colors[0],colors[1],colors[2]);glUniform3f(glGetUniformLocation(p,"u_light"),colors[3],colors[4],colors[5]);}
}
void DualFieldPass::Reset(bool current){if(current){if(bed)glDeleteProgram(bed);if(particles)glDeleteProgram(particles);if(vao)glDeleteVertexArrays(1,&vao);if(xyBuffer)glDeleteBuffers(1,&xyBuffer);if(metaBuffer)glDeleteBuffers(1,&metaBuffer);}bed=particles=vao=xyBuffer=metaBuffer=0;epoch=0;capacity=0;}
Status DualFieldPass::Draw(const FrameInput& f,GLuint texture,const float* xy,const uint32_t* meta,size_t count,size_t nmax,bool enabled,const DualVisual& visual,uint64_t generation){
 if(epoch!=generation||capacity!=nmax){Reset(epoch==generation);epoch=generation;capacity=nmax;
 sourceHash=14695981039346656037ULL;
 for(auto text:{fullVertex,bedFragment,particleVertex,particleFragment})for(size_t i=0;i<std::strlen(text);i++){sourceHash^=static_cast<unsigned char>(text[i]);sourceHash*=1099511628211ULL;}
 bed=Program(fullVertex,bedFragment);particles=Program(particleVertex,particleFragment);if(!bed||!particles)return Status::SHADER_COMPILE_FAILED;
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&xyBuffer);glBindBuffer(GL_ARRAY_BUFFER,xyBuffer);glBufferData(GL_ARRAY_BUFFER,nmax*8,nullptr,GL_DYNAMIC_DRAW);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,8,nullptr);glEnableVertexAttribArray(0);glVertexAttribDivisor(0,1);
 glGenBuffers(1,&metaBuffer);glBindBuffer(GL_ARRAY_BUFFER,metaBuffer);glBufferData(GL_ARRAY_BUFFER,nmax*8,nullptr,GL_DYNAMIC_DRAW);glVertexAttribIPointer(1,2,GL_UNSIGNED_INT,8,nullptr);glEnableVertexAttribArray(1);glVertexAttribDivisor(1,1);bufferAllocations+=2;}
 glBindFramebuffer(GL_FRAMEBUFFER,0);glBindVertexArray(vao);glViewport(0,0,f.surfaceWidthPx,f.surfaceHeightPx);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_STENCIL_TEST);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
 Uniforms(bed,f,texture,visual.colors.data());glDrawArrays(GL_TRIANGLES,0,3);uploadBytes=0;
 if(enabled&&count){glBindBuffer(GL_ARRAY_BUFFER,xyBuffer);glBufferSubData(GL_ARRAY_BUFFER,0,count*8,xy);glBindBuffer(GL_ARRAY_BUFFER,metaBuffer);glBufferSubData(GL_ARRAY_BUFFER,0,count*8,meta);uploadBytes=count*16;Uniforms(particles,f,texture,visual.colors.data()+6);glUniform1f(glGetUniformLocation(particles,"u_radius"),visual.radiusPx);glDrawArraysInstanced(GL_TRIANGLES,0,6,count);}
 glBindBuffer(GL_ARRAY_BUFFER,0);glBindVertexArray(0);glUseProgram(0);error=glGetError();return error==GL_NO_ERROR?Status::OK:Status::GL_DRAW_FAILED;
}
}
