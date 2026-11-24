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


def angle_witness(source):
    original = ("int low=-605394,high=605394;for(uint c=0u;c<127u;c++){int d=int(col(10u,c+1u))-"
                "int(col(10u,c));low=max(low,d-605394);high=min(high,d+605394);}")
    replacement = """int low=-605394,high=605394;
 uint lowFace=127u,highFace=127u;
 for(uint c=0u;c<127u;c++){
  int d=int(col(10u,c+1u))-int(col(10u,c));
  if(d-605394>low){low=d-605394;lowFace=c;}
  if(d+605394<high){high=d+605394;highFace=c;}
 }
 """
    if source.count(original) != 1:
        raise ValueError("T909_ANGLE_SOURCE")
    source = source.replace(original, replacement)
    original = "if(first>last){qfail(14u);return;}"
    if source.count(original) != 1:
        raise ValueError("T909_ANGLE_RANGE")
    replacement = """uint mark=uReplayFrozen+256u;
 W[mark]=uNonce.x;W[mark+1u]=uNonce.y;W[mark+2u]=uint(first-7680);W[mark+3u]=uint(last-7680);
 W[mark+4u]=lowFace;W[mark+5u]=highFace;
 W[mark+6u]=lowFace<127u?col(10u,lowFace):0u;W[mark+7u]=lowFace<127u?col(10u,lowFace+1u):0u;
 W[mark+8u]=highFace<127u?col(10u,highFace):0u;W[mark+9u]=highFace<127u?col(10u,highFace+1u):0u;
 W[mark+10u]=uint(low);W[mark+11u]=uint(high);
 """
    return source.replace(original, replacement + original)


def build(parent, compiler, out, witness=False):
    source = parent.read_bytes()
    if sha(source) != PARENT:
        raise ValueError("T909_PARENT_SOURCE_SHA")
    original = source.decode("utf-8")
    start = original.index("bool inventory(){")
    end = original.index("void prepare(){", start)
    candidate = (angle_witness(original) if witness else
                 original[:start] + Path(__file__).with_name("m1_inventory_v1.glsl").read_text() + original[end:])
    if len(candidate.encode()) > 49152:
        raise ValueError("T909_SOURCE_LIMIT")
    out.mkdir(parents=True, exist_ok=True)
    shader = out / ("quantum_candidate_angle_witness_v1.comp" if witness else "quantum_candidate_inventory_v1.comp")
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
    parser.add_argument("--angle-witness", action="store_true")
    args = parser.parse_args()
    build(args.parent, args.compiler, args.out, args.angle_witness)
