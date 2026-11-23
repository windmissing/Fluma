"""Generate and reflect the five G13 steady Vulkan roles for PC qualification."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "architecture"))
from analyze_t750 import split_group

MODES = ("DenormPreserve", "SignedZeroInfNanPreserve", "RoundingModeRTE")
DECLARATION = """#version 450
#extension GL_EXT_spirv_intrinsics : require
spirv_execution_mode(extensions = ["SPV_KHR_float_controls"], capabilities = [4464, 4466, 4467], 4459, 32);
spirv_execution_mode(4461, 32);
spirv_execution_mode(4462, 32);"""


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def substitute(source, old, new, role):
    if source.count(old) != 1:
        raise ValueError(f"{role}: expected exactly one {old!r}")
    return source.replace(old, new, 1)


def t841_arms(emit):
    def field_arm(arm):
        frozen = ("{{if(uOp==OP_ROOT_FINAL_FIELD_AUDIT||(!selected&&c[768u+uScope]==0u)){fieldAudit();\n"
                  "audit(1u);\naudit(2u);\n{drive();}}\nreturn;}}")
        # Preserve the original arm's final return as in the signed-off T841 shader.
        return substitute(arm, frozen, "fieldAudit();\naudit(1u);\naudit(2u);\nreturn;", "OP7_FIELD_ONLY")

    for name, rewrite in (
        ("OP7_FIELD_ONLY", field_arm),
        ("OP7_DRIVE_ONLY", lambda arm: "if(uOp==7u){lane=gl_LocalInvocationID.x;drive();return;}"),
    ):
        raw, deps = emit((7,), lambda op, arm: rewrite(arm) if op == 7 else arm)
        if name == "OP7_FIELD_ONLY":
            assert "fieldAudit" in deps and "audit" in deps and "drive" not in deps
        else:
            assert {"drive","acceptedTime","softExp","failMath"} <= set(deps)
            assert "fieldAudit" not in deps and "c[522]" not in raw.decode().split("if(uOp==7u){")[-1]
        yield name, [7,66], raw
    for name, opcodes in (("OP6",(6,)), ("OP8",(8,)), ("OP12_14",(12,13,14)),
                          ("OP15",(15,)), ("OP16",(16,)), ("OP17",(17,))):
        raw, _ = emit(opcodes)
        yield name, [*opcodes,66], raw


def build(out, compiler):
    manifest = json.loads((out / "assets_manifest.json").read_bytes())
    assert manifest["variant"] == "DEPENDENCY_SPLIT"
    roles = [x for x in manifest["roles"] if x["role"] != "BOOT"]
    assert [x["role"] for x in roles] == ["OP_00", "OP_18", "OP_37", "OP_48", "OP_61"]
    header = ["#pragma once", "#include <cstdint>", "#include <cstddef>", "namespace grs::pc {"]
    records = []
    sources = []
    for role in roles:
        raw = (out / role["entry"]).read_bytes()
        assert sha(raw) == role["sha256"] and len(raw) == role["bytes"]
        sources.append((role["role"], role["opcodes"], raw))
    emit, opcodes = split_group(sources[0][2])
    assert opcodes == tuple(range(18))
    for first, last in ((0, 5), (6, 11), (12, 17), (6, 8), (9, 11), (7, 7)):
        selection = tuple(range(first, last + 1))
        raw, _ = emit(selection)
        sources.append((f"OP_00_{first}_{last}", [*selection, 66], raw))
    sources.extend(t841_arms(emit))
    for name, opcodes, raw in sources:
        src = raw.decode()
        for a, b in (("#version 310 es", DECLARATION),
                     ("uniform uint uOp,uScope,uJ,uMicro,uBlock,uIndex;",
                      "layout(push_constant) uniform Dispatch {uint uOp,uScope,uJ,uMicro,uBlock,uIndex;};"),
                     ("uniform uint uL[80];",
                      "layout(std140,set=0,binding=4) uniform LayoutWords {uint uL[80];};"),
                     ("highp float ulp=", "precise highp float ulp="),
                     ("highp float retained=", "precise highp float retained="),
                     ("highp float produced=", "precise highp float produced=")):
            src = substitute(src, a, b, name)
        encoded = src.encode()
        if len(encoded) > 49152:
            raise ValueError(f"{name}: complete Vulkan source {len(encoded)} > 49152")
        p = out / f"PC_{name}.comp"
        p.write_bytes(encoded)
        module = p.with_suffix(".spv")
        compiler_log = subprocess.run(
            [str(compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis",
             "-o", str(module), str(p)], check=True, text=True, capture_output=True)
        asm = compiler_log.stdout
        if re.search(r"validation.*(fail|unavailable|not available)|\berror:", asm+compiler_log.stderr, re.I):
            raise ValueError(f"{name}: SPIR-V validation failed")
        entry, = re.findall(r'OpEntryPoint GLCompute (%\S+) "main"', asm)
        assert all(f"OpExecutionMode {entry} {mode} 32" in asm for mode in MODES)
        assert f"OpExecutionMode {entry} LocalSize 64 1 1" in asm and "RelaxedPrecision" not in asm
        assert "DenormFlushToZero" not in asm and asm.count("NoContraction") >= 4
        bindings = dict(re.findall(r"OpDecorate (%\S+) Binding (\d+)", asm))
        sets = dict(re.findall(r"OpDecorate (%\S+) DescriptorSet (\d+)", asm))
        assert sorted(bindings.values()) == list("01234") and all(sets.get(k) == "0" for k in bindings)
        assert re.findall(r"OpMemberDecorate %Dispatch (\d+) Offset (\d+)", asm) == [
            (str(i), str(i*4)) for i in range(6)]
        assert "OpDecorate %_arr_uint_uint_80 ArrayStride 16" in asm
        raw_spv = module.read_bytes()
        records.append(dict(role=name, sourceSHA=sha(encoded), sourceBytes=len(encoded), moduleSHA=sha(raw_spv),
                            moduleBytes=len(raw_spv), parentSHA=sha(raw), opcodes=opcodes,
                            bindings=[0,1,2,3,4], floatModes=MODES, noContraction=asm.count("NoContraction")))
        words = struct.unpack(f"<{len(raw_spv)//4}I", raw_spv)
        header.append(f"inline constexpr uint32_t k{name}[] = {{")
        for i in range(0, len(words), 8):
            header.append("  "+",".join(hex(x) for x in words[i:i+8])+",")
        header.append("};")
        p.with_suffix(".spvasm").write_text(asm, encoding="utf8")
    header.append("struct SteadyModule { const char *name, *sha; const uint32_t *words; size_t bytes; };")
    header.append("inline constexpr SteadyModule kSteadyModules[] = {")
    for r in records:
        n = r["role"]
        header.append(f'  {{"{n}","{r["moduleSHA"]}",k{n},sizeof(k{n})}},')
    header.append("};")
    mapping = manifest["opcodeToRole"]
    assert len(mapping) == 67 and set(mapping) == set(range(6))
    header.append("inline constexpr uint8_t kOpcodeToRole[] = {" +
                  ",".join(map(str, mapping)) + "};")
    header.append("}")
    (out/"pc_steady_spirv.h").write_text("\n".join(header)+"\n", encoding="utf-8")
    (out/"pc_steady_assets.json").write_text(json.dumps(records, indent=2)+"\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    build(args.out, args.compiler)
