"""Build the Windows default-denorm probe from the frozen T916 OP_48 module."""

import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess

FROZEN = "f01959cc0f2fdecff46c953cf6c6073498f531d91bec0ae7d900b35c4c18aceb"
HERE = Path(__file__).resolve().parent


def digest(data):
    return hashlib.sha256(data).hexdigest()


def instructions(words):
    index = 5
    while index < len(words):
        count, op = words[index] >> 16, words[index] & 65535
        if not count or index + count > len(words):
            raise ValueError("Malformed SPIR-V instruction")
        yield op, words[index:index + count]
        index += count


def unpack(data):
    words = struct.unpack(f"<{len(data) // 4}I", data)
    if words[0] != 0x07230203:
        raise ValueError("Expected little-endian SPIR-V")
    return words


def numerical_modes(words):
    return {row[2:] for op, row in instructions(words) if op == 16}


def default_denorm(data):
    if digest(data) != FROZEN:
        raise ValueError("OP_48 is not the published T916 module")
    words = unpack(data)
    result = list(words[:5])
    removed = 0
    for op, row in instructions(words):
        if (op == 17 and row[1:] == (4464,)) or (op == 16 and row[2:] == (4459, 32)):
            removed += 1
        else:
            result.extend(row)
    if removed != 2:
        raise ValueError("Expected one DenormPreserve capability and mode")
    return struct.pack(f"<{len(result)}I", *result)


def check_modes(data):
    words = unpack(data)
    modes = numerical_modes(words)
    capabilities = {row[1] for op, row in instructions(words) if op == 17}
    if {4464, 4465} & capabilities or {(4459, 32), (4460, 32)} & modes:
        raise ValueError("Module must omit both denorm capabilities and modes")
    if not {(4461, 32), (4462, 32)} <= modes:
        raise ValueError("SignedZeroInfNanPreserve and RTE must remain declared")


def array(name, data):
    words = unpack(data)
    lines = [", ".join(f"0x{w:08x}u" for w in words[i:i + 8]) for i in range(0, len(words), 8)]
    return f"constexpr uint32_t {name}[] = {{\n    " + ",\n    ".join(lines) + "\n};\n"

def compile_shader(glslang, source, output):
    result = subprocess.run([str(glslang.resolve()), "-V", "--target-env", "vulkan1.1",
                             "--spirv-val", "--spirv-dis", "-o", str(output), str(source)],
                            check=True, capture_output=True, text=True)
    if re.search(r"validation.*(fail|unavailable|not available)|\berror:", result.stdout + result.stderr, re.I):
        raise ValueError("glslang validation failed")
    return output.read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslang", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    probe_data = compile_shader(args.glslang, HERE / "probe.comp", out / "probe.spv")
    parent = HERE / "op48-t916.comp"
    op48_data = default_denorm(compile_shader(args.glslang, parent, out / "op48-t916.spv"))
    source = parent.read_text(encoding="utf8")
    before = ('spirv_execution_mode(extensions = ["SPV_KHR_float_controls"], '
              'capabilities = [4464, 4466, 4467], 4459, 32);\nspirv_execution_mode(4461, 32);')
    after = ('spirv_execution_mode(extensions = ["SPV_KHR_float_controls"], '
             'capabilities = [4466, 4467], 4461, 32);')
    if source.count(before) != 1:
        raise ValueError("Expected exactly one frozen float-control declaration")
    default_source = out / "op48-default.comp"
    default_source.write_text(source.replace(before, after), encoding="utf8")
    rebuilt = compile_shader(args.glslang, default_source, out / "op48-default.spv")
    if rebuilt != op48_data:
        raise ValueError("Rebuilt and validated OP_48 differs from the frozen module minus its denorm mode")
    for data in (probe_data, op48_data):
        check_modes(data)
    (out / "probe_shaders.inc").write_text(array("kProbe", probe_data) + array("kOp48", op48_data),
                                          encoding="ascii")
    print(f"probe={digest(probe_data)} op48_default={digest(op48_data)}")


if __name__ == "__main__":
    main()
