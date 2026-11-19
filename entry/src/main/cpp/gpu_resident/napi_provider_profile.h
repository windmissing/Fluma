#pragma once
#include "program_cache/sha256.h"
#include <array>
#include <cstring>
#include <dlfcn.h>
#include <mutex>
#include <napi/native_api.h>
#include <string>
#include <time.h>
#ifndef GRS_NAPI_SDK_SHA
#define GRS_NAPI_SDK_SHA "UNRECORDED"
#endif
namespace grs::provider629 {
enum class Unit : uint8_t { UNKNOWN = 0, ELEMENTS = 1, BYTES = 2 };
inline const char *Name(Unit u) {
  return u == Unit::ELEMENTS ? "ELEMENTS"
         : u == Unit::BYTES  ? "BYTES"
                             : "UNKNOWN";
}
struct Sample {
  uint64_t expected = 0, width = 0, raw = 0, backing = 0;
  int32_t createBuffer = -1, createArray = -1, info = -1, backingInfo = -1;
  bool consistent = false;
};
struct Profile {
  napi_env env = nullptr;
  uint64_t boot = 0, symbolAddress = 0;
  Unit unit = Unit::UNKNOWN;
  std::array<char, 192> module{};
  std::array<char, 64> symbol{};
  std::array<char, 65> sdk{}, evidence{};
  std::array<char, 48> id{};
  std::array<Sample, 8> samples{};
  int32_t cleanup = -1, openScope = -1, closeScope = -1;
  bool used = false, ready = false;
};
struct Registry {
  std::mutex mutex;
  std::array<Profile, 8> slots{};
  uint64_t serial = 0;
};
inline Registry &Store() {
  static Registry s;
  return s;
}
inline void Cleanup(void *env) {
  auto &s = Store();
  std::lock_guard<std::mutex> lock(s.mutex);
  for (auto &p : s.slots)
    if (p.env == env)
      p = {};
}
inline uint64_t Boot() {
  static const uint64_t stamp = []() {
    timespec t{};
    return clock_gettime(CLOCK_MONOTONIC, &t) == 0
               ? uint64_t(t.tv_sec) * 1000000000ull + t.tv_nsec
               : 0;
  }();
  return stamp;
}
inline std::string Quote(const std::string &value) {
  std::string r = "\"";
  for (unsigned char c : value) {
    if (c == '"' || c == '\\') {
      r += '\\';
      r += char(c);
    } else if (c >= 32)
      r += char(c);
    else {const char* h="0123456789abcdef";r+="\\u00";r+=h[c>>4];r+=h[c&15];}
  }
  return r + '"';
}
inline std::string Evidence(const Profile &p) {
  std::string out =
      "{\"schema\":\"T629_NATIVE_PROVIDER_QUALIFICATION_V1\",\"boot\":\"" +
      std::to_string(p.boot) +
      "\",\"interop\":\"NATIVE_STANDARD_TYPEDARRAY_API\",\"module\":" +
      Quote(p.module.data()) + ",\"symbol\":" + Quote(p.symbol.data()) +
      ",\"symbolAddress\":\"" + std::to_string(p.symbolAddress) +
      "\",\"sdkHeaderSHA\":" + Quote(p.sdk.data()) +
      ",\"lengthUnit\":" + Quote(Name(p.unit)) + ",\"scopeStatus\":[" +
      std::to_string(p.cleanup) + "," + std::to_string(p.openScope) + "," +
      std::to_string(p.closeScope) + "],\"samples\":[";
  for (size_t i = 0; i < 8; i++) {
    if (i)
      out += ',';
    const auto &s = p.samples[i];
    out += "[" + std::to_string(s.expected) + "," + std::to_string(s.width) +
           "," + std::to_string(s.raw) + "," + std::to_string(s.backing) + "," +
           std::to_string(s.createBuffer) + "," +
           std::to_string(s.createArray) + "," + std::to_string(s.info) + "," +
           std::to_string(s.backingInfo) + "," + (s.consistent ? "1" : "0") +
           "]";
  }
  return out + "]}";
}
inline void Finish(Profile &p) {
  auto raw = Evidence(p);
  p011gpu::Sha256 s;
  s.update(raw.data(), raw.size());
  auto hash = s.finish();
  const char *h = "0123456789abcdef";
  for (size_t i = 0; i < 32; i++) {
    p.evidence[2 * i] = h[hash[i] >> 4];
    p.evidence[2 * i + 1] = h[hash[i] & 15];
  }
  p.ready = true;
}
inline Profile For(napi_env env) {
  auto &store = Store();
  Profile p;
  size_t slot = 8;
  {
    std::lock_guard<std::mutex> lock(store.mutex);
    for (auto &v : store.slots)
      if (v.used && v.env == env)
        return v;
    for (size_t i = 0; i < 8; i++)
      if (!store.slots[i].used) {
        slot = i;
        break;
      }
    if (slot == 8 || store.serial == UINT64_MAX)
      return p;
    p.env = env;
    p.boot = Boot();
    p.used = true;
    auto id = std::string("T629:") + std::to_string(p.boot) + ":" +
              std::to_string(++store.serial);
    if (id.size() >= p.id.size())
      return Profile{};
    std::memcpy(p.id.data(), id.data(), id.size());
    store.slots[slot] = p;
  }
  // Qualification is independent of the caller's N/value. Eight native-created
  // known typed arrays use the exact imported provider and standard interop.
  std::strncpy(p.sdk.data(), GRS_NAPI_SDK_SHA, p.sdk.size() - 1);
  Dl_info binding{};
  auto address = reinterpret_cast<void *>(&napi_get_typedarray_info);
  if (dladdr(address, &binding) && binding.dli_fname && binding.dli_sname &&
      std::strlen(binding.dli_fname) < p.module.size() &&
      std::strlen(binding.dli_sname) < p.symbol.size()) {
    std::strcpy(p.module.data(), binding.dli_fname);
    std::strcpy(p.symbol.data(), binding.dli_sname);
    p.symbolAddress = reinterpret_cast<uintptr_t>(address);
  }
  if (p.boot && std::strlen(p.sdk.data()) == 64 && p.module[0] && p.symbol[0]) {
    p.cleanup = napi_add_env_cleanup_hook(env, Cleanup, env);
    napi_handle_scope scope = nullptr;
    if (p.cleanup == napi_ok) {
      p.openScope = napi_open_handle_scope(env, &scope);
      if (p.openScope == napi_ok) {
        bool elements = true, bytes = true, complete = true;
        const napi_typedarray_type kinds[] = {
            napi_uint8_array, napi_uint16_array, napi_uint32_array,
            napi_float32_array};
        const size_t widths[] = {1, 2, 4, 4};
        for (size_t i = 0; i < 8; i++) {
          auto &q = p.samples[i];
          q.expected = i % 2 ? 3 : 1;
          q.width = widths[i / 2];
          void *original = nullptr;
          napi_value buffer = nullptr, value = nullptr;
          q.createBuffer = napi_create_arraybuffer(env, q.expected * q.width,
                                                   &original, &buffer);
          if (q.createBuffer != napi_ok) {
            complete = false;
            break;
          }
          q.createArray = napi_create_typedarray(env, kinds[i / 2], q.expected,
                                                 buffer, 0, &value);
          if (q.createArray != napi_ok) {
            complete = false;
            break;
          }
          napi_typedarray_type kind{};
          size_t raw = 0, offset = 0;
          void *data = nullptr;
          napi_value backing = nullptr;
          q.info = napi_get_typedarray_info(env, value, &kind, &raw, &data,
                                            &backing, &offset);
          if (q.info != napi_ok) {
            complete = false;
            break;
          }
          q.raw = raw;
          void *base = nullptr;
          size_t backingBytes = 0;
          q.backingInfo =
              napi_get_arraybuffer_info(env, backing, &base, &backingBytes);
          q.backing = backingBytes;
          q.consistent = q.backingInfo == napi_ok && kind == kinds[i / 2] &&
                         original && data == original && base == original &&
                         offset == 0 && backingBytes == q.expected * q.width;
          if (!q.consistent) {
            complete = false;
            break;
          }
          elements = elements && raw == q.expected;
          bytes = bytes && raw == q.expected * q.width;
        }
        p.closeScope = napi_close_handle_scope(env, scope);
        if (complete && p.closeScope == napi_ok && elements != bytes)
          p.unit = elements ? Unit::ELEMENTS : Unit::BYTES;
      }
    }
  }
  Finish(p);
  {
    std::lock_guard<std::mutex> lock(store.mutex);
    if (store.slots[slot].used && store.slots[slot].env == env)
      store.slots[slot] = p;
  }
  return p;
}
inline std::string Lookup(const char *id) {
  auto &s = Store();
  Profile copy;
  {
    std::lock_guard<std::mutex> lock(s.mutex);
    for (const auto &p : s.slots)
      if (p.used && std::strcmp(p.id.data(), id) == 0) {
        copy = p;
        break;
      }
  }
  return copy.used ? Evidence(copy) : "null";
}
} // namespace grs::provider629
