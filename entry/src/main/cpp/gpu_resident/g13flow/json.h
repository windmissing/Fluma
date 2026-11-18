#pragma once
#include "wire.h"
#include <map>
namespace grs::g13 {
// Deliberately flat command envelopes. Nested fixture bytes live in audited
// files; duplicate keys, trailing input and imprecise numeric IDs are rejected.
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
inline std::string ScopeJson(const Identity &id) {
  std::ostringstream o;
  o << '{';
  bool comma = false;
  for (auto pair : std::array<std::pair<const char *, uint64_t>, 10>{
           {{"run", id.run},
            {"owner", id.owner},
            {"context", id.context},
            {"surface", id.surface},
            {"geometry", id.geometry},
            {"config", id.config},
            {"session", id.session},
            {"permission", id.permission},
            {"inputEpoch", id.inputEpoch},
            {"control", id.control}}}) {
    if (comma)
      o << ',';
    comma = true;
    o << Quote(pair.first) << ":\"" << pair.second << '"';
  }
  o << '}';
  return o.str();
}
inline bool ReadScope(const Json &json, Identity &id) {
  return json.Valid() && json.Number("run", id.run) &&
         json.Number("owner", id.owner) && json.Number("context", id.context) &&
         json.Number("surface", id.surface) &&
         json.Number("geometry", id.geometry) &&
         json.Number("config", id.config) &&
         json.Number("session", id.session) &&
         json.Number("permission", id.permission) &&
         json.Number("inputEpoch", id.inputEpoch) &&
         json.Number("control", id.control);
}
} // namespace grs::g13
