#pragma once
#include "receipt_clock.h"
#include "wire.h"
#include <cstdio>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <sys/stat.h>
#include <unistd.h>
namespace grs::g9rx {
struct LoadedIdentity {
  bool valid = false;
  Digest sha{};
  char receipt[1024]{};
};
// Read bounded PT_NOTE records from the same descriptor that is hashed below.
inline bool ReadBuildId(FILE *f, uint64_t bytes, char (&out)[129]) {
  Elf64_Ehdr eh{};
  if (bytes < sizeof(eh) || fseek(f, 0, SEEK_SET) ||
      fread(&eh, 1, sizeof(eh), f) != sizeof(eh) ||
      memcmp(eh.e_ident, ELFMAG, SELFMAG) ||
      eh.e_ident[EI_CLASS] != ELFCLASS64 ||
      eh.e_ident[EI_DATA] != ELFDATA2LSB ||
      eh.e_phentsize != sizeof(Elf64_Phdr) || eh.e_phnum > 256 ||
      eh.e_phoff > bytes ||
      uint64_t(eh.e_phnum) * sizeof(Elf64_Phdr) > bytes - eh.e_phoff)
    return false;
  bool found = false;
  const char *hex = "0123456789abcdef";
  for (uint32_t i = 0; i < eh.e_phnum; ++i) {
    Elf64_Phdr ph{};
    if (fseek(f, long(eh.e_phoff + uint64_t(i) * sizeof(ph)), SEEK_SET) ||
        fread(&ph, 1, sizeof(ph), f) != sizeof(ph))
      return false;
    if (ph.p_type != PT_NOTE)
      continue;
    if (ph.p_offset > bytes || ph.p_filesz > bytes - ph.p_offset ||
        ph.p_filesz > 1048576)
      return false;
    uint64_t cursor = ph.p_offset, end = cursor + ph.p_filesz;
    while (cursor < end) {
      Elf64_Nhdr note{};
      if (end - cursor < sizeof(note) || fseek(f, long(cursor), SEEK_SET) ||
          fread(&note, 1, sizeof(note), f) != sizeof(note))
        return false;
      cursor += sizeof(note);
      uint64_t names = (uint64_t(note.n_namesz) + 3) & ~uint64_t(3),
               desc = (uint64_t(note.n_descsz) + 3) & ~uint64_t(3);
      if (names > end - cursor || desc > end - cursor - names)
        return false;
      if (note.n_type == NT_GNU_BUILD_ID && note.n_namesz == 4) {
        char name[4]{};
        if (fread(name, 1, 4, f) != 4)
          return false;
        if (!memcmp(name, "GNU", 4)) {
          if (found || !note.n_descsz || note.n_descsz > 64)
            return false;
          unsigned char data[64]{};
          if (fseek(f, long(cursor + names), SEEK_SET) ||
              fread(data, 1, note.n_descsz, f) != note.n_descsz)
            return false;
          for (size_t j = 0; j < note.n_descsz; ++j) {
            out[2 * j] = hex[data[j] >> 4];
            out[2 * j + 1] = hex[data[j] & 15];
          }
          out[2 * note.n_descsz] = 0;
          found = true;
        }
      }
      cursor += names + desc;
    }
  }
  return found && fseek(f, 0, SEEK_SET) == 0;
}
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
inline bool ReceiptPath(const char *input, char (&escaped)[512]) {
  size_t used = 0;
  for (size_t i = 0; input[i]; ++i) {
    unsigned char c = input[i];
    if (c < 32 || used + 3 >= sizeof(escaped))
      return false;
    if (c == '"' || c == '\\')
      escaped[used++] = '\\';
    escaped[used++] = c;
  }
  escaped[used] = 0;
  return used > 0;
}
inline LoadedIdentity ReadLoadedIdentity(const void *function) {
  LoadedIdentity result;
  Dl_info info{};
  if (!dladdr(function, &info) || !info.dli_fname) {
    std::snprintf(result.receipt, 1024, "{\"state\":\"DLADDR_UNAVAILABLE\"}");
    return result;
  }
  FILE *f = std::fopen(info.dli_fname, "rb");
  if (!f) {
    std::snprintf(result.receipt, 1024,
                  "{\"state\":\"MODULE_OPEN_FAILED\",\"errno\":%d}", errno);
    return result;
  }
  struct Close {
    FILE *f;
    ~Close() { std::fclose(f); }
  } close{f};
  struct stat before {
  }, after{}, path{};
  if (fstat(fileno(f), &before) || !S_ISREG(before.st_mode)) {
    std::snprintf(result.receipt, 1024,
                  "{\"state\":\"MODULE_STAT_FAILED\",\"errno\":%d}", errno);
    return result;
  }
  char buildId[129]{}, mappedBuild[129]{}, pathJson[512]{};
  if (!ReadBuildId(f, uint64_t(before.st_size), buildId)) {
    std::snprintf(result.receipt, 1024,
                  "{\"state\":\"ELF_OR_BUILD_ID_INVALID\",\"errno\":%d}",
                  errno);
    return result;
  }
  if (!MappedBuildId(info.dli_fbase, mappedBuild) ||
      std::strcmp(buildId, mappedBuild) ||
      !ReceiptPath(info.dli_fname, pathJson)) {
    std::snprintf(result.receipt, 1024,
                  "{\"state\":\"LOAD_IDENTITY_CHANGED_OR_PATH_UNAVAILABLE\"}");
    return result;
  }
  p011gpu::Sha256 hash;
  std::array<uint8_t, 4096> chunk{};
  uint64_t read = 0;
  while (true) {
    size_t n = std::fread(chunk.data(), 1, chunk.size(), f);
    if (!n)
      break;
    hash.update(chunk.data(), n);
    read += n;
  }
  if (std::ferror(f) || fstat(fileno(f), &after) ||
      stat(info.dli_fname, &path) || before.st_size != int64_t(read) ||
      before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
      before.st_size != after.st_size ||
      before.st_mtim.tv_sec != after.st_mtim.tv_sec ||
      before.st_mtim.tv_nsec != after.st_mtim.tv_nsec ||
      before.st_ctim.tv_sec != after.st_ctim.tv_sec ||
      before.st_ctim.tv_nsec != after.st_ctim.tv_nsec ||
      path.st_dev != after.st_dev || path.st_ino != after.st_ino) {
    std::snprintf(
        result.receipt, 1024,
        "{\"state\":\"LOADED_FILE_CHANGED_OR_READ_FAILED\",\"errno\":%d}",
        errno);
    return result;
  }
  result.sha = Finish(hash);
  int receiptBytes = std::snprintf(
      result.receipt, 1024,
      "{\"state\":\"LOADED_FILE_OBSERVED\",\"sha256\":\"%s\",\"bytes\":\"%"
      "llu\",\"device\":\"%llu\",\"inode\":\"%llu\",\"BuildId\":\"%s\","
      "\"memoryBuildIdMatched\":true,\"path\":\"%s\"}",
      Hex(result.sha).c_str(), (unsigned long long)read,
      (unsigned long long)after.st_dev, (unsigned long long)after.st_ino,
      buildId, pathJson);
  if (receiptBytes <= 0 || receiptBytes >= 1024) {
    std::snprintf(result.receipt, 1024, "{\"state\":\"LOADED_RECEIPT_LIMIT\"}");
    return result;
  }
  result.valid = true;
  return result;
}
} // namespace grs::g9rx
