#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace t693w {
struct Action {
 uint64_t first=0,last=0,declared=0,received=0,checkpoint=0,consumed=0;
};
struct Root {
 uint64_t nonce=0,start=0,deadline=0,revision=0,authorize=0,stamp=0;
 uint32_t cut=0,kind=0;
 bool valid=false;
};
inline thread_local Action action;
inline thread_local Root root;
inline thread_local bool replay=false;

struct RootScope {
 Root parent=root;
 RootScope(){root={};}
 ~RootScope(){root=parent;}
};
struct ReplayScope {
 bool parent=replay;
 Root previous=root;
 ReplayScope(){replay=true;}
 ~ReplayScope(){replay=parent;root=previous;}
};

// The wall timestamp remains the original 32-byte row field.
inline std::string_view anchor(std::array<char,512>& buf){
 const auto n=std::snprintf(buf.data(),buf.size(),
  "T703_V1 W=1 S=%s,%llu,%llu,%llu,%llu,%llu,%llu T=%s,%llu,%u,%u,%llu,%llu,%llu,%llu,%llu R=%u",
  action.first?(action.consumed?"OWNER":"DELIVERY"):"NA",
  static_cast<unsigned long long>(action.first),static_cast<unsigned long long>(action.last),
  static_cast<unsigned long long>(action.declared),static_cast<unsigned long long>(action.received),
  static_cast<unsigned long long>(action.checkpoint),static_cast<unsigned long long>(action.consumed),
  root.nonce?(root.valid?(root.stamp?"PERMIT":"AUTHORIZE"):"UNKNOWN"):"NA",
  static_cast<unsigned long long>(root.nonce),root.kind,root.cut,
  static_cast<unsigned long long>(root.start),static_cast<unsigned long long>(root.deadline),
  static_cast<unsigned long long>(root.revision),static_cast<unsigned long long>(root.authorize),
  static_cast<unsigned long long>(root.stamp),unsigned(replay));
 return n>0&&size_t(n)<buf.size()?std::string_view(buf.data(),size_t(n)):std::string_view{};
}
}
