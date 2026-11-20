#pragma once
#ifndef GRS_T692_HOST_READBACK
#error T692 is an isolated host replay interface
#endif
#include "m1/full_transport.h"
#include <atomic>
#include <cstring>
#include <limits>

namespace grs::m1::t692 {
constexpr size_t RootCreditBytes = 16u * 1024 * 1024;
enum class Result { Ok, Identity, Revoked, Budget, Binding, Map, Unmap, GlError };
struct Read {
  GLuint buffer = 0;
  uintptr_t mapped = 0;
  GLenum target = GL_COPY_READ_BUFFER;
  size_t offset = 0, bytes = 0;
  GLbitfield access = GL_MAP_READ_BIT;
  GLenum raw = GL_NO_ERROR;
  bool unmapped = false;
};
struct Root {
  Identity scope;
  RootRequest request;
  AcceptedRoot accepted;
  uint64_t sequence = 0, startedNs = 0, elapsedNs = 0;
  Read bank, events;
  GLenum barrierError = GL_NO_ERROR, restoreError = GL_NO_ERROR;
  Result result = Result::Identity;
};
struct Sink {
  void* context;
  // Reserve preallocated storage, including a charged completion and record.
  uint8_t* (*reserve)(void*, const Root&, size_t payloadBytes) noexcept;
  // The readback succeeded iff result is Ok. Failed storage must be discarded.
  void (*finish)(void*, const Root&) noexcept;
};
inline std::atomic<const Sink*> sink{nullptr};
// Register before the owner starts. Sink and storage must outlive the owner.
inline bool Install(const Sink* value) {
  if (!value || !value->reserve || !value->finish) return false;
  const Sink* empty = nullptr;
  return sink.compare_exchange_strong(empty, value);
}
struct Authority {
  void* context = nullptr;
  bool (*current)(void*, const RootRequest&, const AcceptedRoot&) = nullptr;
};
inline thread_local Authority authority;
}

namespace grs::m1 {
// Existing transport friendship. This definition exists only in the replay TU.
struct FullM1HostAccess {
  static void Observe(FullM1Transport& t, const AcceptedRoot& accepted) {
    using namespace t692;
    const auto* writer = sink.load(std::memory_order_acquire);
    if (!writer) return;
    Root root{};
    root.scope = t.source.identity;
    root.request = t.pending;
    root.accepted = accepted;
    root.sequence = t.source.owner.AcceptedCount();
    root.startedNs = TailObserver::Now();
    const auto& words = accepted.completion;
    const auto& request = t.pending;
    auto current = [&] {
      return authority.current &&
             authority.current(authority.context, request, accepted) &&
             t.source.Current();
    };
    auto finish = [&](Result result) {
      root.result = result;
      root.elapsedNs = TailObserver::Now() - root.startedNs;
      writer->finish(writer->context, root);
    };
    const uint32_t count = words[53];
    if (t.active || t.source.fence || t.quantum.fence ||
        t.submitPhase != SubmitPhase::IDLE ||
        t.source.owner.State() != FlightState::Idle ||
        accepted.bank >= 2 || accepted.bank != t.source.owner.Bank() ||
        accepted.bank != words[33] || accepted.publication != t.lastPublication ||
        accepted.publication != request.publicationSlot ||
        accepted.nonce.lo != request.nonce.lo || accepted.nonce.hi != request.nonce.hi ||
        words != t.tail || (request.kind != 1 && request.kind != 2) ||
        request.kind != words[27] || t.acceptedQuantum != (request.kind == 2) ||
        t.acceptedArena >= 2 || t.acceptedArena != request.eventArena ||
        t.acceptedArena != words[51] || words[52] != request.eventStart ||
        words[54] != request.eventCapacity || words[52] > count ||
        count > request.eventCapacity ||
        request.eventCapacity > (t.acceptedQuantum ? t.quantum.EventCapacity() : t.source.K + 1u)) {
      finish(Result::Identity);
      return;
    }
    const size_t limit = static_cast<size_t>((std::numeric_limits<GLsizeiptr>::max)());
    if (!t.source.words || t.source.words > limit / 4 || size_t(count) > limit / 256) {
      finish(Result::Identity);
      return;
    }
    root.bank.buffer = t.source.banks[accepted.bank];
    root.events.buffer = t.acceptedQuantum ? t.eventArenas[t.acceptedArena]
                                          : t.source.events[t.acceptedArena];
    root.bank.bytes = t.source.words * 4;
    root.events.bytes = size_t(count) * 256;
    if (!root.bank.buffer || !root.events.buffer) {
      finish(Result::Identity);
      return;
    }
    if (!current()) {
      finish(Result::Revoked);
      return;
    }
    if (root.bank.bytes > RootCreditBytes - sizeof(Root) ||
        root.events.bytes > RootCreditBytes - sizeof(Root) - root.bank.bytes) {
      finish(Result::Budget);
      return;
    }
    auto* bytes = writer->reserve(writer->context, root, root.bank.bytes + root.events.bytes);
    if (!bytes) {
      finish(Result::Budget);
      return;
    }
    GLint previous = 0;
    glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &previous);
    const GLenum queryError = glGetError();
    if (queryError != GL_NO_ERROR) {
      root.bank.raw = queryError;
      finish(Result::Binding);
      return;
    }
    if (!current()) {
      finish(Result::Revoked);
      return;
    }
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    root.barrierError = glGetError();
    if (root.barrierError != GL_NO_ERROR) {
      finish(Result::GlError);
      return;
    }
    auto read = [&](Read& span, uint8_t* out) {
      if (!current()) return Result::Revoked;
      if (!span.bytes) return Result::Ok;
      glBindBuffer(GL_COPY_READ_BUFFER, span.buffer);
      span.raw = glGetError();
      if (span.raw != GL_NO_ERROR) return Result::Binding;
      const auto* ptr = glMapBufferRange(span.target, span.offset, GLsizeiptr(span.bytes), span.access);
      span.mapped = reinterpret_cast<uintptr_t>(ptr);
      if (!ptr) {
        span.raw = glGetError();
        return Result::Map;
      }
      std::memcpy(out, ptr, span.bytes);
      span.unmapped = glUnmapBuffer(span.target) == GL_TRUE;
      span.raw = glGetError();
      if (!span.unmapped) return Result::Unmap;
      return span.raw == GL_NO_ERROR ? Result::Ok : Result::GlError;
    };
    auto result = read(root.bank, bytes);
    if (result == Result::Ok) result = read(root.events, bytes + root.bank.bytes);
    glBindBuffer(GL_COPY_READ_BUFFER, static_cast<GLuint>(previous));
    root.restoreError = glGetError();
    if (root.restoreError != GL_NO_ERROR && result == Result::Ok) {
      result = Result::Binding;
    }
    if (result == Result::Ok && !current()) result = Result::Revoked;
    finish(result);
  }
};
}
