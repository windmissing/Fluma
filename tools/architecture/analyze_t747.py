"""Build the standalone U32 probe and generate exact, independent expected words."""
from fractions import Fraction
import json
from pathlib import Path
from random import Random
import re
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory

from analyze_t660_t662 import ROOT, record, sha, write
from analyze_t736 import document


SIGN = 1 << 31
INF = 0x7F800000
QNAN = 0x7FC00000
LIMIT = 49152
CHIP_PROBE = "tools/architecture/t747_u32_probe.comp"
OPS = {"ADD": 0, "SUB": 1, "MUL": 2, "CONVERT": 3, "RELAY": 4}


def power2(n):
    return Fraction(1 << n) if n >= 0 else Fraction(1, 1 << -n)


def decode(bits):
    exponent = (bits >> 23) & 255
    significand = (bits & 0x7FFFFF) | ((1 << 23) if exponent else 0)
    return (-1 if bits & SIGN else 1) * significand * power2(max(exponent, 1) - 150)


def encode(value, zero_sign=0):
    if not value:
        return zero_sign
    sign = SIGN if value < 0 else 0
    value = abs(value)
    e = value.numerator.bit_length() - value.denominator.bit_length()
    if value < power2(e):
        e -= 1
    q = round(value / power2(max(e, -126) - 23))
    if e < -126:
        return sign | q
    if q == 1 << 24:
        e += 1
        q >>= 1
    return sign | (INF if e > 127 else ((e + 127) << 23) | (q & 0x7FFFFF))


def reference(name, a, b, flags, scale):
    if name == "RELAY":
        return a
    if name == "CONVERT":
        value = (a | (b << 32)) * power2(scale)
        return encode(-value if flags else value, flags)
    if name == "SUB":
        b ^= SIGN
    x, y, sign = a & ~SIGN, b & ~SIGN, (a ^ b) & SIGN
    if max(x, y) > INF:
        return QNAN
    if name == "MUL":
        if max(x, y) == INF:
            return QNAN if min(x, y) == 0 else INF | sign
        return encode(decode(a) * decode(b), sign)
    if max(x, y) == INF:
        return QNAN if x == y and sign else a if x == INF else b
    return encode(decode(a) + decode(b), (a & b & SIGN) if x == y == 0 else 0)


def golden_rows():
    # op, first word, second word, expected word, classification of interest.
    examples = [
        ("ADD", 0x00000001, 0, 0x00000001, "op65/op66 denorm input"),
        ("ADD", 0x80000001, 0, 0x80000001, "negative denorm input"),
        ("ADD", 0x007FFFFF, 1, 0x00800000, "carry into normal"),
        ("ADD", 0x3F800000, 0x33800000, 0x3F800000, "op65/op66 tie even"),
        ("ADD", 0x3F800001, 0x33800000, 0x3F800002, "tie odd rounds up"),
        ("ADD", 0x3F800000, 0x33800001, 0x3F800001, "tie plus sticky"),
        ("ADD", 0x3FFFFFFF, 0x33800000, 0x40000000, "tie carry across exponent"),
        ("SUB", 0x00800001, 0x00800000, 1, "normal cancellation produces denorm"),
        ("SUB", 0x00800000, 0x00800001, 0x80000001, "negative cancellation"),
        ("SUB", 0x00800000, 0x00800000, 0, "exact nonzero cancellation"),
        ("SUB", 0x3F800000, 0x33000000, 0x3F800000, "lower binade tie, gap 25"),
        ("SUB", 0x3F800000, 0x33000001, 0x3F7FFFFF, "past lower binade tie"),
        ("SUB", 0x3F800000, 0x32FFFFFF, 0x3F800000, "gap 26 dominant"),
        ("ADD", SIGN, SIGN, SIGN, "negative zero + negative zero"),
        ("ADD", SIGN, 0, 0, "opposed zero signs"),
        ("SUB", SIGN, 0, SIGN, "negative zero - positive zero"),
        ("ADD", 0x7F7FFFFF, 0x72FFFFFF, 0x7F7FFFFF, "below overflow tie"),
        ("ADD", 0x7F7FFFFF, 0x73000000, INF, "overflow tie"),
        ("ADD", INF, SIGN | INF, QNAN, "opposed infinities"),
        ("SUB", INF, INF, QNAN, "same infinities subtract"),
        ("MUL", 0x00800000, 0x3F000000, 0x00400000, "op66 normal inputs produce denorm"),
        ("MUL", 0x80800000, 0x3F000000, 0x80400000, "negative product denorm"),
        ("MUL", 0x00FFFFFF, 0x3F000000, 0x00800000, "tie rounds to min normal"),
        ("MUL", 0x00000001, 0x3F000000, 0, "positive half min denorm"),
        ("MUL", 0x80000001, 0x3F000000, SIGN, "negative half min denorm"),
        ("MUL", 3, 0x3F000000, 2, "tie rounds up to even denorm"),
        ("MUL", SIGN, 0xBF800000, 0, "negative zero times negative one"),
        ("MUL", INF, 0, QNAN, "infinity times zero"),
        ("MUL", INF, 0xBF800000, SIGN | INF, "signed infinity product"),
        ("ADD", 0x7FA12345, 0x3F800000, QNAN, "signaling NaN class"),
        ("MUL", 0xFFC12345, 0x3F800000, QNAN, "quiet NaN class"),
        ("RELAY", 0xFFC12345, 0, 0xFFC12345, "raw word payload survives relay"),
        ("RELAY", 0x80000001, 0, 0x80000001, "raw negative denorm survives relay"),
    ]
    conversions = [
        (1, 0, -149, 0, 1, "minimum denorm"),
        (1, 0, -150, SIGN, SIGN, "negative underflow tie"),
        (3, 0, -150, 0, 2, "denorm odd-to-even tie"),
        (0xFFFFFF, 0, -150, 0, 0x00800000, "round directly to min normal"),
        (0x1000001, 0, 0, 0, 0x4B800000, "U32 integer tie"),
        (0xFFFFFFFF, 0, 0, 0, 0x4F800000, "U32 max"),
        (0x80000000, 0, 0, SIGN, 0xCF000000, "i32 min unsigned magnitude"),
        (1, 0, -32, SIGN, 0xAF800000, "Q32 signed magnitude"),
        (0xFFFFFFFF, 0x7FFFFFFF, -32, 0, 0x4F000000, "Q32 max positive i64"),
        (0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0x5F800000, "two-limb max"),
        (0, 0, 0, SIGN, SIGN, "explicit signed zero input"),
    ]
    rows = [(name, a, b, 0, 0, expected, detail) for name, a, b, expected, detail in examples]
    rows += [("CONVERT", lo, hi, sign, scale, expected, detail)
             for lo, hi, scale, sign, expected, detail in conversions]
    result = []
    for name, a, b, flags, scale, expected, detail in rows:
        assert reference(name, a, b, flags, scale) == expected, detail
        words = [OPS[name] | flags, a, b, scale + 32768 if name == "CONVERT" else 0]
        result.append({"op": name, "case": detail, "input": [f"{v:08x}" for v in words],
                       "expected": f"{expected:08x}"})
    return result


def audit_ranges():
    rng = Random(747)

    def normal(exponent):
        return ((exponent + 127) << 23) | rng.getrandbits(23)

    samples = 0
    covered = dict.fromkeys(("ADD", "SUB", "MUL"), 0)
    edges = (-126, -125, -124, -1, 0, 1, 123, 124, 125, 126, 127)
    for ea in range(-126, 128):
        for eb in edges + (max(-126, ea - 26), max(-126, ea - 25), max(-126, ea - 2), ea):
            a = normal(ea)
            for sign in (0, SIGN):
                b = normal(eb) | sign
                for name in covered:
                    samples += 1
                    effective_opposed = bool(sign) ^ (name == "SUB")
                    if name == "MUL":
                        fast = -126 <= ea + eb <= 125
                    elif effective_opposed:
                        fast = max(ea, eb) >= -124 and abs(ea - eb) >= 2
                    else:
                        fast = max(ea, eb) <= 125
                    if fast:
                        out = reference(name, a, b, 0, 0) & ~SIGN
                        assert 0x00800000 <= out < INF
                        covered[name] += 1
                    if name != "MUL" and ea - eb >= 26:
                        assert reference(name, a, b, 0, 0) == a
    return {"normalInputCases": samples, "fastGuardSamples": covered,
            "scope": "EXACT_RATIONAL_HOST_SANITY_NOT_SHADER_EXECUTION_OR_RANGE_PROOF"}


def compile_probe(compiler):
    raw = (ROOT / CHIP_PROBE).read_bytes().replace(b"\r\n", b"\n")
    core = raw.split(b"const uint signBit", 1)[1].split(b"void main()", 1)[0]
    core_bytes = len(b"const uint signBit" + core)
    vk = raw.replace(b"#version 310 es", b"#version 450", 1)
    gles_flags = ["-S", "comp", "-l", "-q"]
    vk_flags = ["-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis"]
    with TemporaryDirectory(prefix="t747-static-") as name:
        directory = Path(name)
        gles_path, vk_path = (directory / file for file in ("gles.comp", "vulkan.comp"))
        module_path = directory / "u32.spv"
        gles_path.write_bytes(raw)
        vk_path.write_bytes(vk)
        gles_output = subprocess.run([compiler, *gles_flags, gles_path], check=True, capture_output=True, text=True).stdout
        output = subprocess.run([compiler, *vk_flags, "-o", module_path, vk_path], check=True, capture_output=True, text=True).stdout
        module = module_path.read_bytes()
    assert "Local size X: 64" in gles_output
    assert all(re.search(rf"^{name}: .*binding {slot},", gles_output, re.M)
               for slot, name in enumerate(("InputRows", "OutputWords")))
    assert all(word in output for word in ("OpTypeInt 32", "OpUMulExtended", "OpIAddCarry", "OpISubBorrow"))
    assert not any(word in output for word in ("OpTypeFloat", "OpCapability Int64", "DenormPreserve", "RelaxedPrecision"))
    assert len(raw) <= LIMIT and len(vk) <= LIMIT
    return {"sourcePath": CHIP_PROBE, "compilerSHA": sha(compiler.read_bytes()), "arithmeticCoreBytes": core_bytes,
            "gles": {"flags": gles_flags, "sourceBytes": len(raw), "sourceSHA": sha(raw), "staticResult": "PASS"},
            "vulkan": {"flags": vk_flags, "sourceBytes": len(vk), "sourceSHA": sha(vk),
                       "spirvBytes": len(module), "spirvSHA": sha(module), "staticResult": "PASS",
                       "int32Only": True}, "runtimeResult": "NOT_RUN"}


def audit_budgets(core_bytes):
    gles_path = "test/reports/t728/SPLIT.json"
    vk_path = "docs/architecture-T741-evidence-v1.json"
    gles = document(gles_path)["roles"]
    vk = document(vk_path)["vulkanStatic"]["roles"]
    result = []
    for row in vk:
        role = row["role"]
        if role not in gles or gles[role]["kind"] != "compute":
            continue
        sizes = {"gles": gles[role]["bytes"], "vulkan": row["vulkanSourceBytes"]}
        result.append({"role": role, "sourceBytes": sizes, "headroom": {k: LIMIT - v for k, v in sizes.items()},
                       "overageIfEntireProbeCoreAppended": {k: max(0, v + core_bytes - LIMIT) for k, v in sizes.items()}})
    assert len(result) == 6
    return {"existingBuilds": [record(p) for p in (gles_path, vk_path)], "sourceLimitBytes": LIMIT, "roles": result}


def audit_g20():
    path = "test/reports/t491/t738/v5/dm20-v5-final.jsonl"
    rows = [json.loads(line) for line in (ROOT / path).read_text(encoding="utf-8").splitlines()]
    identity, = [row for row in rows if row.get("stage") == "ATTEMPT_IDENTITY"]
    stages = {row["stage"]: row for row in rows if row.get("schema") == "G9BootV1"}
    keys = ("ticket", "pid", "run", "owner", "context")
    assert all(all(row[k] == identity[k] for k in keys) for row in stages.values())
    tail = "test/reports/t491/t738/v5/boot-work-tail.u32"
    assert struct.unpack("<II", (ROOT / tail).read_bytes()) == (0, 0x641647)
    assert stages["FINISH_EXIT"]["raw"] == 37146 and stages["BOOT_AUDIT_OK"]["raw"] == 1
    duration = {name: int(stages[name + "_EXIT"]["ns"]) - int(stages[name + "_ENTER"]["ns"])
                for name in ("SHADER_COMPILE", "PROGRAM_LINK")}
    return {"scoreUnchanged": [4, 12], "dmBoot": {"capture": record(path), "tail": record(tail),
            "attempt": {k: identity[k] for k in (*keys, "processStartTick")}, "durationNs": duration,
            "warmBinaryLoadCapturedInThisAttempt": any("BINARY_LOAD" in r.get("stage", "") for r in rows)}}


def audit(compiler):
    evidence = compile_probe(compiler)
    return {"schema": "T747_G13_U32_FEASIBILITY_OFFLINE_V1", "deviceUsed": False,
            "backendSelected": False, "probe": evidence, "expectedWords": golden_rows(),
            "scopeSanity": audit_ranges(), "budgets": audit_budgets(evidence["arithmeticCoreBytes"]),
            "g20": audit_g20()}


if __name__ == "__main__":
    write("docs/architecture-T747-evidence-v1.json", audit(Path(sys.argv[1]).resolve()))
    print("T747 exact expected words, GLES reflection, Vulkan SPIR-V validation and BOOT scope checked")
