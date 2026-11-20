#pragma once
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
namespace grs {
inline bool MappedBuildId(const void *base, char (&out)[129]) {
  struct Search {
    uintptr_t base;
    char *out;
    bool valid = false;
  } search{reinterpret_cast<uintptr_t>(base), out};
  dl_iterate_phdr(
      [](dl_phdr_info *info, size_t, void *opaque) -> int {
        auto &state = *static_cast<Search *>(opaque);
        if (info->dlpi_addr != state.base)
          return 0;
        const char *hex = "0123456789abcdef";
        bool found = false;
        for (size_t i = 0; i < info->dlpi_phnum; ++i) {
          const auto &note = info->dlpi_phdr[i];
          if (note.p_type != PT_NOTE)
            continue;
          bool readable = false;
          if (note.p_filesz > 1048576 ||
              note.p_vaddr > UINTPTR_MAX - note.p_filesz)
            return 1;
          for (size_t n = 0; n < info->dlpi_phnum; ++n) {
            const auto &load = info->dlpi_phdr[n];
            if (load.p_type == PT_LOAD && (load.p_flags & PF_R) &&
                load.p_vaddr <= note.p_vaddr &&
                note.p_vaddr - load.p_vaddr <= load.p_memsz &&
                note.p_filesz <= load.p_memsz - (note.p_vaddr - load.p_vaddr))
              readable = true;
          }
          if (!readable)
            return 1;
          const auto *memory =
              reinterpret_cast<const uint8_t *>(state.base + note.p_vaddr);
          size_t cursor = 0;
          while (cursor < note.p_filesz) {
            if (note.p_filesz - cursor < sizeof(Elf64_Nhdr))
              return 1;
            Elf64_Nhdr header{};
            memcpy(&header, memory + cursor, sizeof(header));
            cursor += sizeof(header);
            uint64_t names = (uint64_t(header.n_namesz) + 3) & ~uint64_t(3),
                     desc = (uint64_t(header.n_descsz) + 3) & ~uint64_t(3);
            if (names > note.p_filesz - cursor ||
                desc > note.p_filesz - cursor - names)
              return 1;
            if (header.n_type == NT_GNU_BUILD_ID && header.n_namesz == 4 &&
                !memcmp(memory + cursor, "GNU", 4)) {
              if (found || !header.n_descsz || header.n_descsz > 64)
                return 1;
              for (size_t b = 0; b < header.n_descsz; ++b) {
                uint8_t value = memory[cursor + names + b];
                state.out[2 * b] = hex[value >> 4];
                state.out[2 * b + 1] = hex[value & 15];
              }
              state.out[2 * header.n_descsz] = 0;
              found = true;
            }
            cursor += names + desc;
          }
        }
        state.valid = found;
        return 1;
      },
      &search);
  return search.valid;
}
inline bool BuildIdForAddress(const void *address, char (&out)[129]) {
  Dl_info module{};
  if (dladdr(address, &module) && MappedBuildId(module.dli_fbase, out))
    return true;
  out[0] = 0;
  return false;
}
} // namespace grs
