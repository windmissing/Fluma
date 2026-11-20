"""Audit T743/T741 originals and the impact of changing the numeric contract."""
from decimal import Decimal, ROUND_HALF_EVEN, localcontext
import json
from pathlib import Path
import re
import struct
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write


T743 = "test/reports/t491/t743/vk-diag-1.json"
T742 = "test/reports/t742/compile.json"
SIGNED = "test/reports/t257/T743-vk.json"
T741 = "docs/architecture-T741-evidence-v1.json"
RX_LOG = "test/reports/t491/t738/v3/h-warm.txt"
DM_LOG = "test/reports/t491/t738/v3/dm20-v3-final.jsonl"


def doc(path):
    return json.loads((ROOT / path).read_bytes())


def vk_version(number):
    return [number >> 22 & 0x7F, number >> 12 & 0x3FF, number & 0xFFF]


def verify_modes(module):
    words = struct.unpack(f"<{len(module) // 4}I", module)
    assert words[0] == 0x07230203
    entry_points, exec_modes = [], []
    index = 5
    while index < len(words):
        count, opcode = words[index] >> 16, words[index] & 0xFFFF
        assert count > 0 and index + count <= len(words)
        operands = words[index + 1:index + count]
        if opcode == 15:
            assert operands[0] == 5
            entry_points.append(operands[1])
        if opcode == 16:
            exec_modes.append(operands)
        index += count
    entry, = entry_points
    modes = {ins[1]: list(ins[2:]) for ins in exec_modes if ins[0] == entry}
    assert all(modes.get(mode) == [32] for mode in (4459, 4461, 4462))
    assert 4460 not in modes and modes[17] == [64, 1, 1]
    return modes


def vulkan():
    diagnostic, compile_receipt, sign = doc(T743), doc(T742), doc(SIGNED)
    source = compile_receipt["module"]
    assert diagnostic["state"] == "FAILED" and diagnostic["surfaceCreated"]
    assert diagnostic["stage"] == "PHYSICAL_DEVICE_QUALIFICATION"
    assert diagnostic["detail"] == "NO_ELIGIBLE_PHYSICAL_DEVICE"
    assert not any(key in diagnostic for key in ("selectedDevice", "queueFamily", "op65", "op66"))
    device, = diagnostic["devices"]
    assert device["deviceName"] == "Maleoon 935F"
    assert device["qualification"] == "FP32_PROPERTIES_MISSING"
    assert [device[name] for name in ("shaderDenormPreserveFloat32", "shaderRoundingModeRTEFloat32",
                                     "shaderSignedZeroInfNanPreserveFloat32")] == [0, 1, 1]
    assert "shaderDenormFlushToZeroFloat32" not in device
    assert sign["verifyApp"] == "PASS" and sign["input"]["sha256"] == compile_receipt["unsignedHapSHA256"]
    package = Path(sign["signed"]["path"])
    assert sha(package.read_bytes()) == sign["signed"]["sha256"]
    with ZipFile(package) as archive:
        native = archive.read(compile_receipt["nativePath"])
    assert sha(native) == compile_receipt["nativeSHA256"]
    assert build_id(native) == diagnostic["mappedBuildId"] == compile_receipt["buildId"]
    assets = Path(compile_receipt["project"]) / (
        "entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/g13vulkan_boot/g13_assets")
    module = (assets / "T742_BOOT.spv").read_bytes()
    glsl = (assets / "T742_BOOT.comp").read_bytes()
    assert len(module) == source["moduleBytes"] and native.count(module) == 1
    assert sha(module) == source["moduleSHA"] == diagnostic["moduleSHA"]
    assert len(glsl) == source["sourceBytes"] and sha(glsl) == source["sourceSHA"] == diagnostic["sourceSHA"]
    assert source["parentShaderSHA"] == diagnostic["parentSourceSHA"]
    assert (diagnostic["configSHA"], diagnostic["seedSHA"]) == (source["configSHA"], source["seedReceiptSHA"])
    return {"original": record(T743), "compileReceipt": record(T742), "signReceipt": record(SIGNED),
            "signedSHA": sign["signed"]["sha256"], "diagnostic": diagnostic,
            "loaderVersion": vk_version(diagnostic["loaderApi"]), "effectiveVersion": vk_version(device["effectiveApi"]),
            "actualModuleExecutionModes": verify_modes(module), "pipeline": "NOT_REACHED",
            "fp32Output": "NOT_REACHED", "flushToZeroCapabilityReported": False,
            "hardwareInternalReason": "UNDETERMINED"}


def gles():
    m = doc(T741)["historicalGles"]
    q = m["attempt180Qualification"]
    assert len(m["samples"]) == 4 and q["effectiveRejectStage"] == "OP65"
    assert q["op65"]["input744To747"][0] == 1 and q["op65"]["work35_41_47_53_59"][3:] == [0, 0x3F800000]
    assert q["op66"]["mismatchMasks"] == [12, 12]
    assert q["op66"]["presence"] == [True, True]
    return {"t741": record(T741), "rawTickets": [s["readback"]["attempt"] for s in m["samples"]],
            "observed": "SUBNORMAL_INPUT_AND_RESULT_FLUSH_TO_ZERO",
            "bootSHA": q["program"]["sourceSHA"], "configSHA": q["scope"]["configRawSHA"],
            "seedSHA": q["scope"]["seedReceiptSHA"]}


def exp_boundary():
    with localcontext() as context:
        context.prec = 100
        ulp = Decimal(2) ** -149
        bound = Decimal(2) ** -20
        samples = []
        for argument in (-88, -90, -103):
            ideal = Decimal(argument).exp()
            bits = int((ideal / ulp).to_integral_value(rounding=ROUND_HALF_EVEN))
            assert 0 < bits < 0x00800000
            error = abs(bits * ulp - ideal) / ideal
            samples.append({"x": argument, "roundedFP32": f"0x{bits:08x}", "relativeErrorOfRounding": f"{error:.12E}",
                            "roundingWithinT628Bound": error <= bound, "zeroRelativeError": 1})
        assert [s["roundingWithinT628Bound"] for s in samples] == [True, True, False]
    return {"contract": record("docs/architecture-T628-math-domain-v1.json"),
            "scope": "MATH_DOMAIN_COUNTEREXAMPLE_NOT_PRODUCT_EXECUTION",
            "reference": "decimal(exp, precision=100), RNE to integer units of 2^-149",
            "samples": samples,
            "normalInputsThatProduceSubnormal": [
                {"op": "mul", "inputBits": ["0x00800000", "0x3f000000"], "rneBits": "0x00400000"},
                {"op": "sub", "inputBits": ["0x00800001", "0x00800000"], "rneBits": "0x00000001"}]}


def progress():
    log = (ROOT / RX_LOG).read_text(encoding="utf-8-sig")
    phases = {}
    for line in log.splitlines():
        match = re.search(r"\s(43774|45505)\s\d+.*G9RxConfigure: ticket=1 ns=(\d+) phase=(\w+).*mappedBuildId=(\w+) sourceSHA=(\w+)", line)
        if match:
            pid, ns, phase, build, source = match.groups()
            phases.setdefault(pid, {})[phase] = {"ns": int(ns), "buildId": build, "sourceSHA": source}
    cold, warm = phases["43774"], phases["45505"]
    assert "SHADER_COMPILE_EXIT" in cold and "PROGRAM_LINK_EXIT" in cold
    assert "SHADER_COMPILE_EXIT" not in warm and "PROGRAM_LINK_EXIT" not in warm
    assert cold["BOOT_AUDIT_OK"]["buildId"] == warm["BOOT_AUDIT_OK"]["buildId"]
    assert cold["BOOT_AUDIT_OK"]["sourceSHA"] == warm["BOOT_AUDIT_OK"]["sourceSHA"]
    dm = [json.loads(row) for row in (ROOT / DM_LOG).read_bytes().splitlines()]
    assert dm[-1]["state"] == "BOOT_AUDIT_OK" and not dm[-1]["rootsAuthorized"]
    scores = {"G13": [0.5, 0.5, 0.5, 0], "RX": [0.5, 0.5, 0.5, 0], "DM": [0.5, 0.5, 0, 0]}
    return {"metrics": ["coldCompile", "link", "warmCache", "firstValidSwap"],
            "scores": scores, "earned": sum(map(sum, scores.values())), "total": 12,
            "rxOriginal": record(RX_LOG), "dmOriginal": record(DM_LOG),
            "rxColdBootNs": cold["BOOT_AUDIT_OK"]["ns"] - cold["ENQUEUED"]["ns"],
            "rxNewProcessBootNs": warm["BOOT_AUDIT_OK"]["ns"] - warm["ENQUEUED"]["ns"]}


if __name__ == "__main__":
    vk, gl = vulkan(), gles()
    d = vk["diagnostic"]
    assert (d["parentSourceSHA"], d["configSHA"], d["seedSHA"]) == (gl["bootSHA"], gl["configSHA"], gl["seedSHA"])
    result = {"schema": "T744_G13_QUALIFICATION_ROUTE_V1", "deviceUsedForThisAudit": False,
              "vulkan": vk, "gles": gl, "numericCounterexamples": exp_boundary(), "g20": progress()}
    write("docs/architecture-T744-evidence-v1.json", result)
    print("T744 verified package, actual SPIR-V modes, distinct device results and T628 counterexamples")
