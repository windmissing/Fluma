#include "cold.h"
#include <algorithm>
#include <cstring>
#include <new>
#include <sstream>
namespace grs::g11vis {
namespace {
void Pair(uint32_t *b, size_t at, uint64_t v) {
  b[at] = uint32_t(v);
  b[at + 1] = uint32_t(v >> 32);
}
Digest Finish(p011gpu::Sha256 &hash) {
  const auto raw = hash.finish();
  Digest out{};
  for (size_t i = 0; i < 32; ++i)
    out[i / 4] |= uint32_t(raw[i]) << (8 * (i % 4));
  return out;
}
} // namespace
void Cold::Close(size_t index, const char *reason) {
  auto &s = slots_[index];
  s.cancel = true;
  s.result = reason;
  if (s.reader) {
    s.stage = Stage::RETIRING;
    return;
  }
  if (s.stage == Stage::BUILDING)
    s.credit.BuilderDone();
  s.block.reset();
  s.credit.Reset();
  s.bytes = 0;
  s.stage = Stage::FREE;
  if (builder_ == int(index))
    builder_ = -1;
}
void Cold::Maintain(uint64_t now) {
  for (size_t i = 0; i < 2; ++i) {
    const auto &s = slots_[i];
    if (s.stage != Stage::FREE &&
        (now < s.created || now - s.created >= kTtlNs))
      Close(i, now < s.created ? "CLOCK_REVERSED" : "EXPIRED");
  }
}
void Cold::Invalidate() {
  for (size_t i = 0; i < 2; ++i)
    if (slots_[i].stage != Stage::FREE)
      Close(i, "STALE_SCOPE");
}
bool Cold::Empty() const {
  for (const auto &s : slots_)
    if (s.stage != Stage::FREE || s.block || s.reader)
      return false;
  return true;
}
std::string Cold::Begin(const std::string &expected, const std::string &actual,
                        uint64_t now, bool full,
                        const VisualTransport &transport,
                        const std::array<uint32_t, 256> &quota) {
  Maintain(now);
  if (expected.empty() || expected != actual ||
      !transport.HasValidatedPublication())
    return "{\"state\":\"STALE_EXPORT_TOKEN\"}";
  if (builder_ >= 0)
    return "{\"state\":\"WOULD_BLOCK\"}";
  const auto *parent = transport.Parent();
  const auto *r = transport.Resources();
  if (!parent || !r || full && !parent->Full())
    return "{\"state\":\"FULL_NOT_ARMED\"}";
  const auto *resource = SelectCold(r->parent, r->k, r->v);
  if (!resource)
    return "{\"state\":\"INVALID_PROFILE\"}";
  int index = -1;
  for (int i = 0; i < 2; ++i)
    if (slots_[i].stage == Stage::FREE && !slots_[i].reader &&
        !slots_[i].block) {
      index = i;
      break;
    }
  if (index < 0)
    return "{\"state\":\"WOULD_BLOCK\"}";
  auto &s = slots_[index];
  s.count = transport.ColdSegments(full, s.segments);
  if (!s.count)
    return "{\"state\":\"WOULD_BLOCK\"}";
  s.bytes = full ? resource->fullBytes : resource->rootBytes;
  size_t at = 32768;
  for (size_t i = 0; i < s.count; ++i) {
    s.offsets[i] = at;
    at += s.segments[i].bytes;
  }
  if (at != s.bytes)
    return "{\"state\":\"COLD_LAYOUT_MISMATCH\"}";
  if (!s.credit.Reserve(s.bytes, resource->twoFullBytes, 31, s.request, s.id))
    return "{\"state\":\"WOULD_BLOCK\"}";
  s.block.reset(new (std::nothrow) uint32_t[s.bytes / 4]());
  if (!s.block) {
    s.credit.BuilderDone();
    s.credit.Reset();
    s.bytes = 0;
    return "{\"state\":\"NATIVE_ALLOCATION_FAILED\"}";
  }
  s.stage = Stage::BUILDING;
  s.created = now;
  s.scope = transport.Identity();
  s.token = actual;
  s.current = s.done = 0;
  s.full = full;
  s.cancel = s.reader = false;
  s.gl = 0;
  s.result = "BUILDING";
  s.segmentHash = p011gpu::Sha256{};
  s.archiveHash = p011gpu::Sha256{};
  s.archiveDone = 0;
  builder_ = index;
  auto *b = s.block.get();
  b[0] = kConfigMagic;
  b[1] = 2;
  b[2] = 8192;
  b[4] = r->parent ? kCompositeDmCode : kCompositeR1Code;
  b[5] = full ? 1 : 0;
  b[6] = uint32_t(s.count);
  b[7] = r->k;
  Pair(b, 8, s.bytes);
  Pair(b, 10, s.request);
  Pair(b, 12, s.id);
  Pair(b, 14, now);
  Pair(b, 16, s.scope.run);
  Pair(b, 18, s.scope.context);
  Pair(b, 20, s.scope.surface);
  Pair(b, 22, s.scope.pageSession);
  Pair(b, 24, s.scope.inputEpoch);
  const auto &tail = transport.PublishedTail();
  Pair(b, 26, accept647::Pair(tail.data(), 8));
  b[28] = tail[6];
  b[29] = tail[37];
  b[30] = tail[44];
  b[31] = tail[46];
  std::copy(tail.begin(), tail.end(), b + 256);
  std::copy(transport.FrozenInput().begin(), transport.FrozenInput().end(),
            b + 512);
  std::copy(transport.FrozenConfig().begin(), transport.FrozenConfig().end(),
            b + 640);
  std::copy(quota.begin(), quota.end(), b + 768);
  std::copy(transport.Bootstrap().Receipt().words.begin(),
            transport.Bootstrap().Receipt().words.end(), b + 1536);
  const auto banks = transport.ColdVisualBanks();
  for (size_t i = 0; i < 3; ++i)
    b[32 + i] = banks[i];
  b[35] = r->v;
  b[36] = parent->Profile();
  // Freeze original trial summary/controls while the same joint WORK pin is
  // held.
  auto read = [&](size_t offset, size_t words, size_t dst) {
    glBindBuffer(GL_COPY_READ_BUFFER, transport.WorkBuffer());
    auto *raw = glMapBufferRange(GL_COPY_READ_BUFFER, GLintptr(offset),
                                 GLsizeiptr(words * 4), GL_MAP_READ_BIT);
    if (!raw)
      return false;
    std::memcpy(b + dst, raw, words * 4);
    return glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE;
  };
  const auto &layout = parent->Layout();
  if (!read(layout.work[6].begin, 384, 1024) ||
      !read(layout.work[7].begin, 128, 1408)) {
    Close(index, "HEADER_CAPTURE_FAILED");
    return "{\"state\":\"HEADER_CAPTURE_FAILED\"}";
  }
  const auto error = glGetError();
  if (error) {
    s.gl = error;
    Close(index, "HEADER_GL_FAILED");
    return "{\"state\":\"HEADER_GL_FAILED\"}";
  }
  for (size_t i = 0; i < s.count; ++i) {
    auto *d = b + 2048 + i * 32;
    const auto &seg = s.segments[i];
    d[0] = uint32_t(i + 1);
    d[1] = full && i >= 4 && i < 10 ? uint32_t(i - 4) : 0;
    d[2] = i == 2 ? 1 : i == 3 ? 2 : full && i >= 4 && i < 10 ? 1 : 0;
    bool executed = !(full && i >= 4 && i < 10) || b[1024 + (i - 4) * 64] != 0;
    d[3] = executed ? 2 : 1;
    d[4] = full && i >= 4 && i < 10 ? b[1024 + (i - 4) * 64] : tail[37];
    d[5] = full && i >= 4 && i < 10 ? b[1024 + (i - 4) * 64 + 7] : tail[44];
    d[6] = full && i >= 4 && i < 10 ? b[1024 + (i - 4) * 64 + 8] : tail[46];
    Pair(d, 8, s.offsets[i] / 4);
    Pair(d, 10, seg.bytes / 4);
    d[12] = seg.strideWords;
  }
  return Manifest(s);
}
bool Cold::Pump(VisualTransport &transport, uint64_t now) {
  Maintain(now);
  if (builder_ < 0)
    return false;
  auto &s = slots_[builder_];
  const auto index = size_t(builder_);
  if (!transport.Current() || transport.Busy() || s.cancel) {
    Close(index, "CAPTURE_SCOPE_FAILED");
    return false;
  }
  auto *b = s.block.get();
  if (s.current >= s.count) {
    b[3] = 3235778819u;
    if (s.archiveDone < s.bytes) {
      const auto count = std::min<size_t>(4096, s.bytes - s.archiveDone);
      s.archiveHash.update(reinterpret_cast<const uint8_t *>(b) + s.archiveDone,
                           count);
      s.archiveDone += count;
      s.result = "HASHING";
      return false;
    }
    s.archiveSHA = Finish(s.archiveHash);
    s.credit.BuilderDone();
    s.stage = Stage::READY;
    s.result = "TRACE_READY";
    builder_ = -1;
    return true;
  }
  auto &seg = s.segments[s.current];
  const size_t count = std::min<size_t>(4096, seg.bytes - s.done);
  auto *dst = reinterpret_cast<uint8_t *>(b) + s.offsets[s.current] + s.done;
  const auto *descriptor = b + 2048 + s.current * 32;
  if (descriptor[3] == 2) {
    glBindBuffer(GL_COPY_READ_BUFFER, seg.buffer);
    auto *raw =
        glMapBufferRange(GL_COPY_READ_BUFFER, GLintptr(seg.byteOffset + s.done),
                         GLsizeiptr(count), GL_MAP_READ_BIT);
    if (!raw) {
      Close(index, "GPU_MAP_FAILED");
      return false;
    }
    std::memcpy(dst, raw, count);
    const bool ok = glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE;
    const auto error = glGetError();
    if (!ok || error) {
      s.gl = error;
      Close(index, "GPU_READ_FAILED");
      return false;
    }
  }
  s.segmentHash.update(dst, count);
  s.done += count;
  if (s.done == seg.bytes) {
    const auto digest = Finish(s.segmentHash);
    auto *d = b + 2048 + s.current * 32;
    std::copy(digest.begin(), digest.end(), d + 23);
    d[31] = 3235778819u;
    ++s.current;
    s.done = 0;
    s.segmentHash = p011gpu::Sha256{};
  }
  return false;
}
std::string Cold::Manifest(const Slot &s) const {
  std::ostringstream out;
  out << "{\"revision\":\"G11_VIS_COLD_V2\",\"pageRevision\":\"G11_VIS_SEGMENT_"
         "PAGE_V2\",\"state\":\""
      << s.result << "\",\"traceId\":\"" << s.id << "\",\"requestId\":\""
      << s.request << "\",\"createdNs\":\"" << s.created << "\",\"bytes\":\""
      << s.bytes << "\",\"full\":" << (s.full ? "true" : "false")
      << ",\"token\":" << s.token << ",\"rawGl\":" << s.gl
      << ",\"dataSHA256\":\"" << HexDigest(s.archiveSHA)
      << "\",\"headerWords\":8192,\"segments\":[";
  for (size_t i = 0; i < s.count; ++i) {
    if (i)
      out << ',';
    const auto &seg = s.segments[i];
    out << "{\"id\":" << i + 1 << ",\"name\":\"" << seg.name
        << "\",\"byteOffset\":" << s.offsets[i] << ",\"bytes\":" << seg.bytes
        << ",\"strideWords\":" << seg.strideWords;
    const auto *d = s.block ? s.block.get() + 2048 + i * 32 : nullptr;
    if (d) {
      out << ",\"executed\":" << (d[3] == 2 ? "true" : "false");
      if (d[31]) {
        Digest h{};
        std::copy_n(d + 23, 8, h.begin());
        out << ",\"sha256\":\"" << HexDigest(h) << "\"";
      }
    }
    out << '}';
  }
  out << "],\"dictionaryPath\":\"docs/"
         "architecture-T660-g11-manifest-v2.json\",\"dictionarySHA256\":\""
      << kCompositeLayoutSHA
      << "\",\"markerCURRENTMeaning\":\"frozen root BASE(previous CURRENT)\"}";
  auto text = out.str();
  return text.size() <= 16384 ? text : "{\"state\":\"MANIFEST_UTF8_LIMIT\"}";
}
std::string Cold::State(uint64_t id, uint64_t now) {
  Maintain(now);
  for (const auto &s : slots_)
    if (s.id == id && id)
      return Manifest(s);
  return "{\"state\":\"STALE_TRACE_ID\"}";
}
std::string Cold::Read(uint64_t id, uint32_t segment, uint32_t offset,
                       uint64_t now) {
  Maintain(now);
  if (reading_)
    return "{\"state\":\"COLD_READER_BUSY\"}";
  for (auto &s : slots_) {
    if (s.id != id || !id)
      continue;
    if (s.stage != Stage::READY || !s.block)
      return Manifest(s);
    size_t begin = 0, words = 8192;
    uint32_t stride = 1;
    if (segment) {
      if (segment > s.count)
        return "{\"state\":\"INVALID_SEGMENT\"}";
      begin = s.offsets[segment - 1] / 4;
      words = s.segments[segment - 1].bytes / 4;
      stride = s.segments[segment - 1].strideWords;
    }
    if (offset >= words || offset % stride)
      return "{\"state\":\"INVALID_OFFSET\"}";
    size_t count = std::min<size_t>(1024, words - offset);
    count -= count % stride;
    if (!count || !s.credit.ReadBegin())
      return "{\"state\":\"COLD_READER_BUSY\"}";
    reading_ = s.reader = true;
    struct ReaderEnd {
      Slot &slot;
      bool &active;
      ~ReaderEnd() {
        slot.credit.ReadEnd();
        slot.reader = false;
        active = false;
      }
    } release{s, reading_};
    std::ostringstream out;
    out << "{\"revision\":\"G11_VIS_SEGMENT_PAGE_V2\",\"state\":\"TRACE_"
           "READY\",\"traceId\":\""
        << id << "\",\"segment\":" << segment << ",\"wordOffset\":" << offset
        << ",\"stride\":" << stride << ",\"words\":[";
    for (size_t i = 0; i < count; ++i) {
      if (i)
        out << ',';
      out << s.block[begin + offset + i];
    }
    out << "]}";
    auto text = out.str();
    if (text.size() > 16384)
      return "{\"state\":\"PAGE_UTF8_LIMIT\"}";
    return text;
  }
  return "{\"state\":\"STALE_TRACE_ID\"}";
}
std::string Cold::End(uint64_t id, uint64_t now) {
  Maintain(now);
  for (size_t i = 0; i < 2; ++i)
    if (slots_[i].id == id && id) {
      if (slots_[i].reader)
        return "{\"state\":\"COLD_READER_BUSY\"}";
      Close(i, "ENDED");
      return "{\"state\":\"ENDED\"}";
    }
  return "{\"state\":\"STALE_TRACE_ID\"}";
}
std::string Cold::Capabilities(const VisualTransport &transport) const {
  const auto *r = transport.Resources();
  if (!r)
    return "{\"state\":\"NOT_CONFIGURED\"}";
  const auto *c = SelectCold(r->parent, r->k, r->v);
  std::ostringstream out;
  out << "{\"revision\":\"G11_VIS_COLD_V2\",\"slots\":2,\"readers\":1,"
         "\"builders\":1,\"ttlNs\":\"60000000000\",\"pageWords\":1024,"
         "\"pageBytes\":16384,\"rootBytes\":\""
      << c->rootBytes << "\",\"fullBytes\":\"" << c->fullBytes
      << "\",\"twoSlotBudget\":\"" << c->twoFullBytes << "\"}";
  return out.str();
}
} // namespace grs::g11vis
