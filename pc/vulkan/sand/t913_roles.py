"""SOURCE-only algorithm substitutions on the isolated PC role build."""
def replace(text, old, new):
    if text.count(old) != 1:
        raise ValueError("T913 shader anchor: " + old)
    return text.replace(old, new, 1)


def compare(text):
    body = "bool g9CompareState(){for(uint i=0u;i<uStateWords;++i)if(B[i]!=C[i]){g9Fault(25u,i,0u);return false;}return true;}"
    text = replace(text, body, """shared uint t913First[64];
shared bool t913Enter;
void g9CompareState(){
 uint lane=gl_LocalInvocationIndex,first=uStateWords;
 for(uint i=lane;i<uStateWords;i+=64u)if(B[i]!=C[i]){first=i;break;}
 t913First[lane]=first;barrier();
 if(lane==0u){
  for(uint i=1u;i<64u;++i)first=min(first,t913First[i]);
  if(first<uStateWords)g9Fault(25u,first,0u);
 }
}""")
    guard = "if(lane!=0u)return;\nif(!dmStageEnter())return;"
    if guard in text:
        return replace(text, guard,
                       "if(lane==0u)t913Enter=dmStageEnter();memoryBarrierBuffer();barrier();if(!t913Enter)return;")
    return replace(text, "if(lane!=0u)return;\n", "")


def columns(text):
    anchor = "uvec4 total=uvec4(0u),declared=uvec4(0u),nextCommit;"
    text = replace(text, anchor, """uint t913Packets[128];
 for(uint i=0u;i<128u;++i)t913Packets[i]=0u;
 for(uint slot=0u;slot<uK;++slot){
  uint mass=g9Packet(slot,0u),column=g9Packet(slot,5u);
  if(mass==0u||column>=128u)continue;
  uint result;
  t913Packets[column]=g9AddMass(t913Packets[column],mass,result)?result:0xffffffffu;
 }
 """ + anchor)
    return replace(text, "uint packets=0u;for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u&&g9Packet(slot,5u)==c){uint next;if(!g9AddMass(packets,g9Packet(slot,0u),next))return false;packets=next;}",
                   "uint packets=t913Packets[c];if(packets==0xffffffffu)return false;")


def sparse_plan(text):
    scan = "uint g11FindMap(uint table,uint c,uvec3 key){for(uint r=0u;r<64u;++r)if(g11MapMass(table,c,r)>0u&&all(equal(key,g11MapKey(table,c,r))))return r;return 64u;}"
    text = replace(text, scan, """shared uint t913PlanBlocks[2048];
shared uint t913MapRows[1024];
shared bool t913MapReady;
uint g11FindMap(uint table,uint c,uvec3 key){
 if(uRootKind==1u){
  for(uint part=0u;part<2u;++part){
   uint mask=t913MapRows[((table-1u)*128u+c)*2u+part];
   while(mask!=0u){
    uint r=part*32u+uint(findLSB(mask));mask&=mask-1u;
    if(all(equal(key,g11MapKey(table,c,r))))return r;
   }
  }
 }else for(uint r=0u;r<64u;++r)if(g11MapMass(table,c,r)>0u&&all(equal(key,g11MapKey(table,c,r))))return r;
 return 64u;
}""")
    header = "bool g11ResolveAndSeal(){uint count=0u,phase=g11Header(12u);"
    start = text.index(header) + len(header)
    end = text.index("for(uint slot=0u;slot<65536u;++slot){", start)
    original = text[start:end]
    checks = original[:original.index("if(!g11BuildMapRefs())")]
    condition = checks.split("return g11Fault", 1)[0].strip()
    assert condition.startswith("if(") and condition.endswith(")")
    setup = """bool sparse=uRootKind==1u;
 if(sparse){
  uint lane=gl_LocalInvocationIndex;
  for(uint i=lane;i<1024u;i+=64u){
   uint table=i/256u+1u,c=(i/2u)%128u,base=(i%2u)*32u,mask=0u;
   for(uint j=0u;j<32u;++j)if(g11MapMass(table,c,base+j)>0u)mask|=1u<<j;
   t913MapRows[i]=mask;
  }
  barrier();
  if(lane==0u){
   CONDITION t913MapReady=g11Fault(26u,uRole,44u);
   else t913MapReady=g11BuildMapRefs();
  }
  memoryBarrierBuffer();barrier();
  if(!t913MapReady)return false;
  for(uint block=lane;block<2048u;block+=64u){
   uint bits=0u;
   for(uint j=0u;j<32u;++j){
    uint a=g11PlanAt(block*32u+j),nonzero=0u;
    for(uint f=0u;f<8u;++f)nonzero|=W[a+f];
    if(nonzero!=0u)bits|=1u<<j;
   }
   t913PlanBlocks[block]=bits;
  }
  barrier();
  if(lane!=0u)return true;
 }else{
 BASE_CHECKS
 }
 """.replace("CONDITION", condition).replace("BASE_CHECKS", original)
    text = text[:start] + setup + text[end:]
    text = replace(text, "for(uint slot=0u;slot<65536u;++slot){uint a=g11PlanAt(slot),type=W[a];",
                   """for(uint cursor=0u;cursor<65536u;){
  if(sparse){
   uint block=cursor/32u,mask=t913PlanBlocks[block]&(0xffffffffu<<(cursor%32u));
   if(mask==0u){cursor=(block+1u)*32u;continue;}
   cursor=block*32u+uint(findLSB(mask));
  }
  uint slot=cursor++,a=g11PlanAt(slot),type=W[a];""")
    return replace(text, "if(gl_LocalInvocationIndex==0u){if(!g11ResolveAndSeal()||!g11AuditTransfers()){} }",
                   """if(uRootKind==1u){
  bool plan=g11ResolveAndSeal();
  if(gl_LocalInvocationIndex==0u&&plan){if(!g11AuditTransfers()){}}
 }else if(gl_LocalInvocationIndex==0u){if(!g11ResolveAndSeal()||!g11AuditTransfers()){} }""")


def variant(name, source, family):
    text = source.decode()
    if name == "G9_OP_SOURCE_COMPARE":
        return compare(text).encode()
    if family == "dm2" and name in ("G9_OP_SOURCE_VALIDATE_INPUT", "G9_OP_SOURCE_ORIGIN_CERTIFICATE",
                                     "G9_OP_SOURCE_AUDIT_REPLAY", "G9_OP_SOURCE_PACK_SCENE"):
        return columns(text).encode()
    if name == "G11_OP_VIS_PLAN_APPLY":
        return sparse_plan(text).encode()
    return source
