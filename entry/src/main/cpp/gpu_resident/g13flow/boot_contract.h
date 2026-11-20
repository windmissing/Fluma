#pragma once
#include "wire.h"
namespace grs::g13 {
inline constexpr size_t kColdTailWordOffset = 18432;
inline constexpr size_t kProbeWordOffset = kColdTailWordOffset + 200;
inline constexpr std::array<uint32_t, 8> kExpectedFp32{
    0x3f800001u, 0x3f800000u, 1u, 0x00400000u, 7u, 0u, 0u, 0x46503332u};

inline bool QualifiedProbe(const uint32_t *words) {
  return std::equal(kExpectedFp32.begin(), kExpectedFp32.end(), words) &&
         std::equal(kExpectedFp32.begin(), kExpectedFp32.end(), words + 8);
}

inline std::array<uint32_t, 80> BootLayout(const Profile &profile) {
  std::array<uint32_t, 80> layout{};
  layout[0] = profile.nx;
  layout[1] = profile.ny;
  layout[2] = profile.nc;
  layout[3] = profile.nf;
  layout[4] = profile.sw;
  layout[5] = profile.ww;
  layout[6] = profile.pw;
  layout[7] = profile.mgCount;
  layout[8] = profile.groups;
  layout[9] = profile.state[2].offset;
  layout[10] = profile.state[3].offset;
  layout[11] = profile.state[4].offset;
  layout[12] = 0;
  for (uint32_t i = 0; i < profile.mgCount; i++) {
    for (uint32_t j = 0; j < 3; j++)
      layout[40 + 3 * i + j] = profile.mg[i][j];
    layout[12] += profile.mg[i][0] * profile.mg[i][1];
  }
  for (uint32_t i = 0; i < 20; i++)
    layout[16 + i] = profile.work[i].offset;
  return layout;
}

inline std::array<uint32_t, 18688> BootControl(const Config &config,
                                               const std::string &programSHA,
                                               const std::string &environmentSHA) {
  std::array<uint32_t, 18688> control{};
  std::copy(config.begin(), config.end(), control.begin());
  auto mathHash = FromHex(kHash_mathSHA), programHash = FromHex(programSHA),
       deviceHash = FromHex(environmentSHA);
  std::copy(mathHash.begin(), mathHash.end(), control.begin() + 704);
  std::copy(programHash.begin(), programHash.end(), control.begin() + 720);
  std::copy(deviceHash.begin(), deviceHash.end(), control.begin() + 728);
  control[744] = 1u;
  control[745] = 0x33800000u;
  control[746] = 0x3f800000u;
  control[747] = 0u;
  control[18648] = 0x34000000u; // one ULP at 1
  control[18649] = 0x00800000u; // smallest normal
  control[18650] = 0x3f000000u; // factor of one half
  control[18651] = 0x7f800000u; // +infinity
  control[18652] = 0x7fc00000u; // quiet NaN
  control[748] = 0x54442d18u;
  control[749] = 0x3ff921fbu;
  return control;
}

inline std::array<uint32_t, 256> BootHeader(const Profile &profile,
                                           const Config &config,
                                           const Identity &identity) {
  std::array<uint32_t, 256> header{};
  header[0] = kMagic_state;
  header[1] = 1;
  header[2] = kMagic_profile;
  header[3] = profile.sw;
  header[4] = profile.nx;
  header[5] = profile.ny;
  header[6] = config[6];
  Put(header.data() + 8, identity.run);
  Put(header.data() + 10, identity.context);
  Put(header.data() + 12, identity.surface);
  Put(header.data() + 14, identity.geometry);
  header[26] = config[23];
  header[27] = config[21];
  header[28] = config[22];
  header[29] = config[26];
  header[30] = config[20];
  header[206] = config[20];
  header[7] = 1;
  header[202] = config[24];
  header[203] = config[25];
  header[204] = config[27];
  header[205] = config[26];
  if (config[27] != 0) {
    header[200] = 0xffffffffu;
    header[201] = 0xffffffffu;
  }
  for (auto pair : std::array<std::pair<size_t, size_t>, 5>{
           {{32, 76}, {40, 36}, {48, 44}, {56, 52}, {64, 60}}})
    std::copy_n(config.data() + pair.second, 8, header.data() + pair.first);
  return header;
}
} // namespace grs::g13
