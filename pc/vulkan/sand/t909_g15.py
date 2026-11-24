"""Install the T907 dry operators in the existing DM2 Actual/Replay plan."""

from pathlib import Path


def once(text, old, updated):
    if text.count(old) != 1:
        raise ValueError(f"G15 parent anchor mismatch: {old}")
    return text.replace(old, updated)


def install(source):
    helper = Path(__file__).with_name("g15_dm2.glsl").read_text(encoding="utf8")
    geometry = source.index("bool dmGeometry(")
    tail = source.index("bool dmOriginPrefix(",geometry)
    source = source[:geometry] + helper + "\n" + source[tail:]
    source = source.replace("dmGeometry(","g15Geometry(")
    source = once(source,"uint left=edge==0u?0u:edge-1u,right=edge==128u?127u:edge,z=max(g9Column(left,0u),g9Column(right,0u));",
                  "uint left=edge==0u?0u:edge-1u,right=edge==128u?127u:edge;")
    source = once(source,"if(!g9FlowTerms(left,z,fl,pl,ul)||!g9FlowTerms(right,z,fr,pr,ur))",
                  "if(!g15Hydro(edge,left,right,fl,fr,pl,pr,ul,ur))")
    source = once(source,"!g9SMul(g9S32(int(C[22u])),g9U32(g9Column(c,1u)),product)||!g9SDiv(product,g9U32(120u*uJ),1u,gravity)||",
                  "!g9SSub(p,p,gravity)||")
    source = once(source,"W[stage+14u]=g9Column(c,4u)==0u",
                  "W[stage+18u]=g15Normal(c);if(W[stage+18u]==0u)return false;W[stage+14u]=g9Column(c,4u)==0u")
    source = once(source,"if(W[uColumnStage+(edge-1u)*64u+14u]!=0u&&W[uColumnStage+edge*64u+14u]!=0u)W[uFaces+edge*32u]=0u;\n return true;",
                  "if(W[uColumnStage+(edge-1u)*64u+14u]!=0u&&W[uColumnStage+edge*64u+14u]!=0u&&g15StaticFace(edge-1u))W[uFaces+edge*32u]=0u;\n"
                  " if(uOp==G9_OP_CORRECTOR_FACE)g15ClipDonors();\n return true;")
    source = once(source,"if(!g9SMul(g9U32(dmReadCandidateWord(23u)),g9U32(flow),product)",
                  "if(!g9SMul(g9U32(W[uColumnStage+c*64u+18u]),g9U32(flow),product)")
    source = once(source,"g9Column(c,13u)==0u&&g9Zero(chi);",
                  "g9Column(c,13u)==0u&&g9Zero(chi)&&g15QuietColumn(c);")
    source = source.replace("60u*uJ*1048576u","54u*uJ*1048576u")
    begin = source.index("bool g9GravityWork(){")
    end = source.index("bool g9ConfigValid(){",begin)
    gravity = source[begin:end]
    gravity = once(gravity,"bool g9GravityWork(){\n int oldGx=int(B[22u]);uint oldGn=B[23u];uvec4 gxWide,gnWide;",
                   "bool g15GravityWork(uint tick){\n int oldAngle=int(C[20u]),oldGx=int(C[22u]);uint oldGn=C[23u];uvec4 gxWide,gnWide;\n"
                   " int angle,sine,cosine;if(!g15Pose(tick,angle,sine,cosine)){g9Fault(22u,0u,tick);return false;}")
    gravity = once(gravity,"if(uAcceptedAngle< -7680||uAcceptedAngle>7680||uAngleCos<=0||uAngleCos>1048576||uAngleSin< -1048576||uAngleSin>1048576||",
                   "if(")
    gravity = gravity.replace("uAngleSin","sine").replace("uAngleCos","cosine").replace("uAcceptedAngle","angle")
    gravity = once(gravity,"row[15]=B[20u];","row[15]=uint(oldAngle);")
    source = source[:begin] + gravity + source[end:]
    source = once(source,"if(g9Ctl(14u)==0u)g9GravityWork();","if(g9Ctl(14u)==0u)g15GravityWork(g15Tick(0u,false));")
    source = once(source,"if(g9Cfl()&&g9SnapshotMicro()&&!dmRequest()",
                  "if(g15GravityWork(g15Tick(uMicro,true))&&g15SupportAll()&&g9Cfl()&&g9SnapshotMicro()&&!dmRequest()")
    source = once(source,"if(!g9Fold()||!g9Cfl()||!g9AuditMassGeometry()||!g9AuditMomentum())",
                  "if(!g9Fold()||!g15SupportAll()||!g9Cfl()||!g9AuditMassGeometry()||!g9AuditMomentum())")
    source = once(source,"if(!g9AuditMicroEnergy())return;",
                  "if(!g9AuditMicroEnergy())return;\n"
                  "  if(!g15GravityWork(g15Tick(uMicro+1u,false))||!g15SupportAll())return;\n"
                  "  if(uMicro+1u==uJ)g15Scene();")
    return source


def enable_int64(source):
    return source.replace(b"#version 450\n",
                          b"#version 450\n#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n",1)
