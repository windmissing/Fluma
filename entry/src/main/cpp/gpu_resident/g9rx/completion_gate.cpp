#include "completion_gate.h"
#ifdef GRS_G9_BOOT_SOURCE
#include "transport.h"
#endif
namespace grs::g9rx {
uint32_t FrozenExpectedView::Word(size_t at) const {
  const auto &p = permit_;
  auto pair = [&](size_t base, uint64_t value) -> uint32_t {
    return at == base ? uint32_t(value) : uint32_t(value >> 32);
  };
  const uint32_t fixed[] = {0x58455247u,
                            1u,
                            5u,
                            p.kind,
                            kCompletionMagic,
                            2u,
                            kProfileCode,
                            kProfileCode,
                            kSeal,
                            256u,
                            p.kind == 1 ? 12u : 2404u,
                            p.kind == 1 ? 0xffffu : 0x3ffffu};
  if (at < 12)
    return fixed[at];
  if (at < 16)
    return at < 14 ? pair(12, profile_.stateBytes)
                   : pair(14, profile_.workBytes);
  if (at == 16)
    return profile_.eventCapacity;
  if (at == 17)
    return profile_.pathCapacity;
  const uint64_t ids[] = {
      p.nonce,    p.plan,        p.frame,      p.control, p.geometry,
      p.deadline, p.rootBeginNs, p.baseCommit, p.quanta,  p.arenaGeneration};
  if (at >= 18 && at < 38)
    return pair(18 + ((at - 18) / 2) * 2, ids[(at - 18) / 2]);
  const uint32_t banks[] = {p.baseBank, p.candidateBank, p.arena,
                            p.ordinal,  profile_.k,      0};
  if (at >= 38 && at < 44)
    return banks[at - 38];
  const uint64_t scope[] = {
      p.identity.run,        p.identity.owner,       p.identity.context,
      p.identity.inputEpoch, p.identity.pageSession, p.identity.permission,
      p.identity.window,     p.identity.config,      p.identity.surface};
  if (at >= 44 && at < 62)
    return pair(44 + ((at - 44) / 2) * 2, scope[(at - 44) / 2]);
  if (at >= 72 && at < 80)
    return p.parameterSha[at - 72];
  if (at >= 80 && at < 88)
    return seed_[at - 80];
  if (at >= 88 && at < 96)
    return p.layoutSha[at - 88];
  if (at >= 96 && at < 104)
    return shader_[at - 96];
  if (at >= 104 && at < 112)
    return FromHex(
        "b17060c0441e481ec99f3279c76c8b8c94a1013b0fd9045d6b01b6cb827dac77")
        [at - 104];
  if (at >= 120 && at < 128)
#ifdef GRS_G9_BOOT_SOURCE
    return FromHex(SourceRoleSetSHA())[at - 120];
#else
    return FromHex(kPipelineSHA)[at - 120];
#endif
  if (at >= 152 && at < 160)
    return FromHex(kStaticContractSHA)[at - 152];
  if (at >= 160 && at < 168)
    return p.inputSha[at - 160];
  if (at >= 168 && at < 176)
    return FromHex(kInputPolicySHA)[at - 168];
  if (at >= 176 && at < 178)
    return pair(176, p.clockGeneration);
  if (at >= 178 && at < 180)
    return pair(178, p.lifecycleGeneration);
  if (at == 180)
    return 3;
  if (at == 181 || at == 182)
    return 1;
  if (at == 183)
    return 2;
  return 0;
}

bool CompletionGate::Common(const FrozenExpectedView &expected,
                            const Completion256 &w,
                            const NativeEvidence &evidence) {
  const auto &p = expected.profile_;
  const auto &permit = expected.permit_;
  if (!permit.nonce || !permit.plan || permit.kind < 1 || permit.kind > 2 ||
      permit.baseBank >= 5 || permit.candidateBank >= 5 ||
      permit.baseBank == permit.candidateBank || permit.arena > 1 ||
      !permit.arenaGeneration)
    return false;
  const uint32_t count = permit.kind == 1 ? 12 : 2404;
  if (w[0] != kCompletionMagic || w[1] != 2 || w[2] != 256 || w[3] != kSeal ||
      w[4] != kProfileCode || w[5] != permit.kind || w[39] != count ||
      w[40] != count || w[41] != evidence.barriers ||
      evidence.dispatches != count || evidence.barriers != count ||
      evidence.gl || evidence.egl || !evidence.fenceObserved ||
      evidence.fenceNs > permit.deadline ||
      evidence.fenceNs < permit.rootBeginNs || !permit.clockGeneration ||
      !permit.lifecycleGeneration ||
      w[42] != (permit.kind == 1 ? 0xffffu : 0x3ffffu) || w[52] || w[53] ||
      w[63])
    return false;
  const auto &input = expected.input_;
  if (input[0] != 1481788999 || input[1] != 3 || input[2] != kProfileCode ||
      input[3] || input[22] != 2 || input[23] != 2 || input[25] != 1 ||
      HashWords(input.data(), 128) != permit.inputSha ||
      Pair(input.data(), 4) != permit.identity.run ||
      Pair(input.data(), 6) != permit.identity.owner ||
      Pair(input.data(), 8) != permit.identity.context ||
      Pair(input.data(), 10) != permit.nonce ||
      Pair(input.data(), 12) != permit.identity.inputEpoch ||
      Pair(input.data(), 16) > Pair(input.data(), 18) ||
      Pair(input.data(), 18) > Pair(input.data(), 20) ||
      Pair(input.data(), 20) < permit.rootBeginNs ||
      Pair(input.data(), 20) > permit.deadline)
    return false;
  for (size_t i = 34; i < 40; ++i)
    if (input[i])
      return false;
  if ((permit.kind == 1 && input[40] != 1) ||
      (permit.kind == 2 && input[40] != 0 && input[40] != 2))
    return false;
  if (input[40] &&
      (!Pair(input.data(), 41) ||
       Pair(input.data(), 43) > Pair(input.data(), 20) ||
       Pair(input.data(), 20) - Pair(input.data(), 43) > 250000000))
    return false;
  if (permit.kind == 1) {
    for (size_t i = 120; i < 156; ++i)
      if (w[i])
        return false;
    if (Pair(w.data(), 58) != Pair(input.data(), 41))
      return false;
  }
  if (permit.kind == 2 &&
      (w[62] ||
       (input[40] == 2 && Pair(w.data(), 60) != Pair(input.data(), 41))))
    return false;
  if (w[38] && w[38] != 1 && w[38] != 2 && w[38] != 4 && w[38] != 8 &&
      w[38] != 16 && w[38] != 32)
    return false;
  const uint64_t ids[] = {permit.nonce,
                          permit.identity.run,
                          permit.identity.owner,
                          permit.identity.context,
                          permit.identity.surface,
                          permit.geometry,
                          permit.identity.config,
                          permit.identity.inputEpoch,
                          permit.identity.pageSession,
                          permit.plan,
                          permit.baseCommit};
  for (size_t i = 0; i < 11; ++i)
    if (Pair(w.data(), 8 + 2 * i) != ids[i])
      return false;
  if (w[32] != permit.baseBank || w[33] != permit.candidateBank ||
      w[34] != permit.arena || Pair(w.data(), 35) != permit.arenaGeneration ||
      w[43] != w[44] || w[45] != w[46] || w[44] > p.eventCapacity ||
      w[46] > p.pathCapacity || (w[47] & ~63u))
    return false;
  for (size_t i = 0; i < 8; ++i)
    if (w[64 + i] != permit.parameterSha[i] ||
        w[72 + i] != permit.layoutSha[i] || w[80 + i] != permit.shaderSha[i] ||
        w[88 + i] != permit.inputSha[i])
      return false;
  for (size_t i = 156; i < w.size(); ++i)
    if (w[i])
      return false;
  if (permit.kind == 2) {
    if (w[37] && w[37] != 1 && w[37] != 2 && w[37] != 4 && w[37] != 8 &&
        w[37] != 16 && w[37] != 32)
      return false;
    for (uint32_t i = 0; i < 6; ++i) {
      const uint32_t at = 120 + 6 * i, j = 1u << i;
      if (w[47] & (1u << i)) {
        if (w[at] != j || (w[at + 1] != 1 && w[at + 1] != 4))
          return false;
      } else
        for (uint32_t f = 0; f < 6; ++f)
          if (w[at + f])
            return false;
    }
  }
  return true;
}
std::optional<ValidatedRecord>
CompletionGate::Accepted(const FrozenExpectedView &expected,
                         const Completion256 &w,
                         const NativeEvidence &evidence) {
  const auto &p = expected.profile_;
  const auto &permit = expected.permit_;
  if (!Common(expected, w, evidence) || w[7] || w[48] || w[49] || w[50] ||
      w[51])
    return {};
  const uint32_t outcome = w[6];
  if (outcome < 1 || outcome > 3 || permit.kind == 2 && outcome != 1)
    return {};
  if (outcome == 2) {
    if (permit.kind != 1 || Pair(w.data(), 30) != permit.baseCommit ||
        Pair(w.data(), 54) != permit.quanta || w[62] || w[63])
      return {};
  } else {
    uint64_t commit;
    if (!CheckedAdd(permit.baseCommit, 1, commit) ||
        Pair(w.data(), 30) != commit)
      return {};
    uint64_t quanta;
    if (!CheckedAdd(permit.quanta, permit.kind == 2 ? 1 : 0, quanta) ||
        Pair(w.data(), 54) != quanta)
      return {};
  }
  if (permit.kind == 2) {
    if (!w[37] || !w[38] || w[38] > w[37] || w[47] != (w[37] * 2u - 1u))
      return {};
    uint32_t selected = 0;
    for (uint32_t i = 0; i < 6; ++i)
      if (w[120 + 6 * i + 1] == 1) {
        if (selected)
          return {};
        selected = 1u << i;
      }
    if (selected != w[37])
      return {};
  }
  if (permit.kind == 1 && (w[37] || w[38] || w[47] || w[63] ||
                           (outcome == 1 && (w[62] == 0 || w[62] > 244666)) ||
                           (outcome == 3 && w[62])))
    return {};
  return ValidatedRecord(permit, w, expected.Hash(), expected.producer_);
}
std::optional<ValidatedBlockedControlRecord>
CompletionGate::Blocked(const FrozenExpectedView &expected,
                        const Completion256 &w,
                        const NativeEvidence &evidence) {
  const auto &p = expected.profile_;
  const auto &permit = expected.permit_;
  if (!Common(expected, w, evidence) || permit.kind != 2 || w[6] != 4 ||
      w[37] || Pair(w.data(), 30) != permit.baseCommit ||
      Pair(w.data(), 54) != permit.quanta || w[47] != 63 || w[7] == 0 ||
      w[7] > 18)
    return {};
  for (uint32_t i = 0; i < 6; ++i)
    if (w[120 + 6 * i + 1] != 4 || w[120 + 6 * i + 2] == 0)
      return {};
  if (w[48] != w[152] || w[49] != w[153] || w[50] != w[154] || w[51] != w[155])
    return {};
  return ValidatedBlockedControlRecord(
      ValidatedRecord(permit, w, expected.Hash(), expected.producer_));
}
} // namespace grs::g9rx
