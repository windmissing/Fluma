#include "entry/src/main/cpp/gpu_resident/g13flow/boot_contract.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
std::vector<uint32_t> ReadWords(const std::filesystem::path &path, size_t count) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  const auto size = static_cast<std::streamsize>(count * sizeof(uint32_t));
  if (!file || file.tellg() != size) throw std::runtime_error("FIXTURE_SIZE");
  std::vector<uint32_t> words(count);
  file.seekg(0);
  if (!file.read(reinterpret_cast<char *>(words.data()), size)) throw std::runtime_error("FIXTURE_READ");
  return words;
}
}

int main(int argc, char **argv) {
  try {
    if (argc != 3) return 2;
    const auto words = ReadWords(argv[1],128);
    grs::g13::Config config{};
    std::copy(words.begin(), words.end(), config.begin());
    const auto *profile = grs::g13::FindProfile(config[4],config[5]);
    if (!profile || profile->nx != profile->ny || (profile->nx != 16 && profile->nx != 32)) return 3;
    const auto diagnostic = grs::g13::ValidateConfig(config,true);
    const auto production = grs::g13::ValidateConfig(config,false);
    if (diagnostic || !production) return 4;
    const auto seed = ReadWords(argv[2],profile->sw);
    const auto digest = grs::g13::Hash(seed.data(),seed.size()*4);
    if (!std::equal(digest.begin(),digest.end(),config.begin()+60) ||
        seed[0] != grs::g13::kMagic_state || seed[1] != 1 || seed[2] != grs::g13::kMagic_profile ||
        seed[3] != profile->sw || seed[4] != profile->nx || seed[5] != profile->ny || seed[6] != config[6] ||
        std::any_of(seed.begin()+223,seed.begin()+256, [](auto w){return w != 0;})) return 5;
    const auto boot = grs::g13::BootLayout(*profile);
    std::cout << "{\"shape\":" << profile->nx << ",\"diagnostic\":" << diagnostic
              << ",\"production\":" << production << ",\"stateWords\":" << boot[4]
              << ",\"workWords\":" << boot[5] << ",\"packWords\":" << boot[6]
              << ",\"mgLevels\":" << boot[7] << ",\"rhsWord\":" << boot[28] << "}\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 6;
  }
}
