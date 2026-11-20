#include "trace.h"
#include <iostream>

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  t693::Trace trace;
  if (!trace.open(argv[1], "T693 archived prefix replay")) return 3;
  uint64_t sequence, wall, stamp;
  uint32_t tid, function, edge;
  size_t size, count = 0;
  while (std::cin >> sequence >> wall >> stamp >> tid >> function >> edge >> size) {
    std::cin.get();
    std::string bytes(size, '\0');
    if (!std::cin.read(bytes.data(), size) || std::cin.get() != '\n') return 4;
    if (!trace.append(sequence, wall, tid, function, uint8_t(edge), bytes, stamp == wall ? std::string() : std::to_string(stamp))) return 5;
    ++count;
  }
  trace.close();
  std::cout << count << '\n';
}
