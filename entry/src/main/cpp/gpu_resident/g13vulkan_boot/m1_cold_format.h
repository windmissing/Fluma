#ifndef GRS_M1_COLD_FORMAT_H
#define GRS_M1_COLD_FORMAT_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace grs::g13vk {
using M1ColdWords = std::array<uint32_t, 8>;
inline constexpr char kM1ColdMagic[] = "M1PCOLD1";
inline constexpr char kM1RootMagic[] = "M1PROOT1";
inline constexpr std::size_t kM1ColdMagicBytes = sizeof(kM1ColdMagic) - 1;
inline constexpr std::size_t kM1ColdEnvelopeBytes = kM1ColdMagicBytes + 2 * sizeof(M1ColdWords);
static_assert(kM1ColdEnvelopeBytes == 72);

struct M1ColdAnchor {
  uint64_t run,geometry,nonce,generation,fenceNonce;
  uint32_t bank,publication;
};

constexpr bool M1ColdBackingMatches(const std::array<uint32_t,256> &tail,const M1ColdAnchor &anchor,
                                    bool sourceAllowed,uint64_t eventBytes) {
  const auto wide=[&](size_t i) { return uint64_t(tail[i])|(uint64_t(tail[i+1])<<32); };
  return anchor.nonce && anchor.generation && anchor.fenceNonce==anchor.nonce &&
         wide(4)==anchor.nonce && wide(6)==anchor.run && wide(36)==anchor.generation && wide(38)==anchor.geometry &&
         (tail[27]==2 || (sourceAllowed && tail[27]==1)) && anchor.bank<=1 && anchor.publication<=1 &&
         tail[32]==1-anchor.bank && tail[33]==anchor.bank && tail[35]==anchor.publication &&
         tail[51]<=1 && tail[52]<=tail[53] && tail[53]<=tail[54] && tail[54]<=eventBytes/256;
}

// The Windows PC archive stores both tables as little-endian words.
template <class Write>
bool WriteM1Cold(const M1ColdWords &sizes, const M1ColdWords &sha, const void *payload,
                 std::size_t bytes, Write write, bool rootOnly=false) {
  return write(rootOnly ? kM1RootMagic : kM1ColdMagic, kM1ColdMagicBytes) == kM1ColdMagicBytes &&
         write(sizes.data(), sizeof(sizes)) == sizeof(sizes) &&
         write(sha.data(), sizeof(sha)) == sizeof(sha) &&
         write(payload, bytes) == bytes;
}
} // namespace grs::g13vk
#endif
