#include "scene.h"
#include "../resource_gl.h"
#include "scene_sources.h"
namespace grs::g13 {
bool Scene::Prepare(uint64_t context, const Config &config) {
  if (program_ && context_ != context)
    Release(actualContext_ == eglGetCurrentContext());
  context_ = context;
  actualContext_ = eglGetCurrentContext();
  config_ = config;
  if (!program_) {
    programcache::GraphicsProgramDesc desc{
        "G13_SCENE",
        {{{GL_VERTEX_SHADER, kG13Vertex}, {GL_FRAGMENT_SHADER, kG13Fragment}}},
        "G13_RENDER_CELL16_TRACER8_UINT_ATTRIBUTES_V1"};
    if (!programcache::BuildGraphics(desc, program_, error_, context) ||
        !programcache::ReflectGraphics(program_, uniforms_, desc, error_,
                                       context))
      return false;
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    for (GLuint i = 0; i < 6; i++) {
      glVertexAttribDivisor(i, 1);
      glDisableVertexAttribArray(i);
    }
    if (programcache::GraphicsPrepared(program_, context) != GL_NO_ERROR)
      return false;
  }
  policy_ = {};
  policy_[0] = kMagic_view;
  policy_[1] = 1;
  policy_[2] = 64;
  policy_[3] = 0;
  policy_[4] = Bits(0);
  policy_[5] = Bits(2);
  policy_[6] = Bits(-64);
  policy_[7] = Bits(64);
  policy_[8] = Bits(-256);
  policy_[9] = Bits(256);
  policy_[10] = Bits(F32(config[34]) - 128);
  policy_[11] = Bits(F32(config[34]) + 128);
  policy_[12] = Bits(.002f);
  policy_[13] = Bits(.025f);
  policy_[14] = Bits(1.f);
  std::copy_n(config.data() + 76, 8, policy_.data() + 16);
  std::copy_n(config.data() + 44, 8, policy_.data() + 32);
  policy_[63] = kSeal;
  SealHash(policy_, 24);
  return true;
}
bool Scene::View(uint32_t mode, const std::array<uint32_t, 64> &policy) {
  if (mode > 4 || policy[0] != kMagic_view || policy[1] != 1 ||
      policy[2] != 64 || policy[3] != mode || policy[63] != kSeal ||
      !SelfHash(policy, 24))
    return false;
  for (size_t i = 40; i < 63; i++)
    if (policy[i])
      return false;
  for (size_t i = 4; i < 12; i += 2)
    if (!std::isfinite(F32(policy[i])) || !std::isfinite(F32(policy[i + 1])) ||
        F32(policy[i]) >= F32(policy[i + 1]))
      return false;
  if (!std::isfinite(F32(policy[14])) || F32(policy[14]) <= 0 || policy[15] ||
      !std::isfinite(F32(policy[12])) || !std::isfinite(F32(policy[13])) ||
      F32(policy[12]) <= 0 || F32(policy[13]) <= 0)
    return false;
  for (size_t i = 0; i < 8; i++)
    if (policy[16 + i] != config_[76 + i] || policy[32 + i] != config_[44 + i])
      return false;
  if (policy_ != policy) {
    policy_ = policy;
    ++viewGeneration_;
  }
  return true;
}
bool Scene::Draw(Controller &owner, int width, int height,
                 const std::function<bool()> &swap) {
  if (width <= 0 || height <= 0)
    return false;
  bool successful = false;
  auto onExit = [&]() {
    if (!successful && !mapping_.empty()) {
      sceneState_ = "STALE_PREVIOUS_FRAME";
      ++staleFrames_;
      owner.RecordSceneStale();
    }
  };
  struct Finish {
    decltype(onExit) &run;
    ~Finish() { run(); }
  } finish{onExit};
  auto lease = owner.AcquireScene();
  if (!lease.valid)
    return false;
  auto release = [&]() {
    owner.ReleaseScene(lease.generation,
                       glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
  };
  if (!owner.CanPresent(lease, Now()) ||
      targets_.Ensure(width, height, context_) != p013::Status::OK) {
    release();
    return false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, targets_.fbos[targets_.candidate]);
  glViewport(0, 0, width, height);
  glLineWidth(F32(policy_[14]));
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glUseProgram(program_);
  glBindVertexArray(vao_);
  glUniform2f(uniforms_.Find("uGrid"), float(lease.profile->nx),
              float(lease.profile->ny));
  const auto &tail = owner.AcceptedCompletion();
  float theta = F32(tail[136]), cs = std::cos(theta), sn = std::sin(theta);
  float lx = F32(config_[8]), ly = F32(config_[9]);
  float rotatedX = std::abs(cs) * lx + std::abs(sn) * ly,
        rotatedY = std::abs(sn) * lx + std::abs(cs) * ly;
  float pixels =
      .9f * std::min(float(width) / rotatedX, float(height) / rotatedY);
  float boxX = lx * pixels / float(width), boxY = ly * pixels / float(height);
  glUniform2f(uniforms_.Find("uBoxSize"), lx, ly);
  glUniform4f(uniforms_.Find("uBox"), .5f - .5f * boxX, .5f - .5f * boxY,
              .5f + .5f * boxX, .5f + .5f * boxY);
  glUniform2f(uniforms_.Find("uPose"), cs, sn);
  glUniform1ui(uniforms_.Find("uMode"), policy_[3]);
  size_t range = policy_[3] == 2   ? 8
                 : policy_[3] == 3 ? 6
                 : policy_[3] == 4 ? 10
                                   : 4;
  glUniform4f(uniforms_.Find("uRange"), F32(policy_[range]),
              F32(policy_[range + 1]), F32(policy_[13]), F32(policy_[12]));
  glUniform1i(uniforms_.Find("uFrame"), 0);
  glUniform1ui(uniforms_.Find("uDraw"), 0);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindBuffer(GL_ARRAY_BUFFER, lease.pack);
  for (GLuint i = 0; i < 4; i++) {
    glEnableVertexAttribArray(i);
    glVertexAttribIPointer(i, 4, GL_UNSIGNED_INT, 64,
                           reinterpret_cast<void *>(uintptr_t(256 + i * 16)));
  }
  glUniform1ui(uniforms_.Find("uDraw"), 1);
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GLsizei(lease.profile->nc));
  if (policy_[3] == 0) {
    glUniform1ui(uniforms_.Find("uDraw"), 2);
    glDrawArraysInstanced(GL_LINES, 0, 6, GLsizei(lease.profile->nc));
  }
  for (GLuint i = 0; i < 4; i++)
    glDisableVertexAttribArray(i);
  if (policy_[3] == 1) {
    glBindBuffer(GL_ARRAY_BUFFER, lease.field);
    for (GLuint i = 4; i < 6; i++) {
      glEnableVertexAttribArray(i);
      glVertexAttribIPointer(
          i, 4, GL_UNSIGNED_INT, 32,
          reinterpret_cast<void *>(
              uintptr_t(lease.profile->state[4].offset * 4 + (i - 4) * 16)));
    }
    glUniform1ui(uniforms_.Find("uDraw"), 3);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4,
                          GLsizei(owner.Gpu().Configuration()[28]));
    for (GLuint i = 4; i < 6; i++)
      glDisableVertexAttribArray(i);
  }
  glUniform1ui(uniforms_.Find("uDraw"), 4);
  glDrawArrays(GL_LINE_LOOP, 0, 4);
  if (glGetError() != GL_NO_ERROR || !owner.CanPresent(lease, Now())) {
    release();
    return false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, targets_.textures[targets_.candidate]);
  glUniform1ui(uniforms_.Find("uDraw"), 5);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  if (glGetError() != GL_NO_ERROR || !owner.CanPresent(lease, Now()) ||
      !swap()) {
    release();
    return false;
  }
  if (!owner.CanPresent(lease, Now())) {
    release();
    return false;
  }
  targets_.Commit();
  presented_ = lease.generation;
  ++swapFrame_;
  auto token = owner.FieldToken();
  owner.RecordSceneSwap(tail[4] >= 50, token == fieldToken_);
  fieldToken_ = token;
  std::ostringstream map;
  map << "{\"fieldToken\":" << Quote(token) << ",\"packGeneration\":\""
      << lease.generation << "\",\"geometry\":\"" << lease.identity.geometry
      << "\",\"viewPolicySHA\":\"" << Hex(policy_.data() + 24)
      << "\",\"viewport\":[" << width << ',' << height << "],\"uBoxBits\":["
      << Bits(.5f - .5f * boxX) << ',' << Bits(.5f - .5f * boxY) << ','
      << Bits(.5f + .5f * boxX) << ',' << Bits(.5f + .5f * boxY)
      << "],\"thetaBits\":" << tail[136] << ",\"successfulSwapFrame\":\""
      << swapFrame_ << "\"}";
  auto rawMapping = map.str();
  const auto marker = rawMapping.find(",\"successfulSwapFrame\"");
  auto immutable = rawMapping.substr(0, marker) + "}";
  auto sceneHash = Hash(immutable.data(), immutable.size());
  mapping_ = std::string("{\"sceneToken\":\"") + Hex(sceneHash.data()) +
             "\",\"mapping\":" + rawMapping + "}";
  sceneState_ = "SWAPPED_NOT_DISPLAY_VERIFIED";
  successful = true;
  release();
  return true;
}
void Scene::Release(bool current) {
  targets_.Reset(current);
  if (current) {
    if (vao_)
      glDeleteVertexArrays(1, &vao_);
    if (program_) {
      programcache::ForgetGraphics(program_, context_, eglGetCurrentContext());
      glDeleteProgram(program_);
    }
  }
  program_ = vao_ = 0;
}
std::string Scene::Receipt() const {
  std::ostringstream o;
  o << "{\"route\":13,\"sceneState\":" << Quote(sceneState_)
    << ",\"staleAttempts\":\"" << staleFrames_ << "\",\"fieldCommit\":\""
    << presented_ << "\",\"viewGeneration\":\"" << viewGeneration_
    << "\",\"fboBytes\":\"" << targets_.Bytes() << "\",\"rawGL\":" << error_
    << ",\"presentedMapping\":" << (mapping_.empty() ? "null" : mapping_)
    << ",\"displayPresentVerified\":false}";
  return o.str();
}
} // namespace grs::g13
