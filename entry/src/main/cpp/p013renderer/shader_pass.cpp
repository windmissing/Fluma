#include "shader_pass.h"
namespace p013 {
GLuint ShaderPass::Compile(GLenum type, const char* source) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &source, nullptr);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[4096]{};
    GLsizei n = 0;
    glGetShaderInfoLog(s, sizeof(log), &n, log);
    compileLogHash = 14695981039346656037ULL;
    for (int i = 0; i < n; i++) {
      compileLogHash ^= static_cast<unsigned char>(log[i]);
      compileLogHash *= 1099511628211ULL;
    }
    glDeleteShader(s);
    return 0;
  }
  return s;
}
Status ShaderPass::Create(const ShaderSources& src, uint64_t generation) {
  const double t = NowMs();
  Reset(true);
  epoch = generation;
  GLuint v = Compile(GL_VERTEX_SHADER, src.vertex),
         f = Compile(GL_FRAGMENT_SHADER, src.fragment);
  if (!v || !f) {
    if (v) glDeleteShader(v);
    if (f) glDeleteShader(f);
    return Status::SHADER_COMPILE_FAILED;
  }
  program = glCreateProgram();
  glAttachShader(program, v);
  glAttachShader(program, f);
  glLinkProgram(program);
  glDeleteShader(v);
  glDeleteShader(f);
  GLint ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[4096]{};
    GLsizei n = 0;
    glGetProgramInfoLog(program, sizeof(log), &n, log);
    compileLogHash = 14695981039346656037ULL;
    for (int i = 0; i < n; i++) {
      compileLogHash ^= static_cast<unsigned char>(log[i]);
      compileLogHash *= 1099511628211ULL;
    }
    Reset(true);
    return Status::SHADER_COMPILE_FAILED;
  }
  const char* names[] = {
      "u_tdata",          "u_resolution", "u_box",  "u_capacity",
      "u_visualInverted", "u_lightDir",   "u_la",   "u_ld",
      "u_lmin",           "u_lmax",       "u_lref", "u_ellLightPx"};
  for (size_t i = 0; i < locations.size(); i++) {
    locations[i] = glGetUniformLocation(program, names[i]);
    if (locations[i] < 0) {
      Reset(true);
      return Status::SHADER_COMPILE_FAILED;
    }
  }
  glGenVertexArrays(1, &vao);
  error = glGetError();
  buildMs = NowMs() - t;
  if (error != GL_NO_ERROR || !vao) {
    Reset(true);
    return Status::GL_RESOURCE_FAILED;
  }
  return Status::OK;
}
Status ShaderPass::Draw(const FrameUniforms& u, GLuint texture) {
  const auto& f = u.frame;
  const auto& l = u.light;
  const double t = NowMs();
  glViewport(0, 0, static_cast<GLsizei>(f.surfaceWidthPx),
             static_cast<GLsizei>(f.surfaceHeightPx));
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_CULL_FACE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  if (f.diagnosticEnabled) {
    glUseProgram(program);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(locations[0], 0);
    glUniform2f(locations[1], f.surfaceWidthPx, f.surfaceHeightPx);
    glUniform4f(locations[2], f.boxXPx, f.boxYPx, f.boxWidthPx, f.boxHeightPx);
    glUniform1f(locations[3], f.capacity);
    glUniform1i(locations[4], f.visualInverted);
    glUniform3f(locations[5], l.lightX, l.lightY, l.lightZ);
    glUniform1f(locations[6], l.la);
    glUniform1f(locations[7], l.ld);
    glUniform1f(locations[8], l.lmin);
    glUniform1f(locations[9], l.lmax);
    glUniform1f(locations[10], l.lref);
    glUniform1f(locations[11], l.ellLightMaterial * f.boxHeightPx / f.capacity);
  }
  uniformMs = NowMs() - t;
  const double dt = NowMs();
  if (f.diagnosticEnabled) glDrawArrays(GL_TRIANGLES, 0, 3);
  error = glGetError();
  drawMs = NowMs() - dt;
  return error == GL_NO_ERROR ? Status::OK : Status::GL_DRAW_FAILED;
}
void ShaderPass::Reset(bool current) {
  if (!current) {
    if (program) programAbandonCount++;
    if (vao) vaoAbandonCount++;
  }
  if (current) {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (program) glDeleteProgram(program);
  }
  vao = program = 0;
  epoch = 0;
}
}  // namespace p013
