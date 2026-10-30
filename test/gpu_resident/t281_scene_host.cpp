#include "m1/scene.h"
#include "foundation.h"
#include "angles_generated.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <string>
extern "C" void GL_APIENTRY glFinish(){auto f=reinterpret_cast<PFNGLFINISHPROC>(eglGetProcAddress("glFinish"));assert(f);f();}
extern "C" void GL_APIENTRY glCopyBufferSubData(GLenum a,GLenum b,GLintptr c,GLintptr d,GLsizeiptr n){auto f=reinterpret_cast<PFNGLCOPYBUFFERSUBDATAPROC>(eglGetProcAddress("glCopyBufferSubData"));assert(f);f(a,b,c,d,n);}
extern "C" void GL_APIENTRY glReadPixels(GLint x,GLint y,GLsizei w,GLsizei h,GLenum format,GLenum type,void* p){auto f=reinterpret_cast<PFNGLREADPIXELSPROC>(eglGetProcAddress("glReadPixels"));assert(f);f(x,y,w,h,format,type,p);}
extern "C" void GL_APIENTRY glUniform1f(GLint location,GLfloat value){auto f=reinterpret_cast<PFNGLUNIFORM1FPROC>(eglGetProcAddress("glUniform1f"));assert(f);f(location,value);}
extern "C" void GL_APIENTRY glUniform4fv(GLint location,GLsizei count,const GLfloat* v){auto f=reinterpret_cast<PFNGLUNIFORM4FVPROC>(eglGetProcAddress("glUniform4fv"));assert(f);f(location,count,v);}
namespace grs {FoundationInfo GetFoundationInfo(){return {"FULL_M1","FULL_M1","FULL_M1","unused",kAngleSha256,"unused",0,false,false};}}
using namespace grs;
struct Pixels {std::array<std::vector<uint8_t>,7> layers;};
void capture(uint32_t layer,GLuint fbo,void* user){auto& p=*static_cast<Pixels*>(user);glBindFramebuffer(GL_FRAMEBUFFER,fbo);p.layers[layer].resize(256*256*4);glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,p.layers[layer].data());assert(glGetError()==GL_NO_ERROR);}
int main(int argc,char** argv){uint32_t K=argc>1?uint32_t(std::stoul(argv[1])):192,Kw=argc>2?uint32_t(std::stoul(argv[2])):128;using Platform=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);auto platform=reinterpret_cast<Platform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(platform);EGLint args[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto d=platform(0x3202,nullptr,args);EGLint major,minor;assert(eglInitialize(d,&major,&minor));EGLConfig cfg;EGLint n,attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(d,attrs,&cfg,1,&n)&&n);assert(eglBindAPI(EGL_OPENGL_ES_API));EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE},pa[]={EGL_WIDTH,256,EGL_HEIGHT,256,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));
 Identity id;id.run={1,2};id.owner={2,3};id.context={1,4};id.surface={1,5};id.geometry={1,6};id.config={1,7};auto hash=[](const char* s,std::array<uint32_t,8>& v){for(uint32_t i=0;i<32;i++){char h[]={s[2*i],s[2*i+1],0};v[i/4]|=uint32_t(std::stoul(h,nullptr,16))<<(8*(i%4));}};hash(m1::manifestSha,id.layoutSha);hash(m1::FullM1ShaderSha(),id.shaderSha);hash(kAngleSha256,id.angleSha);m1::QuantumParameters physics;physics.kappa=0;m1::FullM1Transport gpu;assert(gpu.Create(K,Kw,id,physics)==TransportCode::OK);m1::Scene scene;m1::SceneOptics optics;uint32_t frame=0,nonce=0,sequence=0,pub=1,render=0;int passed=0;
 auto poll=[&](){AcceptedRoot a;TransportCode c;auto start=std::chrono::steady_clock::now();do{c=gpu.Poll(a);}while(c==TransportCode::BUSY&&std::chrono::steady_clock::now()-start<std::chrono::seconds(180));std::cout<<"POLL "<<int(c)<<" reason="<<gpu.LastCompletion()[29]<<std::endl;assert(c==TransportCode::OK);pub=1-a.publication;return a;};
 auto request=[&](uint32_t kind){RootRequest r;r.kind=kind;r.frame={++frame,0};r.nonce={++nonce,1};r.control={nonce,0};r.publicationSlot=pub;r.eventArena=frame&1;r.quantumOrdinal=kind==2?1:0;return r;};
 auto source=[&](uint32_t kind,double x,double y,double mass){std::array<uint32_t,32> w{};w[0]=1129534023;w[1]=2;w[2]=kind;w[4]=1;w[5]=2;w[6]=++sequence;w[8]=w[10]=w[14]=w[15]=w[24]=w[27]=1;w[12]=1;w[13]=6;w[28]=1;w[29]=7;std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&mass,8);assert(gpu.SubmitSource(w,request(1))==TransportCode::OK);return poll();};
 auto quantum=[&](){assert(gpu.SubmitQuantum(physics,request(2))==TransportCode::OK);return poll();};
 auto geometry=[&](){ResidentSceneFrame f;f.run=id.run;f.context=id.context;f.surface=id.surface;f.geometry=id.geometry;f.renderFrame=++render;f.width=f.height=256;f.boxWidth=f.boxHeight=256;return f;};
 auto draw=[&](const AcceptedRoot& a,const char* name){Pixels px;auto f=geometry();auto result=scene.Draw(gpu,a.publication,a.publicationGeneration,f,[](void*){return true;},&px,optics,capture);std::cout<<"DRAW "<<name<<" current="<<result.current<<" stage="<<result.stage<<" GL="<<result.rawGl<<std::endl;assert(result.current&&result.swapSucceeded&&result.requestedToken==result.renderedToken&&result.renderedToken.find("GRS_GPU_SCENE_Q20_M1_V2|")==0);for(uint32_t i=0;i<7;i++){assert(px.layers[i].size()==256*256*4);std::string path=".codex/local-test/t281/"+std::string(name)+"-"+std::to_string(K)+"-"+std::to_string(Kw)+"-layer"+std::to_string(i)+".rgba";std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(px.layers[i].data()),px.layers[i].size());}glFinish();passed+=8;return px;};
 auto empty=quantum();auto dry=draw(empty,"empty");assert(dry.layers[2]==dry.layers[3]&&dry.layers[3]==dry.layers[4]&&dry.layers[4]==dry.layers[6]);passed++;
 auto water=source(1,64.5,0,64);auto deep=draw(water,"deep");assert(deep.layers[2]!=deep.layers[3]&&deep.layers[3]!=deep.layers[4]);passed++;
 auto airborne=source(1,64.5,150,4);auto flight=draw(airborne,"flight");assert(flight.layers[4]!=flight.layers[6]);passed++;
 // A second near-surface packet arrives; the high one remains the only glyph.
 source(1,66.5,0,64);source(1,66.5,64.0001,4);auto arrival=quantum();assert(arrival.completion[156]==1&&arrival.completion[90]==1);auto after=draw(arrival,"arrival");passed++;
 auto last=scene.LastGoodToken();auto invalid=geometry();invalid.context.lo++;auto bad=scene.Draw(gpu,arrival.publication,arrival.publicationGeneration,invalid,[](void*){return true;},nullptr,optics);assert(!bad.current&&scene.LastGoodToken()==last);glFinish();passed++;
 auto f=geometry();bad=scene.Draw(gpu,arrival.publication,arrival.publicationGeneration,f,[](void*){return false;},nullptr,optics);assert(!bad.current&&!bad.swapSucceeded&&scene.LastGoodToken()==last);glFinish();passed++;
 auto old=arrival;auto next=quantum();bad=scene.Draw(gpu,old.publication,old.publicationGeneration,geometry(),[](void*){return true;},nullptr,optics);assert(!bad.current&&std::string(bad.stage)=="GRS_SCENE_STALE_PUBLICATION");passed++;
 auto invalidOptics=optics;invalidOptics.values[0]=-1;bad=scene.Draw(gpu,next.publication,next.publicationGeneration,geometry(),[](void*){return true;},nullptr,invalidOptics);assert(!bad.current);passed++;
 auto sand=source(0,60.5,150,.125);auto active=draw(sand,"sand");assert(active.layers[1]!=active.layers[2]);passed++;
 auto stats=gpu.Stats();assert(stats.mappedBytes==gpu.Owner().AcceptedCount()*1024);scene.Reset(true);assert(gpu.Release()==TransportCode::OK);std::cout<<"{\"revision\":\"T281_REAL_GL_SCENE_V1\",\"K\":"<<K<<",\"Kw\":"<<Kw<<",\"passed\":"<<passed<<",\"failed\":0,\"execution\":\"PC_SWIFTSHADER\",\"device\":\"NOT_RUN\"}\n";
}
