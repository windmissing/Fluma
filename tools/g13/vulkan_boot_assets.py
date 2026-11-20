"""Build the one T742 BOOT module and verify the Vulkan interface in the binary."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
FLAGS = ["-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis"]
MODES = ["DenormPreserve", "SignedZeroInfNanPreserve", "RoundingModeRTE"]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def replace(source, old, new):
    require(source.count(old) == 1, f"BOOT declaration changed: {old}")
    return source.replace(old, new, 1)


def build(out, compiler):
    evidence = json.loads((ROOT / "docs/architecture-T741-evidence-v1.json").read_bytes())
    approved = next(role for role in evidence["vulkanStatic"]["roles"] if role["role"] == "BOOT")
    parent = (out / "role_BOOT.comp").read_bytes()
    require(sha(parent) == approved["parentShaderSHA"], "BOOT parent differs from T741")
    source = replace(parent.decode(), "#version 310 es", """#version 450
#extension GL_EXT_spirv_intrinsics : require
spirv_execution_mode(extensions = ["SPV_KHR_float_controls"], capabilities = [4464, 4466, 4467], 4459, 32);
spirv_execution_mode(4461, 32);
spirv_execution_mode(4462, 32);""")
    source = replace(source, "uniform uint uOp,uScope,uJ,uMicro,uBlock,uIndex;",
                     "layout(push_constant) uniform Dispatch {uint uOp,uScope,uJ,uMicro,uBlock,uIndex;};")
    source = replace(source, "uniform uint uL[80];",
                     "layout(std140,set=0,binding=4) uniform LayoutWords {uint uL[80];};")
    require(sha(source.encode()) == approved["vulkanSourceSHA"], "BOOT port differs from T741")
    for declaration in ("float subResult=", "highp float ulp=", "highp float retained=",
                        "highp float produced="):
        source = replace(source, declaration, "precise " + declaration)
    glsl = source.encode()
    require(len(glsl) <= 49152, "BOOT exceeds 48 KiB GLSL input cap")
    source_path = out / "T742_BOOT.comp"
    module_path = source_path.with_suffix(".spv")
    source_path.write_bytes(glsl)
    proc = subprocess.run([str(compiler), *FLAGS, "-o", str(module_path), str(source_path)],
                          check=True, capture_output=True, text=True)
    asm = proc.stdout
    require(not re.search(r"validation.*(fail|unavailable|not available)|\berror:", asm + proc.stderr, re.I),
            "BOOT SPIR-V compiler or validator reported an error")
    entry, = re.findall(r'OpEntryPoint GLCompute (%\S+) "main"', asm)
    require('OpExtension "SPV_KHR_float_controls"' in asm, "Missing SPIR-V extension")
    require(set(re.findall(r"OpCapability (\w+)", asm)) == {"Shader", *MODES},
            "Unexpected BOOT capabilities")
    for mode in MODES:
        require(f"OpExecutionMode {entry} {mode} 32" in asm, f"Missing {mode} 32")
    require(f"OpExecutionMode {entry} LocalSize 64 1 1" in asm, "Wrong local size")
    require("RelaxedPrecision" not in asm and "DenormFlushToZero" not in asm, "Relaxed FP32")
    precise_count = asm.count("NoContraction")
    require(precise_count >= 6, "Missing original probe arithmetic decorations")
    bindings = dict(re.findall(r"OpDecorate (%\S+) Binding (\d+)", asm))
    sets = dict(re.findall(r"OpDecorate (%\S+) DescriptorSet (\d+)", asm))
    require(sorted(bindings.values()) == list("01234") and
            all(sets.get(key) == "0" for key in bindings), "Wrong descriptor set layout")
    require(re.findall(r"OpMemberDecorate %Dispatch (\d+) Offset (\d+)", asm) ==
            [(str(i), str(4 * i)) for i in range(6)], "Wrong push layout")
    require("OpDecorate %_arr_uint_uint_80 ArrayStride 16" in asm, "Wrong layout UBO stride")
    config_file = ROOT / "entry/src/main/ets/physics/G13ResearchConfig.ets"
    config = config_file.read_text(encoding="utf8")
    words = json.loads(re.search(r"new Uint32Array\((\[[\d ,]+\])\)", config)[1])
    seed = json.loads(re.search(r'static seed\(\): string \{ return (".*");', config)[1])
    require(len(words) == 128 and words[4:6] == [64, 64] and words[7] == 0
            and struct.pack("<8I", *words[60:68]).hex() == sha(seed.encode())
            and json.loads(seed) == {"mode": "REST"},
            "Canonical REST profile changed")
    config_bytes = struct.pack("<128I", *words)
    module = module_path.read_bytes()
    version = subprocess.check_output([str(compiler), "--version"], text=True).splitlines()[0]
    receipt = {
        "role": "BOOT", "opcodes": [64, 65, 66],
        "parentShaderSHA": sha(parent), "parentShaderBytes": len(parent),
        "t741VulkanSourceSHA": approved["vulkanSourceSHA"],
        "sourceSHA": sha(glsl), "sourceBytes": len(glsl),
        "moduleSHA": sha(module), "moduleBytes": len(module),
        "compiler": version, "compilerSHA": sha(compiler.read_bytes()), "flags": FLAGS,
        "compilerAndValidatorExit": proc.returncode, "floatModes32": MODES,
        "sourceLimitBytes": 49152, "noContractionDecorations": precise_count,
        "configSHA": sha(config_bytes), "configSelfSHA": config_bytes[304:336].hex(),
        "seedReceipt": json.loads(seed), "seedReceiptSHA": sha(seed.encode()),
        "seedConfigSHA": config_bytes[240:272].hex(),
    }
    (out / "T742_BOOT.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf8")
    (out / "T742_BOOT.spvasm").write_text(asm, encoding="utf8")
    words = struct.unpack(f"<{len(module) // 4}I", module)
    header = ["#pragma once", "#include <cstdint>", "namespace grs::g13vk {"]
    for name, key in (("Module", "moduleSHA"), ("Source", "sourceSHA"),
                      ("Parent", "parentShaderSHA"), ("Config", "configSHA"),
                      ("SeedReceipt", "seedReceiptSHA")):
        header.append(f'inline constexpr char k{name}SHA[] = "{receipt[key]}";')
    header.append("inline constexpr uint32_t kRestConfig[] = {")
    header.append(",".join(str(word) + "u" for word in struct.unpack("<128I", config_bytes)) + "};")
    header.append("alignas(4) inline constexpr uint32_t kBootSpirv[] = {")
    for i in range(0, len(words), 8):
        header.append("  " + ", ".join(f"0x{word:08x}u" for word in words[i:i+8]) + ",")
    (out / "vk_boot_spirv.h").write_text("\n".join(header + ["};", "}", ""]), encoding="utf8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    args = parser.parse_args()
    build(args.out, args.compiler.resolve())
