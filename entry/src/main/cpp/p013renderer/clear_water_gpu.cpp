#include "clear_water_gpu.h"
#include "clear_water_shaders.h"
namespace p013 {
namespace {
const char* vertex=R"S(#version 300 es
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)S";
GLuint Compile(GLenum type,const char* source){GLuint s=glCreateShader(type);glShaderSource(s,1,&source,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
GLuint Program(){GLuint v=Compile(GL_VERTEX_SHADER,vertex),f=Compile(GL_FRAGMENT_SHADER,kClearWaterFragment);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;}
void Texture(GLuint tex,int rows){glBindTexture(GL_TEXTURE_2D,tex);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F,128,rows);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}
}
void ClearWaterGlGpu::Reset(bool current){targets.Reset(current);dual.Reset(current);if(current){if(program)glDeleteProgram(program);if(vao)glDeleteVertexArrays(1,&vao);glDeleteTextures(2,profile);if(sand)glDeleteTextures(1,&sand);}program=vao=profile[0]=profile[1]=sand=0;context=0;packet=nullptr;active=0;}
Status ClearWaterGlGpu::Begin(const ClearWaterPacket& p){
 if(context!=p.context||!program){Reset(context==p.context);context=p.context;program=Program();if(!program)return Status::SHADER_COMPILE_FAILED;glGenVertexArrays(1,&vao);glGenTextures(2,profile);Texture(profile[0],2);Texture(profile[1],2);glGenTextures(1,&sand);Texture(sand,1);}
 auto code=targets.Ensure(int(p.frame.surfaceWidthPx),int(p.frame.surfaceHeightPx),p.context);if(code!=Status::OK)return code;
 for(int c=0;c<128;c++){const auto a=p.profile.data()+8*c;float* row=staging.data()+4*c;row[0]=a[0];row[1]=a[1];row[2]=a[2];row[3]=a[4];float* r1=row+512;r1[0]=a[5];r1[1]=a[3];r1[2]=a[6];r1[3]=a[7];bed[4*c]=a[0];bed[4*c+1]=a[2];bed[4*c+2]=a[4];bed[4*c+3]=a[4];}
 glBindTexture(GL_TEXTURE_2D,profile[active]);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,128,2,GL_RGBA,GL_FLOAT,staging.data());glBindTexture(GL_TEXTURE_2D,sand);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,128,1,GL_RGBA,GL_FLOAT,bed.data());packet=&p;return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_RESOURCE_FAILED;
}
Status ClearWaterGlGpu::Layer(int layer,const ClearWaterPacket& p){
 const auto& f=p.frame;glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,targets.width,targets.height);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 if(layer==1||layer==2){DualVisual v;v.radiusPx=1.5;v.colors={.72,.57,.35,.72,.57,.35,.72,.57,.35,.72,.57,.35};return dual.DrawTarget(f,sand,p.xy.data(),p.meta.data(),p.count,p.capacity,layer==2,v,p.context,targets.fbos[targets.candidate],false,layer==1);}
 if(layer==0)glDisable(GL_BLEND);else{glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);}
 glUseProgram(program);glBindVertexArray(vao);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,profile[active]);glUniform1i(glGetUniformLocation(program,"uProfile"),0);glUniform2f(glGetUniformLocation(program,"uResolution"),f.surfaceWidthPx,f.surfaceHeightPx);glUniform4f(glGetUniformLocation(program,"uBox"),f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx);glUniform1i(glGetUniformLocation(program,"uMirror"),f.visualInverted);glUniform1i(glGetUniformLocation(program,"uLayer"),layer);
 float optics[16];for(int i=0;i<16;i++)optics[i]=p.optics[i];glUniform4fv(glGetUniformLocation(program,"uOptics"),4,optics);glDrawArrays(GL_TRIANGLES,0,3);return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_DRAW_FAILED;
}
Status ClearWaterGlGpu::Present(){if(!packet)return Status::NOT_READY;glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,targets.width,targets.height);glDisable(GL_BLEND);glUseProgram(program);glBindVertexArray(vao);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,targets.textures[targets.candidate]);glUniform1i(glGetUniformLocation(program,"uFrame"),1);glUniform1i(glGetUniformLocation(program,"uLayer"),6);glDrawArrays(GL_TRIANGLES,0,3);return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_DRAW_FAILED;}
}
