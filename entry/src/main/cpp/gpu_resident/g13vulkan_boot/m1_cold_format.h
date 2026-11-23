#ifndef GRS_M1_COLD_FORMAT_H
#define GRS_M1_COLD_FORMAT_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace grs::g13vk {
using M1ColdWords = std::array<uint32_t, 8>;
inline constexpr char kM1ColdMagic[] = "M1PCOLD1";
inline constexpr std::size_t kM1ColdMagicBytes = sizeof(kM1ColdMagic) - 1;
inline constexpr std::size_t kM1ColdEnvelopeBytes = kM1ColdMagicBytes + 2 * sizeof(M1ColdWords);
static_assert(kM1ColdEnvelopeBytes == 72);

// The Windows PC archive stores both tables as little-endian words.
template <class Write>
bool WriteM1Cold(const M1ColdWords &sizes, const M1ColdWords &sha, const void *payload,
                 std::size_t bytes, Write write) {
  return write(kM1ColdMagic, kM1ColdMagicBytes) == kM1ColdMagicBytes &&
         write(sizes.data(), sizeof(sizes)) == sizeof(sizes) &&
         write(sha.data(), sizeof(sha)) == sizeof(sha) &&
         write(payload, bytes) == bytes;
}
} // namespace grs::g13vk
#endif
