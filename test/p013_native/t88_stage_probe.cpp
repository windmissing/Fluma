// T88 Part B: (88,78) baseByte/texel/stage probe with a DEBUG MRT variant of the
// production material shader. Debug shader hash independent; production untouched.
// Reuses the T55 fixture pipeline (tdata/grain/strata uploads, real uniform set).
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include "material_pass.h"
#include "material_uploader.h"
#include "p055_shaders.h"
#include <cstdio>
#include <fstream>
#include <cstring>
#include <string>
#include <cmath>

#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
using namespace p013;

static std::vector<uint8_t> Read(const std::string& p) {
  std::ifstream f(p, std::ios::binary); return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// Debug fragment: identical math to production, plus stage capture attachment.
// outStage = uvec3(baseByte.b, grainByte.b, strataByte.b), uvec4.w = k
static const char* kDebugFragDecl = R"(#version 300 es
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
uniform vec4 u_light;
uniform vec2 u_lightDepth;
layout(location=0) out vec4 outColor;
layout(location=1) out uvec4 outStage;
vec3 preparedQ(vec3 c) {
  vec3 lo=c/12.92;
  vec3 hi=pow((c+0.055)/1.055,vec3(2.4));
  vec3 l=min(mix(hi,lo,lessThanEqual(c,vec3(0.04045))),vec3(1.0-1.0/4096.0));
  return l/(1.0-l);
}
vec3 preparedMaterial(vec3 baseByte, vec3 grainByte, vec3 grainBaseByte,
  vec3 strataByte, float c3Byte, vec3 strengths, float lmat, bool lightEnabled) {
  vec3 neutral=preparedQ(vec3(128.0/255.0));
  vec3 dc=strengths.y*(preparedQ(strataByte/255.0)-neutral)
    +strengths.z*(preparedQ(vec3(c3Byte/255.0))-neutral);
  vec3 db=strengths.x*(preparedQ(grainByte/255.0)-preparedQ(grainBaseByte/255.0));
  vec3 r=max(vec3(0.0),preparedQ(baseByte/255.0)+dc+db)*(lightEnabled?lmat:1.0);
  return r/(1.0+r);
}
vec4 D(float x,int row){
 float r=clamp(x/(u_box.z/128.0),0.0,127.0);int i=min(int(floor(r)),126);
 return mix(texelFetch(u_tdata,ivec2(i,row),0),texelFetch(u_tdata,ivec2(i+1,row),0),r-float(i));
}
int positiveMod(int x,int n){return (x%n+n)%n;}
void main(){
 vec2 p=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y);
 vec2 q=p-u_box.xy;outColor=vec4(0.0);outStage=uvec4(0u);
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
 outStage=uvec4(uint(baseByte.b),uint(grainByte.b),uint(strataByte.b),uint(k));
}
)";

int main(int argc, char** argv) {
  if (argc < 2) { std::printf("usage: t88_stage_probe <fixtureRoot>\n"); return 2; }
  std::string root = argv[1];
  std::string outDir = argc > 2 ? argv[2] : ".codex/local-test/t88";
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLConfig cfg; EGLint n = 0; CHECK(eglChooseConfig(dpy, ca, &cfg, 1, &n) && n > 0);
  CHECK(eglBindAPI(EGL_OPENGL_ES_API));
  EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
  EGLint sa[] = {EGL_WIDTH,160,EGL_HEIGHT,288,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
  EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(surf != EGL_NO_SURFACE);
  CHECK(eglMakeCurrent(dpy, surf, surf, ctx));

  auto grain = Read(root + "/grain.rgba"), strata = Read(root + "/strata.rgba");
  CHECK(grain.size() == kMaterialGrainBytes && strata.size() == kMaterialStrataBytes);
  MaterialGlGpu backend; MaterialUploader upload(backend); MaterialDesc desc;
  desc.generation = 1; desc.revision = 1; desc.contextEpoch = 1; desc.surfaceEpoch = 1;
  desc.bundleId = "T88_STAGE_PROBE"; desc.metadata = "T88_debug";
  desc.grainHash = MaterialSha256(grain.data(), grain.size());
  desc.strataHash = MaterialSha256(strata.data(), strata.size());
  CHECK(upload.BindConfiguration(1, 1, std::string(64, 'a')).code == 0);
  CHECK(upload.Stage(desc, grain.data(), grain.size(), strata.data(), strata.size()).code == 0);
  MaterialReceipt receipt;
  do { receipt = upload.Pump(65536, 1, 1); CHECK(receipt.code == 0); } while (receipt.state != "READY");
  CHECK(upload.Activate(1, 1, "t88-probe").code == 0);

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

  // fixture flat_0_1_0_1_0_0_0: on b c spark invert stress alt + bands
  std::array<float,1536> data{};
  { std::ifstream input(root + "/flat_0_1_0_1_0_0_0.bin", std::ios::binary);
    input.read(reinterpret_cast<char*>(data.data()), sizeof(data)); CHECK(input.gcount() == (std::streamsize)sizeof(data)); }
  m.lightEnabled = false; p.grainEnabled = true; p.strataEnabled = false; p.sparkEnabled = true;
  f.visualInverted = false; f.frameId = 1;
  float bandMin[3] = {223,183,143}, bandMax[3] = {263,223,183};
  for (int i = 0; i < 3; ++i) { p.bandMinYCanvasPx[i] = bandMin[i]; p.bandMaxYCanvasPx[i] = bandMax[i]; }
  CHECK(ValidateMaterialFrame(m,l,p,data) == 0);

  // Debug program: production vert + debug frag (MRT)
  GLuint prog = glCreateProgram();
  { GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &kMaterialVert, nullptr); glCompileShader(v);
    GLuint fd = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fd, 1, &kDebugFragDecl, nullptr); glCompileShader(fd);
    char vlog[2048] = {}, flog[2048] = {};
    GLint vs = 0, fs = 0; glGetShaderiv(v, GL_COMPILE_STATUS, &vs); glGetShaderiv(fd, GL_COMPILE_STATUS, &fs);
    glGetShaderInfoLog(v, 2048, nullptr, vlog);
    glGetShaderInfoLog(fd, 2048, nullptr, flog);
    if (!vs) std::printf("T88 VERT_LOG: %s\n", vlog);
    if (!fs) std::printf("T88 FRAG_LOG: %s\n", flog);
    CHECK(vs && fs);
    glAttachShader(prog, v); glAttachShader(prog, fd); glLinkProgram(prog);
    GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    char plog[2048] = {}; if (!ok) glGetProgramInfoLog(prog, 2048, nullptr, plog);
    CHECK(ok);
    std::printf("T88 debug shader compiled+linked (hash recorded below)\n"); }

  // stage capture target: RGBA32UI (integer-exact)
  GLuint stageTex, stageFb, colorTex, colorFb;
  glGenTextures(1, &stageTex); glBindTexture(GL_TEXTURE_2D, stageTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, 160, 288, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
  glGenTextures(1, &colorTex); glBindTexture(GL_TEXTURE_2D, colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 160, 288, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glGenFramebuffers(1, &stageFb); glBindFramebuffer(GL_FRAMEBUFFER, stageFb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, stageTex, 0);
  GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, bufs);
  GLint complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  std::printf("T88 MRT complete=%d (0x8CD5=complete)\n", (int)complete);
  CHECK(complete == GL_FRAMEBUFFER_COMPLETE);

  // Draw via debug program with the same uniform set the production pass uses.
  glViewport(0, 0, 160, 288); glUseProgram(prog);
  // tdata texture: RGBA32F 128x3, upload the fixture data (same as production pass)
  GLuint dataTex; glGenTextures(1, &dataTex);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dataTex);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 128, 3);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 3, GL_RGBA, GL_FLOAT, data.data());
  const std::array<uint32_t,2>* gen = upload.BindActive(1);
  CHECK(gen != nullptr);
  glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[0]);
  glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[1]);
  auto U = [&](const char* nm) { return glGetUniformLocation(prog, nm); };
  glUniform1i(U("u_tdata"), 0); glUniform1i(U("u_grain"), 1); glUniform1i(U("u_strata"), 2);
  glUniform2f(U("u_resolution"), f.surfaceWidthPx, f.surfaceHeightPx);
  glUniform4f(U("u_box"), f.boxXPx, f.boxYPx, f.boxWidthPx, f.boxHeightPx);
  glUniform4f(U("u_canvasBox"), p.canvasBox[0], p.canvasBox[1], p.canvasBox[2], p.canvasBox[3]);
  glUniform1f(U("u_capacity"), f.capacity);
  glUniform1i(U("u_visualInverted"), f.visualInverted);
  glUniform1i(U("u_lightEnabled"), m.lightEnabled);
  glUniform1i(U("u_grainEnabled"), p.grainEnabled);
  glUniform1i(U("u_strataEnabled"), p.strataEnabled);
  glUniform1i(U("u_sparkEnabled"), p.sparkEnabled);
  float palette[6] = {(float)p.paletteBaseSrgb[0],(float)p.paletteBaseSrgb[1],(float)p.paletteBaseSrgb[2],
    (float)p.paletteBaseSrgb[3],(float)p.paletteBaseSrgb[4],(float)p.paletteBaseSrgb[5]};
  float grainBase[9]; for (int i = 0; i < 9; ++i) grainBase[i] = (float)p.grainBaseSrgb[i];
  glUniform3fv(U("u_palette[0]"), 2, palette);
  glUniform3fv(U("u_grainBase[0]"), 3, grainBase);
  glUniform3f(U("u_offsets"), p.grainOffsetCanvasPx[0], p.grainOffsetCanvasPx[1], p.grainOffsetCanvasPx[2]);
  glUniform3f(U("u_bandMin"), p.bandMinYCanvasPx[0], p.bandMinYCanvasPx[1], p.bandMinYCanvasPx[2]);
  glUniform3f(U("u_bandMax"), p.bandMaxYCanvasPx[0], p.bandMaxYCanvasPx[1], p.bandMaxYCanvasPx[2]);
  glUniform3f(U("u_strengths"), p.aB, p.aC, p.aC3);
  glUniform1f(U("u_warpRange"), p.warpRangeCycles);
  glUniform3f(U("u_lightDir"), l.lightX, l.lightY, l.lightZ);
  glUniform4f(U("u_light"), l.la, l.ld, l.lmin, l.lmax);
  glUniform2f(U("u_lightDepth"), l.lref, l.ellLightMaterial);
  glClearColor(0, 0, 0, 0); glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  GLenum drawErr = glGetError(); CHECK(drawErr == GL_NO_ERROR);
  // Read (88,78) and neighborhood from both attachments. GL y-flip: production
  // (88,78) is in GL-readback coordinates as used by T55/T77 (glReadPixels coords).
  struct Stage { unsigned baseB, grainB, strataB, k; };
  auto readStage = [&](int x, int y) {
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    GLuint v[4]; glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, v);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    return Stage{v[0], v[1], v[2], v[3]};
  };
  unsigned char col[4];
  glReadPixels(88, 78, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, col);
  Stage s88 = readStage(88, 78);
  std::printf("T88 (88,78) color=[%u,%u,%u,%u] stage baseB=%u grainB=%u strataB=%u k=%u\n",
    col[0], col[1], col[2], col[3], s88.baseB, s88.grainB, s88.strataB, s88.k);
  // neighborhood 88+-2 x 78+-2
  std::string rows;
  for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) {
    int x = 88 + dx, y = 78 + dy;
    unsigned char c[4]; glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, c);
    Stage st = readStage(x, y);
    char row[160];
    std::snprintf(row, sizeof row, "{\"x\":%d,\"y\":%d,\"color\":[%u,%u,%u],\"baseB\":%u,\"grainB\":%u,\"strataB\":%u,\"k\":%u},",
      x, y, c[0], c[1], c[2], st.baseB, st.grainB, st.strataB, st.k);
    rows += row;
  }
  rows.pop_back();
  std::string json = std::string("{\"executionId\":\"t88-stage-probe-v1\",\"fixture\":\"flat_0_1_0_1_0_0_0\",")
    + "\"debugShaderHash\":\"" + MaterialSha256(reinterpret_cast<const uint8_t*>(kDebugFragDecl), std::strlen(kDebugFragDecl)) + "\","
    + "\"point88\": {\"color\":[" + std::to_string(col[0]) + "," + std::to_string(col[1]) + "," + std::to_string(col[2]) + "]"
    + ",\"baseB\":" + std::to_string(s88.baseB) + ",\"grainB\":" + std::to_string(s88.grainB)
    + ",\"strataB\":" + std::to_string(s88.strataB) + ",\"k\":" + std::to_string(s88.k) + "},"
    + "\"neighborhood\":[" + rows + "]}";
  { std::ofstream o(outDir + "/t88-stage-probe.json"); o.write(json.data(), json.size()); }
  std::printf("T88 stage probe json written\n");
  // cleanup
  CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
  std::puts("PASS T88 stage probe complete");
  return 0;
}
