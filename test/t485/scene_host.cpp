#include "m1/scene.h"
#include "foundation.h"
#include <cassert>
#include <fstream>
#include <iostream>
namespace grs {
FoundationInfo GetFoundationInfo(){return {"HOST","HOST","HOST","HOST","HOST","HOST",0,false,false};}
namespace m1 {
// Existing FullM1HostAccess friend seam. This freezes render input and lease
// identity, without asserting that a compute root was executed by this fixture.
struct FullM1HostAccess {
 static void Set(FullM1Transport& gpu,const Identity& id,bool mixed) {
  constexpr uint32_t K=192,Kw=128;
  auto& s=gpu.source;auto& o=s.owner;
  if(s.context==EGL_NO_CONTEXT) {
   assert(o.Initialize(id,K,Kw)==ControlCode::OK);
   s.context=eglGetCurrentContext();s.display=eglGetCurrentDisplay();
   s.surface=eglGetCurrentSurface(EGL_DRAW);s.thread=std::this_thread::get_id();
   s.ready=true;s.K=K;s.Kw=Kw;glGenBuffers(1,&s.pubs[0]);
  }
  s.Retire();assert(!o.HasLeases());
  auto& h=o.header;h[72]=1;h[73]=0;h[41]=mixed?1:0;h[43]=K-h[41];
  h[42]=mixed?1:0;h[44]=Kw-h[42];
  auto& p=o.pubs[0];p={};p.identity=id;p.generation={1,0};p.available=true;
  p.run=id.run;p.context=id.context;p.surface=id.surface;p.source={2,0};
  p.physics={3,0};p.control={4,0};p.frame={5,0};p.nonce={6,0};
  std::vector<uint32_t> data(16*K+20*Kw+2048+256);
  auto bits=[](float v){uint32_t out;std::memcpy(&out,&v,4);return out;};
  if(mixed) {
   data[3*K]=131072;data[8*K]=bits(60.5f);data[9*K]=bits(100.f);
   data[10*K]=bits(1.5f);data[12*K]=1;
   const size_t w=16*K;data[w]=2097152;data[w+16*Kw]=bits(80.5f);data[w+17*Kw]=bits(80.f);
   const size_t c=w+20*Kw;
   for(unsigned i=40;i<85;++i){data[c+8*128+i]=bits(12.f);data[c+9*128+i]=bits(20.f);data[c+10*128+i]=bits(8.f);}
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,s.pubs[0]);
  glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(data.size()*4),data.data(),GL_STATIC_DRAW);
  assert(glGetError()==GL_NO_ERROR);
 }
 static void Drain(FullM1Transport& gpu){glFinish();gpu.source.Retire();assert(!gpu.source.owner.HasLeases());}
 static uint32_t Revision(FullM1Transport& gpu,uint32_t next){auto& value=gpu.source.owner.header[1];const auto old=value;value=next;return old;}
 static void Available(FullM1Transport& gpu,bool value){gpu.source.owner.pubs[0].available=value;}
 static void End(FullM1Transport& gpu){Drain(gpu);glDeleteBuffers(1,&gpu.source.pubs[0]);gpu.source.ContextLost();}
};
}}
using namespace grs;
using namespace grs::programcache;
struct Pixels {
 int width=0,height=0;bool failDraw=false,late=false,allowed=true;uint64_t clock=1;
 m1::RootIsolation gate;RootRequest root;
 std::array<std::vector<uint8_t>,8> layers;
 Pixels(){gate.context=this;gate.deadline=100;gate.authorize=[](void* p,const RootRequest&,m1::RootCut){const auto& v=*static_cast<Pixels*>(p);return v.allowed&&v.clock<=v.gate.deadline;};}
};
void Capture(uint32_t layer,GLuint fbo,void* value) {
 auto& p=*static_cast<Pixels*>(value);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 p.layers[layer].resize(size_t(p.width)*p.height*4);
 glReadPixels(0,0,p.width,p.height,GL_RGBA,GL_UNSIGNED_BYTE,p.layers[layer].data());
 assert(glGetError()==GL_NO_ERROR);
 if(p.failDraw&&layer==0)glEnable(0xffffffffu);
 if(p.late&&layer==5)p.clock=101;
}
bool Swap(void* value) {
 auto& p=*static_cast<Pixels*>(value);
 if(!p.gate.Check(p.root,m1::RootCut::Swap))return false;
 p.layers[7].resize(size_t(p.width)*p.height*4);
 glBindFramebuffer(GL_FRAMEBUFFER,0);
 glReadPixels(0,0,p.width,p.height,GL_RGBA,GL_UNSIGNED_BYTE,p.layers[7].data());
 return glGetError()==GL_NO_ERROR;
}
struct Context {
 EGLDisplay display;EGLConfig config;EGLSurface surface;EGLContext context;
 Context(){
  using Fn=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);
  auto fn=reinterpret_cast<Fn>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(fn);
  EGLint da[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};
  display=fn(0x3202,nullptr,da);assert(eglInitialize(display,nullptr,nullptr));
  EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE},n;
  assert(eglChooseConfig(display,attrs,&config,1,&n)&&n&&eglBindAPI(EGL_OPENGL_ES_API));
  EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE};
  EGLint sa[]={EGL_WIDTH,320,EGL_HEIGHT,256,EGL_NONE};
  context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);surface=eglCreatePbufferSurface(display,config,sa);
  assert(eglMakeCurrent(display,surface,surface,context));
 }
 ~Context(){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);}
};
int main(int argc,char** argv) {
 assert(argc==4);Context context;assert(Store().Configure(argv[1]));
 const std::string mode=argv[2],out=argv[3];packageSource="HOST_T485_"+mode;
 const auto descriptors=m1::ScenePrograms();
 const auto environment=Environment();
 const auto key=GraphicsKey(descriptors[0],environment);
 auto changed=descriptors[0];std::string vertex(changed.stages[0].bytes);vertex+="\n// key variation\n";
 changed.stages[0].bytes=vertex;assert(GraphicsKey(changed,environment)!=key);
 changed=descriptors[0];std::string fragment(changed.stages[1].bytes);fragment+="\n// key variation\n";
 changed.stages[1].bytes=fragment;assert(GraphicsKey(changed,environment)!=key);
 changed=descriptors[0];changed.bindingAbi="CHANGED";assert(GraphicsKey(changed,environment)!=key);
 assert(GraphicsKey(descriptors[0],environment+"changed")!=key);
 assert(GraphicsKey(descriptors[0],environment,"CHANGED_LINK_OUTPUT_ABI")!=key);
 changed=descriptors[0];changed.stages[1].stage=GL_VERTEX_SHADER;assert(!ValidGraphics(changed));
 m1::Scene scene;
 assert(scene.PreparePrograms(1,nullptr,nullptr,mode!="off"));
 assert(scene.LastGoodToken().empty());
 assert(graphicsRecords.size()==3);
 for(const auto& entry:graphicsRecords) {
  assert(entry.second.prepared);
  if(mode=="warm")assert(entry.second.compileCalls==0&&entry.second.route=="BINARY_LOAD");
  if(mode=="cold")assert(entry.second.compileCalls==2&&entry.second.route=="SOURCE_COMPILE");
 }
 Store().Drain();
 Identity id;id.run=id.owner=id.context=id.surface=id.geometry=id.config={1,0};
 m1::FullM1Transport gpu;m1::SceneOptics optics;uint64_t frame=0;
 for(const std::string name:{"empty","mixed","mirror","viewport"}) {
  m1::FullM1HostAccess::Set(gpu,id,name!="empty");
  ResidentSceneFrame f;f.run=id.run;f.context=id.context;f.surface=id.surface;f.geometry=id.geometry;f.renderFrame=++frame;
  f.width=name=="viewport"?320:256;f.height=name=="viewport"?192:256;
  f.boxWidth=float(f.width);f.boxHeight=float(f.height);f.mirror=name=="mirror";
  Pixels pixels;pixels.width=f.width;pixels.height=f.height;
  const auto r=scene.Draw(gpu,0,{1,0},f,Swap,&pixels,optics,Capture);
  if(!r.current)std::cerr<<"scene failure "<<r.stage<<" GL="<<r.rawGl<<std::endl;
  assert(r.current&&r.swapSucceeded&&r.renderedToken==r.requestedToken);
  m1::FullM1HostAccess::Drain(gpu);
  if(name=="empty")assert(pixels.layers[2]==pixels.layers[3]&&pixels.layers[4]==pixels.layers[6]);
  else assert(pixels.layers[1]!=pixels.layers[2]&&pixels.layers[2]!=pixels.layers[3]&&pixels.layers[4]!=pixels.layers[6]);
  for(size_t layer=0;layer<pixels.layers.size();++layer) {
   assert(pixels.layers[layer].size()==size_t(f.width)*f.height*4);
   const std::string file=out+"/"+name+"-"+std::to_string(layer)+".rgba";
   std::ofstream raw(file,std::ios::binary);raw.write(reinterpret_cast<const char*>(pixels.layers[layer].data()),pixels.layers[layer].size());
  }
  std::ofstream token(out+"/"+name+".token");token<<r.renderedToken;
 }
 ResidentSceneFrame f;f.run=id.run;f.context=id.context;f.surface=id.surface;f.geometry=id.geometry;
 f.width=f.height=256;f.boxWidth=f.boxHeight=256;
 Pixels pixels;pixels.width=pixels.height=256;const auto last=scene.LastGoodToken();
 const auto revision=m1::FullM1HostAccess::Revision(gpu,99);
 f.renderFrame=++frame;auto bad=scene.Draw(gpu,0,{1,0},f,Swap,&pixels,optics,Capture);
 assert(!bad.current&&scene.LastGoodToken()==last);m1::FullM1HostAccess::Revision(gpu,revision);
 m1::FullM1HostAccess::Available(gpu,false);
 f.renderFrame=++frame;bad=scene.Draw(gpu,0,{1,0},f,Swap,&pixels,optics,Capture);
 assert(!bad.current&&scene.LastGoodToken()==last);m1::FullM1HostAccess::Available(gpu,true);
 f.renderFrame=++frame;bad=scene.Draw(gpu,0,{2,0},f,Swap,&pixels,optics,Capture);
 assert(!bad.current&&scene.LastGoodToken()==last);
 f.renderFrame=++frame;f.context={2,0};bad=scene.Draw(gpu,0,{1,0},f,Swap,&pixels,optics,Capture);
 assert(!bad.current&&scene.LastGoodToken()==last);m1::FullM1HostAccess::Drain(gpu);f.context=id.context;
 f.renderFrame=++frame;pixels.allowed=false;bad=scene.Draw(gpu,0,{1,0},f,Swap,&pixels,optics,Capture);
 assert(!bad.current&&scene.LastGoodToken()==last);m1::FullM1HostAccess::Drain(gpu);
 f.renderFrame=++frame;pixels.allowed=true;pixels.late=true;bad=scene.Draw(gpu,0,{1,0},f,Swap,&pixels,optics,Capture);
 assert(!bad.current&&scene.LastGoodToken()==last);m1::FullM1HostAccess::Drain(gpu);
 f.renderFrame=++frame;pixels.late=false;pixels.clock=1;pixels.allowed=true;pixels.failDraw=true;bad=scene.Draw(gpu,0,{1,0},f,Swap,&pixels,optics,Capture);
 assert(!bad.current&&scene.LastGoodToken()==last);m1::FullM1HostAccess::Drain(gpu);
 scene.Reset(true);assert(graphicsRecords.empty());
 m1::FullM1HostAccess::End(gpu);
 // Invalidated candidate cleanup, including successful earlier role artifacts.
 m1::Scene partial;int callbacks=0;
 const auto valid=[](void* p){return ++*static_cast<int*>(p)<5;};
 assert(!partial.PreparePrograms(2,valid,&callbacks));assert(graphicsRecords.empty());
 Store().Drain();
 std::cout<<"{\"schema\":\"T485_REAL_SCENE_PIXELS_V1\",\"mode\":\""<<mode<<"\",\"scenarios\":4,\"layers\":8,\"negativeCases\":8,\"device\":\"NOT_RUN\",\"publication\":\"FROZEN_HOST_FIXTURE_REAL_LEASE_AND_GL_BUFFER\"}"<<std::endl;
 return 0;
}
