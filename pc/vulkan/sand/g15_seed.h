#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>

namespace grs::pc::sand {
inline constexpr uint32_t kG15ColumnCount=128;
inline constexpr int64_t kG15Q20=1048576;

// T906 15-B fixture evaluated at column centers in the .10 m box. Round
// the .60 slope inward so the quantized entries satisfy 5*|dB|<=3*dx.
inline std::array<uint32_t,kG15ColumnCount> G15Dune(bool reverse) {
  std::array<uint32_t,kG15ColumnCount> columns{};
  constexpr int64_t bed=(128*kG15Q20+5)/10;
  constexpr int64_t peak=(3072*kG15Q20+50)/100;
  constexpr int64_t slope=3*kG15Q20/5;
  for (uint32_t i=0;i<columns.size();++i) {
    const auto col=reverse ? columns.size()-1-i : i;
    const int64_t distance=std::abs(int64_t(10*col+5)-576);
    const int64_t drop=(slope*distance+5)/10;
    columns[i]=uint32_t(bed+(drop<peak?peak-drop:0));
  }
  return columns;
}
}
