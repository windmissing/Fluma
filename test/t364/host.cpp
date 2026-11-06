#include "single_sand_pass.h"
#include "material_uploader.h"
#include <EGL/eglext.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
using namespace p013;
static EGLDisplay display;
static EGLSurface surface;
static bool failSwap=false;
static std::array<uint8_t,4> bed{},air{},background{};
Status EglContext::Swap(uint64_t){
 if(failSwap){failSwap=false;error=EGL_BAD_SURFACE;return Status::SWAP_FAILED;}
 glReadPixels(32,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,bed.data());
 glReadPixels(32,40,1,1,GL_RGBA,GL_UNSIGNED_BYTE,air.data());
 glReadPixels(16,40,1,1,GL_RGBA,GL_UNSIGNED_BYTE,background.data());
 assert(glGetError()==GL_NO_ERROR);
 // Pbuffer pixels are real GLES output. The compositor's window swap
 // outcome is injected here, since this host has no XComponent window.
 return Status::OK;
}
static std::string hash(const void* p,size_t n){return MaterialSha256(static_cast<const uint8_t*>(p),n);}
static void configDigest(SingleSandConfig& c){
 std::string text;
 for(const std::string& s:{std::string(SingleSandVersions),std::to_string(c.revision),c.generatorHash,c.grainHash})text+=std::to_string(s.size())+":"+s;
 const char* hex="0123456789abcdef";
 for(double d:c.values){uint64_t bits;std::memcpy(&bits,&d,8);for(int i=0;i<8;i++){unsigned b=(bits>>(8*i))&255;text+=hex[b>>4];text+=hex[b&15];}}
 c.digest=hash(text.data(),text.size());
}
static void packetHash(SingleSandPacket& p){
 p.displayHash=hash(p.profile.data(),512);p.pbHash=hash(p.profile.data()+128,512);
 std::array<uint8_t,8192> data{};
 std::memcpy(data.data(),p.xy.data(),p.count*8);std::memcpy(data.data()+p.count*8,p.meta.data(),p.count*8);
 p.particleHash=hash(data.data(),p.count*16);p.token=SingleIdentity(p);
}
int main(){
 display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
 EGLConfig cfg;EGLint count;assert(eglChooseConfig(display,attrs,&cfg,1,&count)&&count);
 assert(eglBindAPI(EGL_OPENGL_ES_API));const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
 EGLContext ctx=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);assert(ctx!=EGL_NO_CONTEXT);
 const EGLint sa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
 surface=eglCreatePbufferSurface(display,cfg,sa);assert(surface!=EGL_NO_SURFACE);assert(eglMakeCurrent(display,surface,surface,ctx));
 EglContext egl;glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_BACK,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&egl.attachmentEncoding);
 EGLint colorspace=0;assert(eglQuerySurface(display,surface,EGL_GL_COLORSPACE_KHR,&colorspace));egl.actualColorSpace=colorspace;
 const char* extensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
 egl.writeControlSupported=extensions&&std::strstr(extensions,"GL_EXT_sRGB_write_control");
 if(egl.writeControlSupported)glEnable(0x8DB9);
 egl.srgbWriteEnabled=!egl.writeControlSupported||glIsEnabled(0x8DB9);
 assert(egl.attachmentEncoding==GL_SRGB);
 SingleSandPass pass;SingleSandConfig c;
 c.values={145,168,111,16,16,36,2,2,0,0,8,-.35,0,.82,0,0,.60,.95,20261022};c.revision=c.session=1;
 std::array<uint8_t,65536> tile;tile.fill(128);c.grainHash=hash(tile.data(),tile.size());c.generatorHash=std::string(64,'a');configDigest(c);
 assert(std::string(c.Validate())=="OK");auto invalid=c;invalid.values[8]=NAN;assert(std::string(invalid.Validate())=="SINGLE_CONFIG_INVALID");
 pass.pendingConfig=c;
 tile[0]=127;assert(std::string(pass.Stage(1,c.digest,tile.data()))=="SINGLE_ASSET_HASH");tile[0]=128;
 assert(std::string(pass.Stage(1,c.digest,tile.data()))=="OK");
 SingleSandPacket p;p.session=p.surface=p.context=p.geometry=p.continuity=p.epoch=p.commit=p.frame=p.certificate=p.seal=p.revision=p.generation=1;
 p.run="host";p.kind="RESET";p.digest=c.digest;p.grainHash=c.grainHash;p.rawHash=std::string(64,'0');
 p.width=p.height=64;p.logicalW=p.logicalH=64;p.box={0,0,64,64};p.capacity=256;p.slots=2;p.count=1;p.enabled=true;
 p.xy[0]=65;p.xy[1]=162;p.meta[0]=256;p.meta[1]=1;for(int i=0;i<128;i++)p.profile[i]=64;packetHash(p);
 assert(std::string(p.Validate())=="OK");auto bad=p;bad.meta[0]=257;assert(std::string(bad.Validate())=="UNSUPPORTED_MATERIAL_SET");
 bad=p;bad.logicalW=63;assert(std::string(bad.Validate())=="SINGLE_GEOMETRY_UNSUPPORTED");
 bad=p;bad.profile[0]=63;assert(std::string(bad.Validate())=="SINGLE_PUBLICATION_MISMATCH");
 auto r=pass.Draw(egl,p);std::cout<<r.reason<<" "<<r.stage<<" GL="<<r.rawGl<<std::endl;assert(r.state=="CURRENT"&&r.actual==p.token);
 const std::array<uint8_t,4> sand={145,168,111,255},bg={16,16,36,255};
 const std::array<uint8_t,4> sandLinear={72,100,40,255},bgLinear={1,1,4,255};
 for(const auto& pixel:{bed,air,background}){for(auto byte:pixel)std::cout<<int(byte)<<" ";std::cout<<std::endl;}
 assert((bed==sand&&air==sand&&background==bg)||(bed==sandLinear&&air==sandLinear&&background==bgLinear));
 const auto pbufferBed=bed;
 glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
 std::array<uint8_t,4> stored{};
 glReadPixels(32,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,stored.data());
 assert(glGetError()==GL_NO_ERROR&&stored==sand);
 glReadPixels(32,40,1,1,GL_RGBA,GL_UNSIGNED_BYTE,stored.data());
 assert(glGetError()==GL_NO_ERROR&&stored==sand);
 glReadPixels(16,40,1,1,GL_RGBA,GL_UNSIGNED_BYTE,stored.data());
 assert(glGetError()==GL_NO_ERROR&&stored==bg);
 glBindFramebuffer(GL_FRAMEBUFFER,0);
 auto old=p.token;p.frame++;packetHash(p);failSwap=true;r=pass.Draw(egl,p);
 assert(r.state=="STALE"&&r.actual==old&&r.rawEgl==EGL_BAD_SURFACE);
 pass.Reset(true);p.context++;p.frame++;packetHash(p);r=pass.Draw(egl,p);
 assert(r.state=="CURRENT"&&r.uploaded==65536+1024+16&&bed==pbufferBed);
 p.frame++;packetHash(p);r=pass.Draw(egl,p);assert(r.state=="CURRENT"&&r.uploaded==1024+16);
 c.revision=2;c.values[8]=.08;c.values[14]=.10;c.values[15]=.16;
 tile.fill(255);c.grainHash=hash(tile.data(),tile.size());configDigest(c);
 assert(std::string(c.Validate())=="OK");pass.pendingConfig=c;
 assert(std::string(pass.Stage(2,c.digest,tile.data()))=="OK");
 p.frame++;p.generation=2;p.revision=2;p.digest=c.digest;p.grainHash=c.grainHash;
 p.xy[1]=34;p.meta[0]=512;packetHash(p);
 r=pass.Draw(egl,p);assert(r.state=="CURRENT"&&r.actualDigest==c.digest&&r.occludedCenters==0);
 glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
 std::array<uint8_t,4> grainPixel{},contactPixel{};
 glReadPixels(16,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,grainPixel.data());
 glReadPixels(32,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,contactPixel.data());
 assert(glGetError()==GL_NO_ERROR&&grainPixel[0]>sand[0]&&contactPixel[0]>grainPixel[0]);
 glBindFramebuffer(GL_FRAMEBUFFER,0);
 p.frame++;p.meta[0]=256;packetHash(p);
 r=pass.Draw(egl,p);assert(r.state=="CURRENT"&&r.occludedCenters==1);
 glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
 glReadPixels(32,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,contactPixel.data());
 assert(glGetError()==GL_NO_ERROR&&contactPixel==grainPixel);
 glBindFramebuffer(GL_FRAMEBUFFER,0);
 p.frame++;p.count=0;p.profile.fill(0);packetHash(p);
 r=pass.Draw(egl,p);assert(r.state=="CURRENT"&&std::string(r.bedSupport)=="EMPTY_BED");
 p.frame++;for(int i=0;i<128;i++)p.profile[i]=.15f;packetHash(p);
 r=pass.Draw(egl,p);assert(r.state=="CURRENT"&&std::string(r.bedSupport)=="SUBPIXEL_BED");
 pass.Reset(true);
 std::cout<<"PASS config/hash/geometry/phase, actual GLES sRGB FBO storage and pbuffer readback, injected swap failure stale, context reupload and warm bytes"<<std::endl;
}
