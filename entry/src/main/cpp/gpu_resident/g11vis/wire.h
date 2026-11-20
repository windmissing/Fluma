#pragma once
#include "parent_adapter.h"
#include <map>
#include <sstream>
namespace grs::g11vis {
class Json {
  std::map<std::string, std::string> fields_;
  bool valid_ = false;

public:
  explicit Json(const std::string &s) {
    if (s.size() > 4096)
      return;
    size_t p = 0;
    auto ws = [&]() {
      while (p < s.size() &&
             (s[p] == ' ' || s[p] == '\n' || s[p] == '\r' || s[p] == '\t'))
        p++;
    };
    auto string = [&](std::string &out) {
      if (p >= s.size() || s[p++] != '"')
        return false;
      while (p < s.size()) {
        char c = s[p++];
        if (c == '"')
          return true;
        if (c == '\\') {
          if (p >= s.size())
            return false;
          c = s[p++];
          if (c != '\\' && c != '"' && c != '/')
            return false;
        }
        if (static_cast<unsigned char>(c) < 32)
          return false;
        out += c;
      }
      return false;
    };
    ws();
    if (p >= s.size() || s[p++] != '{')
      return;
    ws();
    while (p < s.size() && s[p] != '}') {
      std::string key, value;
      if (!string(key))
        return;
      ws();
      if (p >= s.size() || s[p++] != ':')
        return;
      ws();
      if (!string(value))
        return;
      if (!fields_.emplace(key, value).second)
        return;
      ws();
      if (p < s.size() && s[p] == ',') {
        p++;
        ws();
        if (p == s.size() || s[p] == '}')
          return;
      } else
        break;
    }
    if (p >= s.size() || s[p++] != '}')
      return;
    ws();
    valid_ = p == s.size();
  }
  bool Valid() const { return valid_; }
  std::string Text(const char *key) const {
    auto i = fields_.find(key);
    return i == fields_.end() ? "" : i->second;
  }
  bool Number(const char *key, uint64_t &out, bool required = true) const {
    std::string s = Text(key);
    if (s.empty()) {
      out = 0;
      return !required;
    }
    if (s.size() > 20 || (s.size() > 1 && s[0] == '0'))
      return false;
    out = 0;
    for (char c : s) {
      if (c < '0' || c > '9' || out > (UINT64_MAX - uint64_t(c - '0')) / 10)
        return false;
      out = out * 10 + uint64_t(c - '0');
    }
    return true;
  }
};
inline bool DecodeScope(const std::string &text, Scope &s) {
  Json j(text);
  return j.Valid() && j.Number("run", s.run) && j.Number("owner", s.owner) &&
         j.Number("context", s.context) &&
         j.Number("inputEpoch", s.inputEpoch) &&
         j.Number("pageSession", s.pageSession) &&
         j.Number("permission", s.permission) && j.Number("window", s.window) &&
         j.Number("config", s.config) && j.Number("surface", s.surface);
}
inline std::string EncodeScope(const Scope &s) {
  std::ostringstream out;
  out << "{\"run\":\"" << s.run << "\",\"owner\":\"" << s.owner
      << "\",\"context\":\"" << s.context << "\",\"inputEpoch\":\""
      << s.inputEpoch << "\",\"pageSession\":\"" << s.pageSession
      << "\",\"permission\":\"" << s.permission << "\",\"window\":\""
      << s.window << "\",\"config\":\"" << s.config << "\",\"surface\":\""
      << s.surface << "\"}";
  return out.str();
}
inline bool Decimal(const std::string &text, uint64_t &n) {
  return Json("{\"n\":\"" + text + "\"}").Number("n", n);
}
inline std::string Quote(const std::string &text) {
  std::ostringstream o;
  o << '"';
  for (unsigned char c : text) {
    if (c == '"' || c == '\\')
      o << '\\' << char(c);
    else if (c < 32) {
      const char *h = "0123456789abcdef";
      o << "\\u00" << h[c >> 4] << h[c & 15];
    } else
      o << char(c);
  }
  o << '"';
  return o.str();
}
} // namespace grs::g11vis
