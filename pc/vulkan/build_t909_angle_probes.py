"""Generate T907's Q20 angle table and compile its private operator probes."""
import argparse
from decimal import Decimal, ROUND_HALF_EVEN, localcontext
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
import subprocess


PI = Decimal("3.141592653589793238462643383279502884197169399375105820974944592307816406286"
             "20899862803482534211706798214808651328230664709384460955")
Q20 = 1 << 20
LIMIT = 45 * 256


def table(precision):
    with localcontext() as context:
        context.prec = precision
        step = PI / (180 * 256)
        square = step * step
        sin_term, cos_term = step, Decimal(1)
        increment_sin, increment_cos = sin_term, cos_term
        for n in range(1, 32):
            sin_term *= -square / ((2 * n) * (2 * n + 1))
            cos_term *= -square / ((2 * n - 1) * (2 * n))
            increment_sin += sin_term
            increment_cos += cos_term
        s, c = Decimal(0), Decimal(1)
        positive = []
        for _ in range(LIMIT + 1):
            positive.append(tuple(int((v * Q20).to_integral_value(rounding=ROUND_HALF_EVEN)) for v in (s, c)))
            s, c = s * increment_cos + c * increment_sin, c * increment_cos - s * increment_sin
    return [(-s, c) for s, c in reversed(positive[1:])] + positive


def build(compiler, directory):
    directory.mkdir(parents=True, exist_ok=True)
    angles = table(75)
    assert angles == table(105)
    assert angles[0] == (-741455, 741455) and angles[LIMIT] == (0, Q20) and angles[-1] == (741455, 741455)
    raw = b"".join(struct.pack("<ii", *pair) for pair in angles)
    assert len(raw) == 184328
    (directory / "g15_angle_q20.i32le").write_bytes(raw)
    result = {"angleTable": {"sha256": sha256(raw).hexdigest(), "rows": len(angles), "bytes": len(raw)}}
    for name, count in (("g15_support_v1", 3), ("g16_isolated_contact_v1", 2), ("g15_pose_v1", 3),
                        ("g15_flux_v1", 3), ("g15_corrector_v1", 3), ("g15_origin_v1", 4)):
        if name == "g15_pose_v1":
            source = directory / (name + ".comp")
            original = Path(__file__).with_name("g15_support_v1.comp").read_text()
            source.write_bytes(original.replace("#version 450\n", "#version 450\n#define G15_POSE\n", 1).encode())
        elif name in ("g15_flux_v1", "g15_corrector_v1"):
            template = Path(__file__).with_name("g15_flux_v1.comp").read_text()
            if name == "g15_corrector_v1":
                template = template.replace("#version 450\n", "#version 450\n#define G15_CORRECTOR\n", 1)
            else:
                # Keep the shipped standalone face module byte-compatible.
                template = re.sub(r"\n#ifdef G15_CORRECTOR\nuint midpoint.*?\n#endif\n", "", template, flags=re.S)
                template = re.sub(r" if\(i<faceCount&&i<faces.length\(\)&&i<result.length\(\)\)\{.*?\n }",
                                  " if(i<faceCount&&i<faces.length()&&i<result.length())result[i]=evaluate(i);",
                                  template, flags=re.S)
                template = template.replace("evaluate(uint i,InputFace f)", "evaluate(uint i)")
                template = template.replace(" bool leftWall=i==0u", " InputFace f=faces[i];bool leftWall=i==0u", 1)
            parent = Path(__file__).resolve().parents[2] / "entry/src/main/cpp/gpu_resident/g9dm/frozen_t647/corrected_reference.comp"
            wide = parent.read_text(encoding="utf-8").split("bool g9Zero(", 1)[1].split("struct G9EnergyInterval", 1)[0]
            support = Path(__file__).with_name("g15_support_v1.comp").read_text()
            pose = support[support.index("u64vec2 mulWord"):support.index("shared bool groupValid;")+len("shared bool groupValid;")]
            source = directory / (name + ".comp")
            source.write_bytes(template.replace("/* T647_WIDE */", "bool g9Zero(" + wide).replace("/* G15_POSE */", pose).encode())
        else:
            source = Path(__file__).with_name(name + ".comp")
        module = directory / (name + ".spv")
        compilation = subprocess.run([str(compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis",
                                      "-o", str(module), str(source)], text=True, capture_output=True)
        if compilation.returncode:
            raise RuntimeError(compilation.stdout + compilation.stderr)
        asm = compilation.stdout
        assert source.stat().st_size <= 49152
        assert sorted(re.findall(r"OpDecorate %\S+ Binding (\d+)", asm)) == list(map(str, range(count)))
        assert name != "g16_isolated_contact_v1" or "NoContraction" in asm
        result[name] = {"spvSHA": sha256(module.read_bytes()).hexdigest(), "spvBytes": module.stat().st_size,
                        "sourceSHA": sha256(source.read_bytes()).hexdigest(), "sourceBytes": source.stat().st_size}
    (directory / "receipt.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    build(args.compiler, args.out)
