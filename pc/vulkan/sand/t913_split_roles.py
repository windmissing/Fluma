"""Versioned PC SOURCE dispatch continuations, with the parent event opcode."""
import re


REPLAY_STAGES = ("BASE_COPY", "TRANSACTION", "AUDIT")
PLAN_STAGES = ("MAP_COPY", "RESOLVE_AUDIT", "MARKERS", "REFILL")


def main_body(source):
    start = source.index("void main(){")
    match = re.match(r"void main\(\)\{\s*if\(uOp==(\d+)u\)\{", source[start:])
    if not match:
        raise ValueError("T913 continuation main")
    return start, int(match[1])


def continued(source, opcode, original, statements, *, cleanup=False):
    base = source[:source.index("void main(){")]
    if base.count(" uint uOp;") != 1:
        raise ValueError("T913 physical opcode push field")
    base = base.replace(" uint uOp;", " uint t913PhysicalOp;", 1)
    # Only the Vulkan dispatch chooses the continuation. Event and audit
    # writers still see the canonical parent opcode via the shared helpers.
    first = base.index("};", base.index("uniform Dispatch"))+2
    base = base[:first] + f"\nconst uint uOp={original}u;\n" + base[first:]
    precheck = """
 uint events=0u,ready=0u;
 if(lane==0u){events=W[uStageDiagnostic];ready=W[uStageDiagnostic+1u];
             W[uStageDiagnostic]=0u;W[uStageDiagnostic+1u]=0u;}
""" if cleanup else ""
    return (base + f"""
void main(){{
 if(t913PhysicalOp!={opcode}u) return;
 uint lane=gl_LocalInvocationIndex;
 if(lane==0u){{
  if(g9Ctl(16u)!=uPass)g9Fault(25u,0u,uPass);
  if(uRootKind!=1u)g9Fault(25u,0u,uPass);
  g9Set(16u,g9Ctl(16u)+1u);
 }}
 memoryBarrierBuffer();barrier();
 uJ=(uRole==2u||uRole==4u)?g9Ctl(11u):uJslot;
 {precheck}
 if(g9Ctl(0u)!=0u)return;
 {statements}
}}
""").encode()


def replay(source):
    source = source.decode()
    start, original = main_body(source)
    body = source[start:]
    lo = body.index("g9Set(6u,0u);")
    hi = body.index("if(g9Ctl(7u)!=events", lo)
    transaction = body[lo:hi]
    if "g9EndAcceptedMetadata(outcome);" not in transaction:
        raise ValueError("T913 replay transaction boundary")
    audit = ("if(lane==0u&&ready==1u&&(g9Ctl(7u)!=events||!g9AuditMassGeometry()||"
             "!g9AuditMomentum()||!g9AuditStream()))g9Fault(25u,0u,0u);")
    parts = [
        "g9CopyBase(); if(lane==0u){W[uStageDiagnostic]=g9Ctl(7u);W[uStageDiagnostic+1u]=0u;}",
        "if(lane==0u){" + transaction + "W[uStageDiagnostic+1u]=1u;}",
        audit,
    ]
    return [(0x380+i, original, "T913_REPLAY_"+name,
             continued(source, 0x380+i, original, instructions, cleanup=i==2))
            for i, (name, instructions) in enumerate(zip(REPLAY_STAGES, parts, strict=True))]


def plan(source):
    source = source.decode()
    _, original = main_body(source)
    lo = source.index("bool visPlanApply(){") + len("bool visPlanApply(){")
    hi = source.index("memoryBarrierBuffer();barrier();return g9Ctl(0u)==0u;", lo)
    body = source[lo:hi]
    first = body.index("memoryBarrierBuffer();barrier();")
    marker = body.index("uint bank=uRole==4u?2u:1u;")
    refill = body.index("if(gl_LocalInvocationIndex==0u&&g9Ctl(0u)==0u)", marker)
    parts = [
        body[:first],
        "bool sealed=g11ResolveAndSeal(); if(lane==0u&&sealed){if(!g11AuditTransfers()){}}",
        body[marker:body.index("memoryBarrierBuffer();barrier();", marker)],
        "uint bank=uRole==4u?2u:1u;"+body[refill:],
    ]
    return [(0x390+i, original, "T913_VIS_"+name, continued(source, 0x390+i, original, instructions))
            for i, (name, instructions) in enumerate(zip(PLAN_STAGES, parts, strict=True))]


def split_source(family, jobs, passes):
    spliced = {}
    for opcode, src, name in jobs:
        if name == "G9_OP_SOURCE_AUDIT_REPLAY":
            spliced[opcode] = replay(src)
        elif family == "r1" and name == "G11_OP_VIS_PLAN_APPLY":
            spliced[opcode] = plan(src)
    emitted = {}
    expanded = []
    for row in passes["Source"]:
        parts = spliced.get(row[0])
        if parts is None:
            expanded.append(row)
            continue
        for opcode, binding, name, src in parts:
            emitted[opcode] = (binding,name,src)
            expanded.append([opcode,*row[1:]])
    assert len(expanded)==(27 if family=="r1" else 14)
    passes["Source"] = expanded
    additions = [(op, src, name) for op, (_,name,src) in sorted(emitted.items())]
    return jobs + additions, {op: fields[0] for op, fields in emitted.items()}
