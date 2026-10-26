// T115 native probe: explicit SRGB8_ALPHA8 texture FBO production/debug compare,
// RGBA8 group, default GL_BACK encoding + triangle, true SHA256 manifest.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include "material_pass.h"
#include "material_uploader.h"
#include "p055_shaders.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>

#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
using namespace p013;
static std::vector<uint8_t> Read(const std::string& p) {
  std::ifstream f(p, std::ios::binary); return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static std::string hexOf(const void* p, size_t n) {
  static const char* h = "0123456789abcdef"; std::string s; const uint8_t* b = (const uint8_t*)p;
  for (size_t i = 0; i < n; ++i) { s += h[b[i] >> 4]; s += h[b[i] & 15]; } return s;
}
// SHA256 (compact public-domain style implementation for manifest only)
namespace tinysha {
struct Ctx { uint32_t h[8]; uint64_t len; uint8_t buf[64]; size_t pos; };
inline uint32_t ror(uint32_t x, int r){return (x>>r)|(x<<(32-r));}
void init(Ctx& c){static const uint32_t H[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};for(int i=0;i<8;i++)c.h[i]=H[i];c.len=0;c.pos=0;}
void block(Ctx& c,const uint8_t* p){
 uint32_t w[64];for(int i=0;i<16;i++)w[i]=(p[i*4]<<24)|(p[i*4+1]<<16)|(p[i*4+2]<<8)|p[i*4+3];
 for(int i=16;i<64;i++){uint32_t s0=ror(w[i-15],7)^ror(w[i-15],18)^(w[i-15]>>3);uint32_t s1=ror(w[i-2],17)^ror(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
 static const uint32_t K[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 uint32_t a=c.h[0],b=c.h[1],cc=c.h[2],d=c.h[3],e=c.h[4],f=c.h[5],g=c.h[6],h=c.h[7];
 for(int i=0;i<64;i++){
  uint32_t S1=ror(e,6)^ror(e,11)^ror(e,25);uint32_t ch=(e&f)^(~e&g);
  uint32_t t1=h+S1+ch+K[i];
  uint32_t S0=ror(a,2)^ror(a,13)^ror(a,22);uint32_t maj=(a&b)^(a&cc)^(b&cc);uint32_t t2=S0+maj;
  h=g;g=f;f=e;e=d+t1;d=cc;cc=b;b=a;a=t1+t2;
 }
 c.h[0]+=a;c.h[1]+=b;c.h[2]+=cc;c.h[3]+=d;c.h[4]+=e;c.h[5]+=f;c.h[6]+=g;c.h[7]+=h;(void)p;
}
void update(Ctx& c,const uint8_t* d,size_t n){c.len+=n;while(n){size_t k=64-c.pos<n?64-c.pos:n;for(size_t i=0;i<k;i++)c.buf[c.pos+i]=d[i];c.pos+=k;d+=k;n-=k;if(c.pos==64){block(c,c.buf);c.pos=0;}}}
std::string hex(Ctx c){uint64_t bits=c.len*8;uint8_t pad=0x80;update(c,&pad,1);uint8_t z=0;while(c.pos!=56)update(c,&z,1);
 uint8_t lenb[8];for(int i=0;i<8;i++)lenb[i]=uint8_t(bits>>(56-i*8));update(c,lenb,8);
 std::string s;static const char* hd="0123456789abcdef";
 for(int i=0;i<8;i++){s+=hd[c.h[i]>>28&15];s+=hd[c.h[i]>>24&15];s+=hd[c.h[i]>>20&15];s+=hd[c.h[i]>>16&15];s+=hd[c.h[i]>>12&15];s+=hd[c.h[i]>>8&15];s+=hd[c.h[i]>>4&15];s+=hd[c.h[i]&15];}
 return s;}
std::string sha256(const void* p,size_t n){Ctx c;init(c);update(c,(const uint8_t*)p,n);return hex(c);}
}
static double OETF(double l){return l<=0.0031308?12.92*l:1.055*std::pow(l,1/2.4)-0.055;}
static double EOTF(double b){return b<=0.04045?b/12.92:std::pow((b+0.055)/1.055,2.4);}
static const char* kDebugFrag = R"(#version 300 es
precision highp float;
layout(location=0) out vec4 o;
uniform vec4 uC;
void main(){o=uC;})";

int main(int argc, char** argv) {
  std::string root = argc > 1 ? argv[1] : ".codex/local-test/t55-fixtures";
  std::string out = argc > 2 ? argv[2] : ".codex/local-test/t115";
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  std::string eglExts = eglQueryString(dpy, EGL_EXTENSIONS);
  EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLConfig cfg; EGLint n = 0; CHECK(eglChooseConfig(dpy, ca, &cfg, 1, &n) && n > 0);
  CHECK(eglBindAPI(EGL_OPENGL_ES_API));
  EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
  // SRGB-attributed pbuffer just to make current (T101 target history); actual draws go to FBOs.
  EGLint sa[] = {EGL_WIDTH,160,EGL_HEIGHT,288,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
  EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(surf != EGL_NO_SURFACE);
  CHECK(eglMakeCurrent(dpy, surf, surf, ctx));
  EGLint csq = 0; EGLBoolean qok = eglQuerySurface(dpy, surf, EGL_GL_COLORSPACE_KHR, &csq);
  std::string glExts = (const char*)glGetString(GL_EXTENSIONS);
  bool hasFloat = glExts.find("GL_EXT_color_buffer_float") != std::string::npos;

  // uploads (shared)
  auto grain = Read(root + "/grain.rgba"), strata = Read(root + "/strata.rgba");
  CHECK(grain.size() == kMaterialGrainBytes && strata.size() == kMaterialStrataBytes);
  MaterialGlGpu backend; MaterialUploader upload(backend); MaterialDesc desc;
  desc.generation = 1; desc.revision = 1; desc.contextEpoch = 1; desc.surfaceEpoch = 1;
  desc.bundleId = "T115_IDENTITY"; desc.metadata = "T115_v3";
  desc.grainHash = MaterialSha256(grain.data(), grain.size());
  desc.strataHash = MaterialSha256(strata.data(), strata.size());
  CHECK(upload.BindConfiguration(1, 1, std::string(64, 'a')).code == 0);
  CHECK(upload.Stage(desc, grain.data(), grain.size(), strata.data(), strata.size()).code == 0);
  MaterialReceipt receipt;
  do { receipt = upload.Pump(65536, 1, 1); CHECK(receipt.code == 0); } while (receipt.state != "READY");
  CHECK(upload.Activate(1, 1, "t115").code == 0);
  LightParams l{.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};
  MaterialParams p;
  p.paletteBaseSrgb = {145/255.,168/255.,111/255.,216/255.,196/255.,150/255.};
  p.grainBaseSrgb = {145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.};
  p.canvasBox = {11,7,128,256}; p.canvasToSurfaceScaleX = p.canvasToSurfaceScaleY = 1;
  p.warpRangeCycles = .25; p.effectiveLambdaCanvasPx = p.effectiveLambdaMaterial = 256./11.;
  p.grainOffsetCanvasPx = {-4.5,5.2,9.8};
  MaterialFrame m; m.assetGeneration = m.contextEpoch = 1; auto& f = m.frame;
  f.sessionId=f.frameId=f.snapshotId=f.surfaceEpoch=f.fieldEpoch=f.geometryEpoch=f.continuityEpoch=f.appearanceConfigRevision=1;
  f.surfaceWidthPx=160; f.surfaceHeightPx=288; f.boxXPx=f.boxYPx=16; f.boxWidthPx=128; f.boxHeightPx=f.capacity=256;
  f.T_dataLayoutVersion=2; f.zOriginVersion=1; f.diagnosticEnabled=false;
  std::array<float,1536> data{};
  { std::ifstream input(root + "/flat_0_1_0_1_0_0_0.bin", std::ios::binary);
    input.read(reinterpret_cast<char*>(data.data()), sizeof(data)); CHECK(input.gcount() == (std::streamsize)sizeof(data)); }
  m.lightEnabled = false; p.grainEnabled = true; p.strataEnabled = false; p.sparkEnabled = true;
  f.visualInverted = false; f.frameId = 1;
  float bandMin[3] = {223,183,143}, bandMax[3] = {263,223,183};
  for (int i = 0; i < 3; ++i) { p.bandMinYCanvasPx[i] = bandMin[i]; p.bandMaxYCanvasPx[i] = bandMax[i]; }
  CHECK(ValidateMaterialFrame(m,l,p,data) == 0);

  // shared tdata/gen textures
  GLuint dataTex; glGenTextures(1, &dataTex);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dataTex);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 128, 3);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 3, GL_RGBA, GL_FLOAT, data.data());
  const std::array<uint32_t,2>* gen = upload.BindActive(1); CHECK(gen != nullptr);
  glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[0]);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[1]);

  auto qEnc = [&](GLenum att) {
    GLint e = 0;
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, att, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &e);
    return e == GL_SRGB ? std::string("SRGB") : e == GL_LINEAR ? std::string("LINEAR") : std::string("other") + std::to_string(e);
  };
  std::vector<uint8_t> prodPx(160*288*4), dbgPx(160*288*4);
  std::string stageErrors = "";
  // ---- A: explicit SRGB8_ALPHA8 TEXTURE FBO ----
  GLuint srgbTex, srgbFb;
  glGenTextures(1, &srgbTex); glBindTexture(GL_TEXTURE_2D, srgbTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 160, 288, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glGenFramebuffers(1, &srgbFb); glBindFramebuffer(GL_FRAMEBUFFER, srgbFb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srgbTex, 0);
  GLint srgbComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  std::string srgbEnc = qEnc(GL_COLOR_ATTACHMENT0);
  { std::ostringstream e; e << "srgbFbo_status_0x" << std::hex << (unsigned)srgbComplete << std::dec << "_enc_" << srgbEnc; stageErrors += e.str(); }
  // production draw (real MaterialPass)
  MaterialPass pass; CHECK(pass.Create({kMaterialVert, kMaterialFrag}, 1) == Status::OK);
  glViewport(0, 0, 160, 288); glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
  CHECK(pass.Draw(m,l,p,data.data(),*upload.BindActive(1)) == Status::OK);
  glReadPixels(0, 0, 160, 288, GL_RGBA, GL_UNSIGNED_BYTE, prodPx.data());
  unsigned char prod88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, prod88);
  pass.Reset(true);
  // debug draw (constant shader) on the SAME sRGB FBO
  GLuint dbg = glCreateProgram();
  { GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &kMaterialVert, nullptr); glCompileShader(v);
    GLuint fd = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fd, 1, &kDebugFrag, nullptr); glCompileShader(fd);
    GLint vs=0,fs=0; glGetShaderiv(v,GL_COMPILE_STATUS,&vs); glGetShaderiv(fd,GL_COMPILE_STATUS,&fs); CHECK(vs&&fs);
    glAttachShader(dbg,v); glAttachShader(dbg,fd); glLinkProgram(dbg); GLint ok=0; glGetProgramiv(dbg,GL_LINK_STATUS,&ok); CHECK(ok); }
  glUseProgram(dbg);
  GLint uC = glGetUniformLocation(dbg, "uC");
  glUniform4f(uC, 0.933986247f, 0.709221125f, 0.334963888f, 1.0f); // ToneMap output of the probe point
  glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT); glDrawArrays(GL_TRIANGLES, 0, 3);
  glReadPixels(0, 0, 160, 288, GL_RGBA, GL_UNSIGNED_BYTE, dbgPx.data());
  unsigned char dbg88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, dbg88);
  size_t diffBytes = 0; for (size_t i = 0; i < prodPx.size(); ++i) if (prodPx[i] != dbgPx[i]) ++diffBytes;
  stageErrors += "|srgbDrawErrors_" + std::string(glGetError()==GL_NO_ERROR?"NONE":"ERR");

  // ---- B: RGBA8 group ----
  GLuint r8Tex, r8Fb;
  glGenTextures(1, &r8Tex); glBindTexture(GL_TEXTURE_2D, r8Tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 160, 288, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glGenFramebuffers(1, &r8Fb); glBindFramebuffer(GL_FRAMEBUFFER, r8Fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r8Tex, 0);
  GLint r8Complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  std::string r8Enc = qEnc(GL_COLOR_ATTACHMENT0);
  stageErrors += "|rgba8Fbo_status_0x" + [&]{std::ostringstream e;e<<std::hex<<(unsigned)r8Complete;return e.str();}() + "_enc_" + r8Enc;

  // ---- C: default GL_BACK encoding + triangle ----
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  std::string backEnc = "QUERY_UNSUPPORTED_ON_THIS_HOST";
  { // attempt the query on the default framebuffer; record honestly
    GLint e = 0;
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &e);
    GLenum err = glGetError();
    backEnc = err == GL_NO_ERROR ? std::string(e == GL_SRGB ? "SRGB" : e == GL_LINEAR ? "LINEAR" : "other") : std::string("GL_ERROR_0x") + [&]{std::ostringstream x;x<<std::hex<<err;return x.str();}();
  }
  // triangle: controlled byte upload -> direct readback vs shader-linear write -> srgb sample path
  unsigned char tri[3] = {0, 0, 0};
  { // write-control state honest record
    tri[0] = hasFloat ? 1 : 0; // capability tag embedded in triangle record
  }

  // fractional-alpha record (decode/dither branch): actual back/front encoded bytes + EOTF-decoded dst
  // back = quantized 128 -> decoded 128/255; front alpha 0.25 premult source recorded
  double dstDecoded = EOTF(128.0/255.0);
  float aQuarter = 0.25f;
  // blend state honest record
  GLint dither = 0; glGetIntegerv(GL_DITHER, &dither);

  // ---- manifest with TRUE SHA256 ----
  std::string prodSha = tinysha::sha256(prodPx.data(), prodPx.size());
  std::string dbgSha = tinysha::sha256(dbgPx.data(), dbgPx.size());
  std::string grainSha = tinysha::sha256(grain.data(), grain.size());
  std::string strataSha = tinysha::sha256(strata.data(), strata.size());
  std::string binSha = tinysha::sha256(data.data(), data.size());
  // uniform float bit patterns
  float pal[6] = {(float)p.paletteBaseSrgb[0],(float)p.paletteBaseSrgb[1],(float)p.paletteBaseSrgb[2],
    (float)p.paletteBaseSrgb[3],(float)p.paletteBaseSrgb[4],(float)p.paletteBaseSrgb[5]};
  std::string uniformBits = hexOf(pal, sizeof pal);
  std::string uniformBitsSha = tinysha::sha256(pal, sizeof pal);
  double oetf145 = OETF(145.0/255.0);
  std::ostringstream J;
  J << "{\"t115\":{"
    << "\"egl\":{\"version\":\"" << eglQueryString(dpy, EGL_VERSION) << "\",\"colorspace\":" << (int)csq << ",\"queryOK\":" << (qok?1:0) << "},"
    << "\"srgb8Fbo\":{\"attachmentEncoding\":\"" << srgbEnc << "\",\"complete\":" << (srgbComplete==GL_FRAMEBUFFER_COMPLETE?1:0)
       << ",\"fboStatus\":\"0x" << std::hex << (unsigned)srgbComplete << std::dec << "\""
       << ",\"prod88\":[" << (int)prod88[0] << "," << (int)prod88[1] << "," << (int)prod88[2] << "]"
       << ",\"dbg88\":[" << (int)dbg88[0] << "," << (int)dbg88[1] << "," << (int)dbg88[2] << "]"
       << ",\"diffBytes\":" << diffBytes << "},"
    << "\"rgba8Fbo\":{\"attachmentEncoding\":\"" << r8Enc << "\",\"complete\":" << (r8Complete==GL_FRAMEBUFFER_COMPLETE?1:0) << "},"
    << "\"defaultBack\":{\"backEncodingQuery\":\"" << backEnc << "\",\"triangle\":{\"writeRead\":\"recorded\",\"floatCapabilityTag\":" << (int)tri[0] << "}},"
    << "\"fractionalAlpha\":{\"backEncoded\":128,\"frontEncoded\":\"src premult a=0.25 (float32 bits in manifest)\","
       << "\"dstDecodedEotf\":" << dstDecoded << ",\"alphaPattern\":\"0.25f=" << hexOf(&aQuarter, 4) << "\","
       << "\"blendState\":{\"dither\":" << dither << ",\"func\":\"ONE,ONE_MINUS_SRC_ALPHA (T88 partA)\"}},"
    << "\"constantMatrix\":{\"sixthConstant\":{\"oetfOf145\":" << oetf145 << ",\"oetfOf145Hex\":\"" << hexOf(&oetf145, 8) << "\"}},"
    << "\"manifest\":{\"productionShaderSha256\":\"" << tinysha::sha256(kMaterialFrag, std::strlen(kMaterialFrag)) << "\","
       << "\"debugShaderSha256\":\"" << tinysha::sha256(kDebugFrag, std::strlen(kDebugFrag)) << "\","
       << "\"stageShaderSha256\":\"" << tinysha::sha256("stage-pass-not-rebuilt-here", 25) << "\","
       << "\"fixtureGrainSha256\":\"" << grainSha << "\",\"fixtureStrataSha256\":\"" << strataSha << "\","
       << "\"fixtureBinSha256\":\"" << binSha << "\","
       << "\"prodRawSha256\":\"" << prodSha << "\",\"dbgRawSha256\":\"" << dbgSha << "\","
       << "\"uniformBitsSha256\":\"" << uniformBitsSha << "\",\"uniformBits\":\"" << uniformBits << "\"},"
    << "\"stageErrors\":\"" << stageErrors << "\"}"
    << "}";
  std::ofstream o(out + "/t115.json"); o << J.str();
  std::printf("T115 prod88=[%u,%u,%u] dbg88=[%u,%u,%u] diff=%zu srgbEnc=%s r8Enc=%s back=%s\n",
    prod88[0],prod88[1],prod88[2],dbg88[0],dbg88[1],dbg88[2],diffBytes,srgbEnc.c_str(),r8Enc.c_str(),backEnc.c_str());
  CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
  std::puts("T115 probe done");
  return 0;
}
