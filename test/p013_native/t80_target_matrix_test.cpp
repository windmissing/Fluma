// T80 H1 target-chain constant-shader matrix (analysis-T77 section 6.1).
// Same-process, alternating four targets; constant fragment outputs fixed
// linear L values; per-target full state record. Independent executor; no
// product code change; no device; no Canvas.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>

#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)

static std::string g_dir;
static void dump(const std::string& name, const std::string& text) {
  std::ofstream f(g_dir + "/" + name, std::ios::binary); f.write(text.data(), text.size());
}
static std::string qErr(const char* tag) {
  std::string s;
  for (int e = eglGetError(); e != EGL_SUCCESS; e = eglGetError()) s += std::string(tag) + ":0x" + std::to_string(e) + ";";
  for (GLenum e = glGetError(); e != GL_NO_ERROR; e = glGetError()) s += std::string(tag) + ":gl0x" + std::to_string(e) + ";";
  return s.empty() ? "NONE" : s;
}
static std::string qi(GLenum p) { GLint v = -999; glGetIntegerv(p, &v); return std::to_string(v); }

static const char* kVert = "#version 300 es\nvoid main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);}\n";
// constant output L, injected per target via uniform
static const char* kFrag =
  "#version 300 es\nprecision highp float;uniform vec4 uL;out vec4 o;void main(){o=uL;}\n";

struct TargetResult { std::string id; int width=64, height=64; std::string surfaceQuery, attachQuery,
  writeControl, dither, drawBuf, readBuf, packState, errLog, sha; unsigned char px[6][4]{}; bool complete=false; };

static GLuint mkProg(const char* frag) {
  GLuint v=glCreateShader(GL_VERTEX_SHADER);glShaderSource(v,1,&kVert,nullptr);glCompileShader(v);
  GLuint f=glCreateShader(GL_FRAGMENT_SHADER);glShaderSource(f,1,&frag,nullptr);glCompileShader(f);
  GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
  GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
  if(!ok){char log[2048]{};glGetProgramInfoLog(p,2048,nullptr,log);
    char vlog[2048]{};glGetShaderInfoLog(v,2048,nullptr,vlog);
    char flog[2048]{};glGetShaderInfoLog(f,2048,nullptr,flog);
    std::printf("LINK_FAIL prog=[%s] vert=[%s] frag=[%s]\n",log,vlog,flog);return 0;}
  return p;
}
static void drawConstants(GLuint prog, EGLDisplay dpy, EGLSurface surf, TargetResult& r, bool isFbo) {
  glUseProgram(prog);
  glViewport(0, 0, 64, 64);
  glDisable(GL_BLEND); glDisable(GL_SCISSOR_TEST); glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  // L values: 0, 0.0031308 (sRGB knee), 0.18 (linear ~45.9 / sRGB ~117.6458), 0.5, 1.0, EOTF(palette[145/255])
  const double EOTF = std::pow(145.0/255.0, 2.4) * 1.055 - 0.055; // exact sRGB EOTF
  const double Ls[6] = {0.0, 0.0031308, 0.18, 0.5, 1.0, EOTF};
  for (int i = 0; i < 6; ++i) {
    float v = (float)Ls[i];
    glUniform4f(glGetUniformLocation(prog, "uL"), v, v, v, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    unsigned char px[4] = {0, 0, 0, 0};
    GLenum drawErr = glGetError();
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    GLenum readErr = glGetError();
    for (int c = 0; c < 4; ++c) r.px[i][c] = px[c];
    if (drawErr || readErr) r.errLog += std::string("[") + std::to_string(i) + ":d0x" + std::to_string(drawErr) + ",r0x" + std::to_string(readErr) + "]";
  }
  // state record
  r.writeControl = "EXT_sRGB_write_control:"; // token-precise from extension list; resolved by caller
  GLint drawB=0, readB=0, dither=0, packAlign=0, packRow=0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawB); glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readB);
  glGetIntegerv(GL_DITHER, &dither); glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign); glGetIntegerv(GL_PACK_ROW_LENGTH, &packRow);
  char buf[256]; std::snprintf(buf, sizeof buf, "draw=%d read=%d dither=%d packAlign=%d packRow=%d", drawB, readB, dither, packAlign, packRow);
  r.drawBuf = buf; r.errLog = qErr("state");
  if (surf != EGL_NO_SURFACE) {
    EGLint cs = 0;
    EGLBoolean ok = eglQuerySurface(dpy, surf, EGL_GL_COLORSPACE_KHR, &cs);
    char b2[64]; std::snprintf(b2, sizeof b2, "query=%s colorspace=%d", ok ? "OK" : "FAIL", (int)cs);
    r.surfaceQuery = b2;
  } else r.surfaceQuery = "no-surface(FBO-only)";
  if (isFbo) { GLint enc = 0; glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &enc);
    r.attachQuery = std::string("attachment0=") + (enc == GL_SRGB ? "SRGB" : enc == GL_LINEAR ? "LINEAR" : "other"); }
  else { GLint enc = 0; glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &enc); // per contract: GL_BACK default query
    r.attachQuery = std::string("GL_BACK_impl_read_format=0x") + std::to_string(enc); }
  // raw dump
  unsigned char full[64*64*4];
  glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, full);
  std::ofstream f(g_dir + "/" + r.id + "-raw.rgba", std::ios::binary); f.write((char*)full, sizeof full);
}
int main(int argc, char** argv) {
  g_dir = argc > 1 ? argv[1] : ".codex/local-test/t80-matrix";
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  std::string exts = eglQueryString(dpy, EGL_EXTENSIONS);
  bool hasColorspace = exts.find("EGL_KHR_gl_colorspace") != std::string::npos;
  std::string header = std::string("EGL ") + eglQueryString(dpy, EGL_VERSION) + "\n";
  dump("environment.txt", header + exts + "\n");
  GLuint prog = 0; // deferred until a context is current (created inside T1 block)
  std::vector<TargetResult> results;
  auto runCurrent = [&](const char* id, bool isFbo, GLuint fbo) {
    TargetResult r; r.id = id;
    drawConstants(prog, dpy, eglGetCurrentSurface(EGL_DRAW), r, isFbo);
    r.writeControl += exts.find("GL_EXT_sRGB_write_control") != std::string::npos ? "SUPPORTED" : "NOT_SUPPORTED";
    results.push_back(r);
  };
  // T1: default pbuffer (LINEAR request omitted => driver default)
  { EGLint sa[] = {EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};
    EGLConfig cfg; EGLint n=0; EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    eglChooseConfig(dpy, ca, &cfg, 1, &n); CHECK(n>0);
    EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
    EGLSurface s = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(s != EGL_NO_SURFACE);
    CHECK(eglMakeCurrent(dpy, s, s, ctx));
    if (!prog) { prog = mkProg(kFrag); CHECK(prog); }
    runCurrent("T1-default-pbuffer", false, 0); results.back().width = results.back().height = 64;
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    // T2: same CONTEXT, SRGB pbuffer (alternating targets in one process, same context)
    if (hasColorspace) { EGLint sa2[] = {EGL_WIDTH,64,EGL_HEIGHT,64,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
      EGLSurface s2 = eglCreatePbufferSurface(dpy, cfg, sa2); CHECK(s2 != EGL_NO_SURFACE);
      CHECK(eglMakeCurrent(dpy, s2, s2, ctx));
      runCurrent("T2-srgb-pbuffer", false, 0);
      eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); eglDestroySurface(dpy, s2); }
    else results.push_back({"T2-srgb-pbuffer-SKIPPED: EGL_KHR_gl_colorspace NOT in display extensions"});
    eglDestroyContext(dpy, ctx); eglDestroySurface(dpy, s);
  }
  // T3/T4: explicit FBOs in a fresh context (texture-backed)
  { EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    EGLConfig cfg; EGLint n=0; eglChooseConfig(dpy, ca, &cfg, 1, &n); CHECK(n>0);
    EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
    EGLint sa[] = {EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};
    EGLSurface s = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(s != EGL_NO_SURFACE);
    CHECK(eglMakeCurrent(dpy, s, s, ctx));
    GLuint prog2 = mkProg(kFrag); CHECK(prog2); // fresh context: program must be recreated
    auto fboTarget = [&](const char* id, GLenum internal) {
      GLuint tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
      glTexImage2D(GL_TEXTURE_2D, 0, internal, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
      GLuint fb; glGenFramebuffers(1, &fb); glBindFramebuffer(GL_FRAMEBUFFER, fb);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
      GLint complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      // run in this binding
      TargetResult r; r.id = id;
      drawConstants(prog2, dpy, EGL_NO_SURFACE, r, true);
      r.complete = complete == GL_FRAMEBUFFER_COMPLETE;
      char b[64]; std::snprintf(b, sizeof b, "fboStatus=0x%x", (unsigned)complete); r.drawBuf += " "; r.drawBuf += b;
      results.push_back(r);
      glBindFramebuffer(GL_FRAMEBUFFER, 0); glDeleteFramebuffers(1, &fb); glDeleteTextures(1, &tex);
    };
    fboTarget("T3-rgba8-fbo", GL_RGBA8);
    fboTarget("T4-srgb8alpha8-fbo", GL_SRGB8_ALPHA8);
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx); eglDestroySurface(dpy, s);
  }
  eglTerminate(dpy);
  // emit JSON
  std::string j = "[\n";
  for (auto& r : results) {
    j += "{\"id\":\"" + r.id + "\",\"surfaceQuery\":\"" + r.surfaceQuery + "\",\"attachQuery\":\"" + r.attachQuery +
         "\",\"writeControl\":\"" + r.writeControl + "\",\"state\":\"" + r.drawBuf + "\",\"err\":\"" + r.errLog + "\"";
    if (!r.id.empty() && r.id[0] == 'T' && isdigit(r.id[1])) {
      j += ",\"constantOutputs\":[";
      for (int i = 0; i < 6; ++i) {
        char b[96]; std::snprintf(b, sizeof b, "[%u,%u,%u,%u]", r.px[i][0], r.px[i][1], r.px[i][2], r.px[i][3]);
        j += b; if (i < 5) j += ",";
      }
      j += "]";
    }
    j += "},\n";
  }
  j += "]\n";
  dump("t80-target-matrix.json", j);
  std::printf("%s", j.c_str());
  std::puts("PASS T80 H1 target matrix recorded");
  return 0;
}
