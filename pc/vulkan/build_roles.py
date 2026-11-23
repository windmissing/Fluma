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


def t849_arms(emit):
    def cut_drive(name, text, motion):
        if name != "drive" or text == "void drive();":
            return text
        cut = "uvec4 fq=uvec4(baseState[18],baseState[19],0u,0u);"
        if text.count(cut) != 1 or not text.startswith("void drive(){if(lane!=0u)return;"):
            raise ValueError("T849 drive boundary changed")
        front, tail = text.split(cut)
        return front+"}" if motion else "void drive(){if(lane!=0u)return;"+cut+tail

    def op7(op, arm):
        return "if(uOp==7u){lane=gl_LocalInvocationID.x;drive();return;}" if op == 7 else arm

    for name, motion in (("OP7_MOTION_ONLY", True), ("OP7_MMS_FORCE_ONLY", False)):
        raw, deps = emit((7,), op7, lambda name, text: cut_drive(name, text, motion))
        assert ("softReduce" in deps) == motion and ("softExp" in deps) != motion
        yield name, [7,66], raw
    for name, statement in (("OP17_LEDGER_ONLY", "drive();\n"), ("OP17_DRIVE_CONTEXT", "\nentryLedger();")):
        raw, deps = emit((17,), lambda op, arm: substitute(arm, statement, "", name) if op == 17 else arm)
        assert "traceStage" in deps and ("drive" in deps) != ("entryLedger" in deps)
        yield name, [17,66], raw
    for name, motion in (("OP17_MOTION_CONTEXT", True), ("OP17_MMS_CONTEXT", False)):
        raw, deps = emit(
            (17,),
            lambda op, arm: substitute(arm, "\nentryLedger();", "", name) if op == 17 else arm,
            lambda fn, body: cut_drive(fn, body, motion),
        )
        assert {"drive", "traceStage", "acceptedTime", "coldFP32Probe"} <= set(deps)
        assert "entryLedger" not in deps
        assert ("softReduce" in deps) == motion and ("softExp" in deps) != motion
        yield name, [17,66], raw
    reduce = ("if(uOp==7u){lane=gl_LocalInvocationID.x;if(lane==0u){uint quadrant;"
              "Soft reduced=softReduce(softF32(uintBitsToFloat(outState[26])),quadrant);"
              "uvec4 raw=mathRaw(reduced);c[624]=quadrant;for(uint i=0u;i<4u;++i)c[625u+i]=raw[i];}return;}")
    raw, deps = emit((7,), lambda op, arm: reduce if op == 7 else arm)
    assert {"softReduce","softF32","mathRaw"} <= set(deps)
    assert not {"softPolynomial","softDiv","softExp"} & set(deps)
    yield "OP7_REDUCE_ONLY", [7,66], raw


OP17_PROTOCOL = """
const uint t854Tag=0x54383534u;
bool t854Range(){return uPrivateWord>=uL[5]&&uPrivateWord%4u==0u&&
                                uPrivateWord<=w.length()&&w.length()-uPrivateWord>=32u;}
bool t854Previous(uint phase){
 if(!t854Range())return false;
 uint p=uPrivateWord;
 return w[p]==t854Tag&&w[p+1u]==phase&&w[p+2u]==c[136]&&w[p+3u]==c[137]&&
        w[p+4u]==uOp&&w[p+5u]==uScope&&w[p+6u]==uJ&&w[p+7u]==uMicro&&w[p+8u]==uBlock&&
        w[p+9u]==uIndex&&w[p+10u]<=1u&&w[p+11u]==0u&&w[p+30u]==0u&&w[p+31u]==0u;
}
void t854Stage(uint phase){memoryBarrierBuffer();w[uPrivateWord+1u]=phase;}
void t854MathSave(){
 uint p=uPrivateWord;
 w[p+12u]=uint(softDomain);w[p+13u]=localMathStatus;w[p+14u]=localMathFunction;
 w[p+15u]=localMathObserved;w[p+16u]=localRounded;w[p+17u]=uint(expContext);
 for(uint i=0u;i<4u;++i){w[p+18u+i]=localArg0[i];w[p+22u+i]=localArg1[i];w[p+26u+i]=localSoftResult[i];}
}
void t854MathRestore(){
 uint p=uPrivateWord;
 softDomain=w[p+12u]!=0u;localMathStatus=w[p+13u];localMathFunction=w[p+14u];
 localMathObserved=w[p+15u];localRounded=w[p+16u];expContext=w[p+17u]!=0u;
 for(uint i=0u;i<4u;++i){localArg0[i]=w[p+18u+i];localArg1[i]=w[p+22u+i];localSoftResult[i]=w[p+26u+i];}
}
void t854MotionLatch(bool isActive){
 if(lane!=0u)return;
 uint p=uPrivateWord;
 w[p]=t854Tag;w[p+2u]=c[136];w[p+3u]=c[137];
 w[p+4u]=uOp;w[p+5u]=uScope;w[p+6u]=uJ;w[p+7u]=uMicro;w[p+8u]=uBlock;w[p+9u]=uIndex;
 w[p+10u]=uint(isActive);w[p+11u]=0u;w[p+30u]=0u;w[p+31u]=0u;
 t854MathSave();t854Stage(1u);
}
"""


def t854_private_arms(emit, slot_ordinals):
    def drive_part(name, body, motion):
        if name != "drive" or body == "void drive();":
            return body
        cut = "uvec4 fq=uvec4(baseState[18],baseState[19],0u,0u);"
        front, tail = body.split(cut)
        return front+"}" if motion else "void drive(){if(lane!=0u)return;"+cut+tail

    def motion(arm):
        arm = substitute(arm, "lane=gl_LocalInvocationID.x;", "lane=gl_LocalInvocationID.x;\n"
                         "if(!t854Range()||(c[136]|c[137])==0u)return;\n"
                         "if(lane==0u)w[uPrivateWord+1u]=0u;", "T854 private motion")
        arm = substitute(arm, "if(noop){traceStage(true);",
                         "if(noop){traceStage(true);t854MotionLatch(false);", "T854 private motion")
        return substitute(arm, "entryLedger();}}}\ntraceStage(false);",
                          "}}}\nt854MotionLatch(true);", "T854 private motion")

    mms = """if(uOp==17u){lane=gl_LocalInvocationID.x;if(lane!=0u||!t854Previous(1u))return;
 if(w[uPrivateWord+10u]==1u){t854MathRestore();drive();t854MathSave();}t854Stage(2u);return;}"""
    ledger = """if(uOp==17u){lane=gl_LocalInvocationID.x;if(!t854Previous(2u))return;
 if(w[uPrivateWord+10u]==1u){entryLedger();traceStage(false);}
 if(lane==0u)t854Stage(3u);return;}"""
    cases = "".join(f"case {ordinal}u:slot={slot}u;break;" for slot, ordinal in enumerate(slot_ordinals))
    snapshot_protocol = f"""
void t899Snapshot(){{
 if(lane!=0u||!t854Range()||w.length()-uPrivateWord<32u+{len(slot_ordinals)}u*42u)return;
 uint slot;
 switch(uIndex){{{cases}default:return;}}
 uint dst=uPrivateWord+32u+slot*42u;
 for(uint i=0u;i<32u;++i)w[dst+i]=w[uPrivateWord+i];
 for(uint i=0u;i<10u;++i)w[dst+32u+i]=c[513u+i];
}}
"""
    def snapshot_ledger(_):
        arm = substitute(ledger, "if(!t854Previous(2u))return;",
                         "if(!t854Previous(2u)){t899Snapshot();return;}", "T899 rejected slot")
        return substitute(arm, "if(lane==0u)t854Stage(3u);return;",
                          "if(lane==0u){t854Stage(3u);memoryBarrierBuffer();t899Snapshot();}return;",
                          "T899 completed slot")

    for name, arm, rewrite in (
            ("OP17_PRIVATE_MOTION", motion, lambda fn, body: drive_part(fn, body, True)),
            ("OP17_PRIVATE_MMS", lambda _: mms, lambda fn, body: drive_part(fn, body, False)),
            ("OP17_PRIVATE_LEDGER", lambda _: ledger, None),
            ("OP17_PRIVATE_LEDGER_SNAPSHOT", snapshot_ledger, None)):
        raw, _ = emit((17,), lambda op, old: arm(old) if op == 17 else old, rewrite)
        protocol = OP17_PROTOCOL + (snapshot_protocol if arm is snapshot_ledger else "")
        src = substitute(raw.decode(), "void main(){", protocol+"\nvoid main(){", name)
        yield name, [17,66], src.encode()


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
    sources.extend(t849_arms(emit))
    dispatch = Path(__file__).resolve().parents[2] / "docs" / "architecture-T607-g13-dispatch-v1.json"
    plan = json.loads(dispatch.read_bytes())
    slot_ordinals = tuple(i for i, row in enumerate(plan["quantum"]) if row["opcode"] == 17)
    assert len(slot_ordinals) == 14 and all(row["opcode"] != 17 for row in plan["source"])
    header.append("inline constexpr size_t kT899SlotOrdinals[] = {"+
                  ",".join(map(str, slot_ordinals))+"};")
    sources.extend(t854_private_arms(emit, slot_ordinals))
    for name, opcodes, raw in sources:
        src = raw.decode()
        for a, b in (("#version 310 es", DECLARATION),
                     ("uniform uint uOp,uScope,uJ,uMicro,uBlock,uIndex;",
                      "layout(push_constant) uniform Dispatch {uint uOp,uScope,uJ,uMicro,uBlock,uIndex;};"),
                     ("uniform uint uL[80];",
                      ("layout(std140,set=0,binding=4) uniform LayoutWords {uint uL[80];"+
                       ("uint uPrivateWord;" if name.startswith("OP17_PRIVATE_") else "")+"};")),
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
        if name.startswith("OP17_PRIVATE_"):
            assert "OpMemberDecorate %LayoutWords 1 Offset 1280" in asm
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
