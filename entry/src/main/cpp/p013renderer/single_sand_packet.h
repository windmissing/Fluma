#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace p013 {
inline constexpr char SingleSandVersions[] =
 "P013_SINGLE_SAND_CONFIG_V1|SINGLE_SAND_DUALSTATE_SCENE_V1|SINGLE_H_SAND_V1|DRY_H_ONLY_V1|"
 "SINGLE_SAND_BOUNDED_LINEAR_V1|SINGLE_SAND_SRGB8_V1|SINGLE_GRAIN_R8_SIGNED_V1|"
 "SINGLE_DISC_INNER_RIM_V1|BINARY_NATIVE_CENTER_V1|CONTAINER_LOGICAL_PX_V1|"
 "E2_GAUSS_COL6_R18_P1_V2|LINEAR_DISPLAY_PREMULT_OVER_V1|T356_YCODE_DUALSTATE_V1|T364_RAW_GRAIN_V1";
struct SingleSandConfig {
 std::array<double,19> values{};
 std::array<float,3> albedo{},background{},light{};
 std::string digest,grainHash,generatorHash;
 uint64_t revision=0,session=0;
 const char* Validate();
};
struct SingleSandPacket {
 uint64_t session=0,surface=0,context=0,geometry=0,continuity=0,epoch=0,commit=0,frame=0,certificate=0,seal=0,revision=0,generation=0;
 std::string run,token,kind,ledger,digest,grainHash,rawHash,displayHash,pbHash,particleHash,shaderHash;
 int width=0,height=0,count=0,slots=0;
 double capacity=0,logicalW=0,logicalH=0,scale=0;
 std::array<double,4> box{};
 bool inverted=false,enabled=false;
 std::array<float,256> profile{};
 std::array<float,1024> xy{};
 std::array<uint32_t,1024> meta{};
 const char* Validate();
};
bool SingleHash(const std::string&);
std::string SingleIdentity(const SingleSandPacket&);
}
