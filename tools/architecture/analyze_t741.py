"""Verify same-device GLES evidence and build strict Vulkan candidates offline."""
import json
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write
from analyze_t737 import exact_diagnostic


REPORT = "test/reports/t491/t730b/"
COMPILE = "test/reports/t739/SPLIT.json"
SIGNED = "test/reports/t257/T729-split-3ff632a03a1d.json"
NATIVE = "libs/arm64-v8a/libgrs_g13.so"
LIMIT = 49152
MODES = ("DenormPreserve", "SignedZeroInfNanPreserve", "RoundingModeRTE")
PRELUDE = """#extension GL_EXT_spirv_intrinsics : require
spirv_execution_mode(extensions = ["SPV_KHR_float_controls"], capabilities = [4464, 4466, 4467], 4459, 32);
spirv_execution_mode(4461, 32);
spirv_execution_mode(4462, 32);"""


def doc(relative):
    return json.loads((ROOT / relative).read_bytes())


def t730():
    expected = [1065353217, 1065353216, 0, 0, 7, 0, 0, 0x46503332]
    samples = []
    for path, ticket in (("v4/l3.json", "195"), ("v5/l2.json", "250"),
                         ("v6/l2.json", "196"), ("v7/t739-diag-full-0.json", "180")):
        relative = REPORT + path
        data = doc(relative)
        if data.get("schema") != "G13InitDiagnosticV1":
            data, = exact_diagnostic(data)
        raw = data["bootReadback"]
        assert data["queuedTicket"] == data["firstFailureAttempt"] == raw["attempt"] == ticket
        assert data["bootReadyNs"] == "0" and data["firstConfigFailure"] == "INIT_ERROR_27"
        assert (raw["fence"], raw["firstGl"], raw["mathReadGl"],
                raw["ctrl255"], raw["ctrl7"]) == (0x911A, 0, 0, 0xC0DE1301, 27)
        assert raw["ctrl200To215"] == expected * 2
        assert raw["work65_35_41_47_53_59"] == [0, 1065353216, 1065353216, 0, 1065353216]
        samples.append({"original": record(relative), "readback": raw})
    assert len({item["readback"]["boot"] for item in samples}) == 4
    assert len({item["readback"]["sourceSHA"] for item in samples}) == 1
    qualification = data["bootQualification"]
    assert qualification["effectiveRejectStage"] == "OP65"
    assert qualification["program"]["route"] == "SOURCE_COMPILE"
    assert qualification["op65"]["input744To747"] == [1, 0x33800000, 0x3F800000, 0]
    assert qualification["op66"]["input18648To18652"] == [
        0x34000000, 0x00800000, 0x3F000000, 0x7F800000, 0x7FC00000]
    assert qualification["op66"]["presence"] == [True, True]
    assert qualification["op66"]["mismatchMasks"] == [12, 12]
    return {"samples": samples, "attempt180Qualification": qualification,
            "ready": "NOT_REACHED", "firstSwap": "NOT_REACHED"}


def adapt(source):
    replacements = (
        ("#version 310 es", "#version 450\n" + PRELUDE),
        ("uniform uint uOp,uScope,uJ,uMicro,uBlock,uIndex;",
         "layout(push_constant) uniform Dispatch {uint uOp,uScope,uJ,uMicro,uBlock,uIndex;};"),
        ("uniform uint uL[80];", "layout(std140,set=0,binding=4) uniform LayoutWords {uint uL[80];};"),
    )
    for before, after in replacements:
        assert source.count(before) == 1
        source = source.replace(before, after, 1)
    return source.encode()


def compile_role(compiler, raw, temp, name):
    source = adapt(raw.decode())
    assert len(source) <= LIMIT, (name, len(source))
    path = temp / (name + ".comp")
    path.write_bytes(source)
    module = path.with_suffix(".spv")
    proc = subprocess.run([compiler, "-V", "--target-env", "vulkan1.1", "--spirv-val",
                           "--spirv-dis", "-o", module, path], check=True, capture_output=True, text=True)
    asm = proc.stdout
    assert not re.search(r"validation.*(fail|unavailable|not available)|\berror:", asm + proc.stderr, re.I)
    assert 'OpExtension "SPV_KHR_float_controls"' in asm
    entry, = re.findall(r'OpEntryPoint GLCompute (%.+?) "main"', asm)
    assert f"OpExecutionMode {entry} LocalSize 64 1 1" in asm
    for mode in MODES:
        assert f"OpCapability {mode}" in asm
        assert f"OpExecutionMode {entry} {mode} 32" in asm
    assert f"OpExecutionMode {entry} DenormFlushToZero" not in asm and "RelaxedPrecision" not in asm
    bindings = dict(re.findall(r"OpDecorate (%\S+) Binding (\d+)", asm))
    sets = dict(re.findall(r"OpDecorate (%\S+) DescriptorSet (\d+)", asm))
    assert sorted(int(slot) for slot in bindings.values()) == list(range(5))
    assert all(sets[target] == "0" for target in bindings)
    assert re.findall(r"OpMemberDecorate %Dispatch (\d+) Offset (\d+)", asm) == [
        (str(i), str(i * 4)) for i in range(6)]
    assert "OpDecorate %_arr_uint_uint_80 ArrayStride 16" in asm
    spirv = module.read_bytes()
    return {"role": name, "parentShaderBytes": len(raw), "parentShaderSHA": sha(raw),
            "vulkanSourceBytes": len(source), "vulkanSourceSHA": sha(source),
            "spirvBytes": len(spirv), "spirvSHA": sha(spirv),
            "compileAndBundledValidationExit": proc.returncode,
            "floatModes32": list(MODES), "relaxedPrecisionDecorations": 0,
            "noContractionDecorationsObserved": asm.count("NoContraction")}


def verify_artifact(compiler, history):
    receipt, manifest = doc(SIGNED), doc(COMPILE)
    signed = Path(receipt["signed"]["path"])
    assert sha(signed.read_bytes()) == receipt["signed"]["sha256"] and receipt["verifyApp"] == "PASS"
    with ZipFile(signed) as archive:
        native = archive.read(NATIVE)
    observed = history["attempt180Qualification"]["program"]["loadedModule"]
    assert observed["memoryBuildIdMatched"]
    assert sha(native) == observed["sha256"] == manifest["nativeSHA256"]
    assert build_id(native) == observed["BuildId"] == manifest["buildId"]
    roles = []
    with TemporaryDirectory(prefix="t741-strict-") as scratch:
        for name, role in manifest["roles"].items():
            if role["kind"] != "compute":
                continue
            raw = (Path(manifest["project"]) / role["path"]).read_bytes()
            assert len(raw) == role["bytes"] and sha(raw) == role["sha256"]
            assert raw + b"\0" in native
            roles.append(compile_role(compiler, raw, Path(scratch), name))
    assert len(roles) == 6 and roles[0]["role"] == "BOOT"
    assert roles[0]["parentShaderSHA"] == history["attempt180Qualification"]["program"]["sourceSHA"]
    maximum = max(roles, key=lambda role: role["vulkanSourceBytes"])
    return {"signedReceipt": record(SIGNED), "compileReceipt": record(COMPILE),
            "signedSHA": receipt["signed"]["sha256"], "nativeSHA": sha(native),
            "buildId": observed["BuildId"], "compilerSHA": sha(compiler.read_bytes()),
            "flags": ["-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis"],
            "sizeLimitBytes": LIMIT, "maxRole": maximum["role"],
            "maxVulkanSourceBytes": maximum["vulkanSourceBytes"], "roles": roles}


if __name__ == "__main__":
    history = t730()
    vulkan = verify_artifact(Path(sys.argv[1]).resolve(), history)
    write("docs/architecture-T741-evidence-v1.json", {
        "schema": "T741_G13_NUMERIC_BACKEND_EVIDENCE_V1", "historicalGles": history,
        "vulkanStatic": vulkan, "deviceUsedForThisAudit": False,
        "vulkanDeviceFloatControls": "NOT_MEASURED", "backendSelected": False})
    print(f"T741 verified {len(history['samples'])} GLES readbacks and six Vulkan roles; "
          f"largest source {vulkan['maxVulkanSourceBytes']} B")
