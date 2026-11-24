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


def compact_inventory():
    directory = Path(__file__).parent
    inventory = (directory / "m1_inventory_v1.glsl").read_text()
    return once(inventory, "if(malformed||s>CQ||w>CQ-s||",
                "if(malformed||s>CQ||w>CQ-s||((c<compactFirst||c>=compactEnd)&&s+w+j+p!=0u)||")


def fast_candidate(root):
    directory = Path(__file__).parent
    start, end = root.index("bool inventory(){"), root.index("void prepare(){")
    root = root[:start] + compact_inventory() + root[end:]
    group_sort = (directory / "m1_group_sort.glsl").read_text()
    root = once(root, "void candidateMain(){", group_sort + "\nvoid candidateMain(){")
    for init, stage, offset, count, ns, arrival in (
        (6, 7, "uSandIndex", "uPad", "0u", "false"),
        (8, 9, "uWaterIndex", "uKw", "1u", "false"),
        (16, 17, "uArrivalIndex", "uKw", "1u", "true"),
    ):
        original = (f"if(uPass=={init}u){{if(i<{count})W[{offset}+i]=i;return;}}"
                    f"if(uPass=={stage}u){{sortIndex(i,{offset},{count},{ns},{arrival});return;}}")
        root = once(root, original, f"if(uPass=={stage}u){{groupSort({offset},{count},{ns},{arrival});return;}}")
    return root


def integrated_arithmetic(root):
    start = root.index("bool grsDivide128(")
    end = root.index("\n}", start) + 2
    return root[:start] + Path(__file__).with_name("m1_fast_divide.glsl").read_text() + root[end:]


def integrated_audit(root, breakdown):
    inventory = compact_inventory()
    inventory = once(inventory,
        "for(uint next=n+1u;next<live[ns];++next)if(C[at(ns,5u,liveSlot[start+next])]==id){qfail(15u);return false;}",
        "auditKey[n]=id;")
    inventory = once(inventory, "auditKey[n]=id;\n  }\n }",
                     "auditKey[n]=id;\n  }\n  if(!uniqueAuditKeys(live[ns])){qfail(15u);return false;}\n }")
    start = root.index("bool inventory(){")
    end = root.index("\n}", start) + 2
    root = root[:start] + Path(__file__).with_name("m1_audit_keys.glsl").read_text() + inventory + root[end:]
    start = root.index("int first=15361,last=-1;", root.index("bool quantumAudit(){"))
    end = root.index("if(!requireQ(first<=last", start)
    bound = """int first=0,end=15361;
 while(first<end){int mid=first+(end-first)/2;if(int(W[uAngles+4u*uint(mid)])<low)first=mid+1;else end=mid;}
 int last=first;end=15361;
 while(last<end){int mid=last+(end-last)/2;if(int(W[uAngles+4u*uint(mid)])<=high)last=mid+1;else end=mid;}
 --last;"""
    root = root[:start] + bound + root[end:]
    if breakdown:
        root = once(root, "bool quantumAudit(){\n if(!inventory())return false;",
                    "bool auditEntryAngle(){")
        root = once(root, " if(!requireQ(C[uHeader+14u]==C[uHeader+34u]",
                    " return true;\n}\nbool auditSource(){\n if(!requireQ(C[uHeader+14u]==C[uHeader+34u]")
        root = once(root, " if(!requireQ(C[uHeader+122u]==1u",
                    " return true;\n}\nbool auditState(){\n if(!requireQ(C[uHeader+122u]==1u")
        root = once(root, "if(i==0u){W[19]=0xffffffffu;quantumAudit();}",
                    "if(i==0u)auditState();")
        root = once(root, " if(uPass==21u)", """ if(uPass==29u){if(i==0u){W[19]=0xffffffffu;inventory();}return;}
 if(uPass==30u){if(i==0u)auditEntryAngle();return;}
 if(uPass==31u){if(i==0u)auditSource();return;}
 if(uPass==21u)""")
        root = once(root, "W[2]=uPass;", "W[2]=uPass>=29u?21u:uPass;")
    return root


def build(assembled, compiler, out, optimized=False, integrated=False, breakdown=False):
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
    units = list(quantum_units(root.encode()))
    if optimized or integrated:
        candidate = next(quantum_units(fast_candidate(root).encode()))
        if candidate[0] != "quantum_candidate" or units[0][0] != candidate[0]:
            raise ValueError("COMPACT_CANDIDATE_ROLE")
        units[0] = candidate
    if integrated:
        name, passes, raw = units[0]
        units[0] = name, passes, integrated_arithmetic(raw.decode()).encode()
        name, passes, raw = units[1]
        units[1] = name, passes + ([29, 30, 31] if breakdown else []), integrated_audit(
            integrated_arithmetic(raw.decode()), breakdown).encode()
    for name, passes, raw in units:
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
    variant = parser.add_mutually_exclusive_group()
    variant.add_argument("--fast-candidate", action="store_true")
    variant.add_argument("--integrated", action="store_true")
    variant.add_argument("--audit-breakdown", action="store_true")
    args = parser.parse_args()
    build(args.assembled, args.compiler, args.out, args.fast_candidate,
          args.integrated or args.audit_breakdown, args.audit_breakdown)
