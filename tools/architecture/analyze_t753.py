"""Reproduce G13 dispatch/trace attribution and check the isolated visibility declaration."""
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write
from analyze_t741 import COMPILE, LIMIT, NATIVE, SIGNED, adapt, doc
from analyze_t750 import CPP, DISPATCH, split_group


LAYOUT = "docs/architecture-T607-g13-layout-v1.json"
GLSL_FLAGS = ["-S", "comp", "-l", "-q"]
SPIRV_FLAGS = ["-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis"]


@dataclass(frozen=True)
class Binding:
    phase: str
    shader_scope: int
    ssbo0: str
    ssbo1: str


def binding(kind, p):
    op = p["opcode"]
    phase = p.get("scope", 0)
    if kind == "QUANTUM" and 12 <= op <= 49:
        assert phase in (1, 2) and p["J"] in (1, 2, 4) and 0 <= p["micro"] < p["J"]
        if phase == 1:
            return Binding("ACTUAL", 0, "CURRENT", "CANDIDATE")
        return Binding("REPLAY", 1, "CURRENT", "REPLAY")
    assert phase == 0 and p.get("J", 0) == p.get("micro", 0) == 0
    if kind == "SOURCE" and 54 <= op <= 59:
        if op <= 56:
            return Binding("SOURCE_ACTUAL", 0, "CURRENT", "CANDIDATE")
        return Binding("SOURCE_REPLAY", 0, "CURRENT", "REPLAY")
    assert (kind == "QUANTUM" and 0 <= op <= 11) or (kind == "SOURCE" and (50 <= op <= 63 or op in (9, 10)))
    if op in (6, 60):
        return Binding("COMPARE", 0, "CANDIDATE", "REPLAY")
    if op in (9, 10):
        return Binding("PACK", 0, "CANDIDATE", "PENDING_PACK")
    return Binding("OUTER", 0, "CURRENT", "CANDIDATE")


def header_passes(path, table):
    raw = path.read_text(encoding="utf-8")
    body = re.search(rf"k_{table}\[\]=\{{(.*?)\}};", raw, re.S)[1]
    return [tuple(map(int, re.findall(r"(\d+)u", line))) for line in body.splitlines() if "{" in line]


def audit_passes(manifest):
    dispatch = doc(DISPATCH)
    layouts = doc(LAYOUT)
    profiles = layouts["profiles"]
    for profile in profiles:
        segments = [(row["wordStart"], row["wordStart"] + row["words"]) for row in profile["CTRL"]]
        assert segments == [(0, 1024), (1024, 2048), (2048, 10240), (10240, 18432), (18432, 18688)]
        assert [row["name"] for row in profile["FULLColdDirectory"]] == [
            "PREFIX", "BASE", "CANDIDATE", "REPLAY", "WORK", "CTRL", "PACK0", "PACK1"]
    canonical_header = ROOT / CPP / "contract_generated.h"
    frozen_header = (Path(manifest["project"]) / manifest["roles"]["OP_48"]["path"]).with_name("contract_generated.h")
    fields = ("opcode", "scope", "J", "micro", "iterationBlock")
    result = {}
    for kind, size in (("QUANTUM", 820), ("SOURCE", 16)):
        rows = dispatch[kind.lower()]
        assert [p["index"] for p in rows] == list(range(size))
        published = [tuple(p.get(k, 0) for k in fields) for p in rows]
        assert published == header_passes(canonical_header, kind.lower()) == header_passes(frozen_header, kind.lower())
        plan = [binding(kind, p) for p in rows]
        runs = []
        for index, rule in enumerate(plan):
            if runs and runs[-1]["route"] == vars(rule):
                runs[-1]["endExclusive"] = index + 1
            else:
                runs.append({"begin": index, "endExclusive": index + 1, "route": vars(rule)})
        result[kind] = {"count": size, "routes": runs, "countByPhase": dict(Counter(p.phase for p in plan)),
                        "binding2": "WORK", "binding3": "CONTROL"}
    assert result["QUANTUM"]["countByPhase"] == {
        "OUTER": 9, "ACTUAL": 404, "REPLAY": 404, "COMPARE": 1, "PACK": 2}
    assert result["SOURCE"]["countByPhase"] == {
        "OUTER": 7, "SOURCE_ACTUAL": 3, "SOURCE_REPLAY": 3, "COMPARE": 1, "PACK": 2}
    quantum = dispatch["quantum"]
    old_addresses = []
    new_addresses = []
    for p in quantum:
        if p["opcode"] == 48 and p["iterationBlock"] == 0:
            index = binding("QUANTUM", p).shader_scope
            old_addresses.append(10240 + 1024 * (p["J"] + p["micro"]) + 384)
            new_addresses.append((2048, 10240)[index] + 1024 * (p["J"] + p["micro"]) + 384)
    assert len(set(old_addresses)) == 7 and len(set(new_addresses)) == 14
    assert 518 + 2 == 520  # Old REPLAY success writes the root opcode slot.
    frozen = Path(manifest["project"]) / CPP / "transport.cpp"
    transport = frozen.read_text(encoding="utf-8")
    assert "p.scope ? replay_ : candidate_" in transport and "locations.at[Scope], p.scope" in transport
    latest_transport = (ROOT / CPP / "transport.cpp").read_text(encoding="utf-8")
    current_tail = latest_transport.split("uint32_t Transport::PumpBounded", 1)[1].split("bool Transport::CopyBuffer", 1)[0]
    assert "glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)" in current_tail
    assert "GL_BUFFER_UPDATE_BARRIER_BIT" not in current_tail and "GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT" not in current_tail
    scene = (ROOT / CPP / "scene.cpp").read_text(encoding="utf-8")
    assert "glVertexAttribIPointer" in scene and "glMemoryBarrier" not in scene
    traces = (ROOT / CPP / "trace.glsl").read_text(encoding="utf-8")
    shader = (ROOT / CPP / "pipeline.comp").read_text(encoding="utf-8")
    assert "uScope==0u?2048u:10240u" in traces and "c[518u+uScope]=uJ" in traces
    assert "c[520]=c[129]" in shader and "c[t+4u]=c[520]" in shader
    return {"authority": [record(p) for p in (DISPATCH, LAYOUT, CPP + "contract_generated.h", CPP + "transport.cpp",
                                               CPP + "pipeline.comp", CPP + "trace.glsl", CPP + "scene.cpp")],
            "t739BuildInputTransport": record(frozen.relative_to(ROOT).as_posix()),
            "t739GeneratedHeader": record(frozen_header.relative_to(ROOT).as_posix()),
            "plan": result, "oldOp48Block0Addresses": old_addresses, "correctedOp48Block0Addresses": new_addresses,
            "oldReplayWritesSelectedToRootOpcodeWord": 520,
            "scopeCompareWordsExcludedPerTrace": list(range(4, 8192, 1024))}


def compile_sources(compiler, manifest):
    receipt = doc(SIGNED)
    signed = Path(receipt["signed"]["path"])
    assert receipt["verifyApp"] == "PASS" and sha(signed.read_bytes()) == receipt["signed"]["sha256"]
    with ZipFile(signed) as archive:
        elf = archive.read(NATIVE)
    assert build_id(elf) == manifest["buildId"] and sha(elf) == manifest["nativeSHA256"]
    roles = []
    with TemporaryDirectory(prefix="t753-static-") as scratch:
        temp = Path(scratch)
        for name, role in manifest["roles"].items():
            if role["kind"] != "compute":
                continue
            original = (Path(manifest["project"]) / role["path"]).read_bytes()
            assert sha(original) == role["sha256"] and elf.count(original + b"\0") == 1
            if name == "OP_48":
                emit, _ = split_group(original)
                block_control, _ = emit((48,))
            roles.append(compile_visibility_role(compiler, temp, name, original))
        assert len(roles) == 6
        native_block = compile_visibility_role(compiler, temp, "VISC_48", block_control)
    return {"signedReceipt": record(SIGNED), "manifest": record(COMPILE), "signedSHA": receipt["signed"]["sha256"],
            "nativeBuildId": build_id(elf), "nativeSHA": sha(elf), "compilerSHA": sha(compiler.read_bytes()),
            "legacyQualificationPreserved": True, "glesFlags": GLSL_FLAGS, "vulkanFlags": SPIRV_FLAGS,
            "fullGroups": roles, "isolatedViscosity": native_block, "runtimeResult": "NOT_RUN"}


def compile_visibility_role(compiler, temp, name, original):
    changed = original
    for block in (b"Candidate", b"Work", b"Control"):
        old = b"buffer " + block + b" {"
        assert changed.count(old) == 1
        changed = changed.replace(old, b"coherent " + old)
    assert len(changed) == len(original) + 27
    vulkan = adapt(changed.decode("utf-8"))
    assert max(len(changed), len(vulkan)) <= LIMIT
    glsl_path = temp / (name + "-gles.comp")
    vk_path = temp / (name + "-vk.comp")
    module = temp / (name + ".spv")
    glsl_path.write_bytes(changed)
    vk_path.write_bytes(vulkan)
    gles_result = subprocess.run([compiler, *GLSL_FLAGS, glsl_path], check=True, capture_output=True, text=True)
    assert "Local size X: 64" in gles_result.stdout
    gles_bindings = dict(re.findall(r"^([A-Za-z]+): offset -1, type ffffffff, size \d+, index \d+, binding (\d+),", gles_result.stdout, re.M))
    declared = {"Base": "0", "Candidate": "1", "Work": "2", "Control": "3"}
    assert gles_bindings.get("Control") == "3" and all(declared[block] == slot for block, slot in gles_bindings.items())
    if name in ("BOOT", "VISC_48"):
        assert gles_bindings == declared
    proc = subprocess.run([compiler, *SPIRV_FLAGS, "-o", module, vk_path], check=True, capture_output=True, text=True)
    asm = proc.stdout
    assert not re.search(r"validation.*(fail|unavailable|not available)|\berror:", asm + proc.stderr, re.I)
    assert "OpMemoryModel Logical GLSL450" in asm
    assert all(f"OpExecutionMode %main {mode} 32" in asm
               for mode in ("DenormPreserve", "SignedZeroInfNanPreserve", "RoundingModeRTE"))
    assert set(re.findall(r"OpMemberDecorate %(\w+) 0 Coherent", asm)) == {"Candidate", "Work", "Control"}
    assert sorted(map(int, re.findall(r"OpDecorate %\S+ Binding (\d+)", asm))) == list(range(5))
    assert "OpDecorate %_arr_uint_uint_80 ArrayStride 16" in asm
    assert re.findall(r"OpMemberDecorate %Dispatch (\d+) Offset (\d+)", asm) == [(str(i), str(4 * i)) for i in range(6)]
    if name == "VISC_48":
        assert "OpControlBarrier %uint_2 %uint_2 %uint_264" in asm
        assert "OpMemoryBarrier %uint_1 %uint_72" in asm
    binary = module.read_bytes()
    return {"role": name, "parentSourceSHA": sha(original), "glesBytes": len(changed), "glesSHA": sha(changed),
            "glesActiveBlocks": {name: int(slot) for name, slot in gles_bindings.items()},
            "vulkanGlslBytes": len(vulkan), "vulkanGlslSHA": sha(vulkan), "spirvBytes": len(binary),
            "spirvSHA": sha(binary), "memoryModel": "Logical GLSL450", "coherentMembers": ["Candidate", "Work", "Control"],
            "controlBarriers": sorted(set(re.findall(r"OpControlBarrier .+", asm))),
            "memoryBarriers": sorted(set(re.findall(r"OpMemoryBarrier .+", asm)))}


if __name__ == "__main__":
    manifest = doc(COMPILE)
    compiler = Path(sys.argv[1]).resolve()
    write("docs/architecture-T753-evidence-v1.json", {
        "schema": "T753_G13_BINDING_VISIBILITY_ARCHITECTURE_V1", "deviceUsed": False,
        "dispatch": audit_passes(manifest), "staticVisibility": compile_sources(compiler, manifest)})
    print("T753 checked 836 original dispatch rows, scoped trace spans, signed sources and static barrier decorations")
