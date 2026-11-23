#include "../../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_cold_format.h"
#include "../../entry/src/main/cpp/gpu_resident/g13flow/wire.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

int main() {
  using namespace grs::g13vk;
  const M1ColdWords sizes{1, 2, 3, 4, 5, 6, 7, 8};
  std::array<uint8_t, 36> payload{};
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = uint8_t(i);
  const auto sha = grs::g13::Hash(payload.data(), payload.size());
  for (int shortSegment = 0; shortSegment <= 4; ++shortSegment) {
    int calls = 0;
    std::vector<uint8_t> bytes;
    const auto write = [&](const void *data, size_t count) {
      if (++calls == shortSegment) --count;
      const auto *first = static_cast<const uint8_t *>(data);
      bytes.insert(bytes.end(), first, first + count);
      return count;
    };
    const bool complete = WriteM1Cold(sizes, sha, payload.data(), payload.size(), write);
    assert(complete == (shortSegment == 0));
    assert(calls == (shortSegment ? shortSegment : 4));
    if (complete) {
      assert(bytes.size() == kM1ColdEnvelopeBytes + payload.size());
      assert(std::memcmp(bytes.data(), "M1PCOLD1", 8) == 0);
      for (size_t i = 0; i < sizes.size(); ++i)
        for (size_t byte = 0; byte < sizeof(uint32_t); ++byte)
          assert(bytes[8 + 4 * i + byte] == uint8_t(sizes[i] >> (8 * byte)));
      const auto expectedSha = grs::g13::Hex(sha.data());
      const char *digits = "0123456789abcdef";
      for (size_t i = 0; i < 32; ++i) {
        assert(digits[bytes[40 + i] >> 4] == expectedSha[2 * i]);
        assert(digits[bytes[40 + i] & 15] == expectedSha[2 * i + 1]);
      }
      assert(std::memcmp(bytes.data() + 72, payload.data(), payload.size()) == 0);
    }
  }
  std::cout << "M1PCOLD1 complete write and four short writes passed\n";
}
