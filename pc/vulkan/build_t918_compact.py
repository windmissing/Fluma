"""Compile the isolated T918 M1 SOURCE, candidate and independent replay."""

import argparse
import json
from pathlib import Path
import re

from check_m1_spirv import compile_stage, quantum_units


def once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"Expected exactly one compact profile patch target: {old}")
    return text.replace(old, new)


def build(assembled, compiler, out):
    header = Path(__file__).with_name("m1_compact.h").read_text()
    lo, hi, height = (int(re.search(rf"\b{name}=(\d+);", header)[1]) for name in
                      ("kCompactFirstColumn", "kCompactEndColumn", "kCompactHeight"))
    if not 0 < lo < hi < 128 or not 0 < height < 256:
        raise ValueError("COMPACT_DOMAIN")
    out.mkdir(parents=True, exist_ok=True)
    limits = f"const uint compactFirst={lo}u,compactEnd={hi}u;\n"
    source = (assembled / "full_source.comp").read_text()
    source = once(source, "const uint CQ=268435456u", limits + f"const uint CQ={height << 20}u")
    source = once(source, "uint col=x>>20u,s=", "uint col=x>>20u;if(col<compactFirst||col>=compactEnd){reject(2u);return;}uint s=")
    source = once(source, "if(p>CQ-s-w-j||", "if(((c<compactFirst||c>=compactEnd)&&s+w+j+p!=0u)||p>CQ-s-w-j||")
    root = (assembled / "commit.comp").read_text()
    root = once(root, "const uint Q=1048576u,CQ=268435456u", limits + f"const uint Q=1048576u,CQ={height << 20}u")
    for col in ("c", "from"):
        root = once(root, f"(dx<0&&{col}==0u)||(dx>0&&{col}==127u)",
                    f"(dx<0&&{col}==compactFirst)||(dx>0&&{col}+1u==compactEnd)")
    if root.count("nx=dx<0?0:int(128u*Q-1u)") != 2:
        raise ValueError("COMPACT_WALL_GEOMETRY")
    root = root.replace("nx=dx<0?0:int(128u*Q-1u)", "nx=dx<0?int(compactFirst*Q):int(compactEnd*Q-1u)")
    root = once(root, "if(p>CQ-s-w||", "if(((c<compactFirst||c>=compactEnd)&&s+w+j+p!=0u)||p>CQ-s-w||")
    for where in ("uint c=edge,r=edge+1u;int ha", "int delta=int(col(10u,edge+1u))"):
        root = once(root, where, "if(edge<compactFirst||edge+1u>=compactEnd)return;" + where)
    # A wall has no neighboring stock. Keep the original slope bound on every
    # internal interface in both candidate and replay, including at the ends.
    for old, new in (
        ("for(uint c=0u;c<127u;c++){int d=int(col", "for(uint c=compactFirst;c+1u<compactEnd;c++){int d=int(col"),
        ("for(uint c=0u;c<127u;c++){int delta=int(B", "for(uint c=compactFirst;c+1u<compactEnd;c++){int delta=int(B"),
        ("int d=c<127u?int(col", "int d=c>=compactFirst&&c+1u<compactEnd?int(col"),
        ("if(c<127u&&!requireQ(abs(int(col", "if(c>=compactFirst&&c+1u<compactEnd&&!requireQ(abs(int(col"),
    ):
        root = once(root, old, new)
    old_guard = "if(e<0||e>=127)continue;"
    if root.count(old_guard) != 2:
        raise ValueError("COMPACT_SLOPE_GUARD")
    root = root.replace(old_guard, "if(e<int(compactFirst)||e+1>=int(compactEnd))continue;")
    root = once(root, "&&from<128u&&to<128u&&ev(row,0u)", "&&from>=compactFirst&&from<compactEnd&&"
                "to>=compactFirst&&to<compactEnd&&ev(row,0u)")
    roles = {"full_source": compile_stage(compiler, Path("full_source.comp"), out, raw=source.encode(), bounded=True)}
    for name, passes, raw in quantum_units(root.encode()):
        roles[name] = dict(compile_stage(compiler, Path(name + ".comp"), out, raw=raw, bounded=True), passes=passes)
    if roles["quantum_candidate"]["offsets"] != roles["quantum_commit"]["offsets"]:
        raise ValueError("COMPACT_UNIFORM_LAYOUT")
    (out / "m1_static.json").write_text(json.dumps(roles, indent=2) + "\n", encoding="utf-8")
    for name, role in roles.items():
        print(name, role["spvSHA"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assembled", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    build(args.assembled, args.compiler, args.out)
