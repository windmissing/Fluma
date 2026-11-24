"""Build the isolated PC M1 inventory candidate from the frozen T830 source."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


PARENT = "e1d3c59b9be83812b2e87a6276773cce2572f395c1b1573cdbd8e3f4a2065257"


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def build(parent, compiler, out):
    source = parent.read_bytes()
    if sha(source) != PARENT:
        raise ValueError("T909_PARENT_SOURCE_SHA")
    original = source.decode("utf-8")
    start = original.index("bool inventory(){")
    end = original.index("void prepare(){", start)
    candidate = original[:start] + Path(__file__).with_name("m1_inventory_v1.glsl").read_text() + original[end:]
    if len(candidate.encode()) > 49152:
        raise ValueError("T909_SOURCE_LIMIT")
    out.mkdir(parents=True, exist_ok=True)
    shader = out / "quantum_candidate_inventory_v1.comp"
    shader.write_bytes(candidate.encode())
    spirv = shader.with_suffix(".spv")
    compiled = subprocess.run([str(compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis",
                               "-o", str(spirv), str(shader)], check=True, text=True, capture_output=True)
    if (re.search(r"validation.*(fail|unavailable|not available)|\berror:", compiled.stdout + compiled.stderr, re.I)
            or not re.search(r"OpExecutionMode %\S+ LocalSize 64 1 1", compiled.stdout)
            or sorted(re.findall(r"OpDecorate %\S+ Binding (\d+)", compiled.stdout)) != list("01234")):
        raise ValueError("T909_SPIRV_CONTRACT")
    doc = dict(parentSHA=PARENT, sourceSHA=sha(shader.read_bytes()), spvSHA=sha(spirv.read_bytes()),
               sourceBytes=shader.stat().st_size, spvBytes=spirv.stat().st_size)
    (out / "receipt.json").write_text(json.dumps(doc, indent=2) + "\n")
    print(json.dumps(doc))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--parent", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    build(args.parent, args.compiler, args.out)
