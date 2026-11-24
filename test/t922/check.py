"""Independent role package and sealed SOURCE semantic comparison for T922."""

import argparse
import hashlib
import json
from pathlib import Path
import struct


def demand(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def canonical_roles(path):
    data = json.loads(path.read_bytes())
    digest = data.pop("roleSetSHA")
    demand(digest == sha(json.dumps(data, sort_keys=True, separators=(",", ":")).encode()), "ROLE_SET_DIGEST")
    data["roleSetSHA"] = digest
    return data


def pe_rows(data, modules):
    pe, = struct.unpack_from("<I", data, 60)
    demand(data[:2]==b"MZ" and data[pe:pe+4]==b"PE\0\0", "PE_MAGIC")
    machine, section_count = struct.unpack_from("<HH", data, pe+4)
    size, = struct.unpack_from("<H", data, pe+20)
    opt = pe+24
    demand(machine==0x8664 and struct.unpack_from("<H",data,opt)[0]==0x20b, "PE_AMD64")
    base, = struct.unpack_from("<Q",data,opt+24)
    sections = [struct.unpack_from("<III",data,opt+size+40*i+12) for i in range(section_count)]

    def offsets_to_va(offset):
        return [base+rva+offset-file for rva,length,file in sections if file<=offset<file+length]

    def string(address):
        for rva,length,file in sections:
            off = address-base-rva
            if 0<=off<length:
                start = file+off
                end = data.find(b"\0",start,min(start+128,file+length))
                if end>=0:
                    return data[start:end].decode("ascii")
        raise ValueError("PE_STRING_UNMAPPED")

    name = modules[0]["spv"].encode()+b"\0"
    strings = []
    pos = data.find(name)
    while pos>=0:
        strings.append(pos)
        pos=data.find(name,pos+1)
    tables = []
    for pos in strings:
        for va in offsets_to_va(pos):
            raw = struct.pack("<Q",va)
            where = data.find(raw)
            while where>=8:
                start = where-8
                try:
                    rows = []
                    for i,module in enumerate(modules):
                        op,n,filename,digest = struct.unpack_from("<IIQQ",data,start+24*i)
                        if op!=module["op"] or string(filename)!=module["spv"]:
                            break
                        rows.append((op,n,string(digest)))
                    if len(rows)==len(modules):
                        tables.append((start,rows))
                except (ValueError,struct.error):
                    pass
                where = data.find(raw,where+1)
    demand(len(tables)==1, f"PE_ROLE_TABLE_AMBIGUITY:{len(tables)}")
    return tables[0]


def package(exe, assets, variant):
    data = canonical_roles(assets/"roles.json")
    actual_variant = data.get("roleVariant","baseline")
    demand(variant==actual_variant,"WRONG_VARIANT")
    family = data["family"]
    demand(family in ("dm2","r1"),"WRONG_FAMILY")
    dispatch = (14 if family=="dm2" else 27) if variant=="t913-split-v2" else (12 if family=="dm2" else 19)
    demand(len(data["passes"]["Source"])==dispatch,"SOURCE_DISPATCH_COUNT")
    modules = [m for m in data["roles"] if m["state"]=="STATIC_SPIRV"]
    ops = [m["op"] for m in modules]
    demand(len(ops)==len(set(ops)) and all(row[0] in ops for rows in data["passes"].values() for row in rows),
           "ROLE_REFERENCES")
    image = exe.read_bytes()
    demand(data["roleSetSHA"].encode()+b"\0" in image,"PE_ROLE_SET_MISSING")
    offset, table = pe_rows(image,modules)
    issues = []
    for (op,size,digest),module in zip(table,modules,strict=True):
        raw = (assets/module["spv"]).read_bytes()
        if size!=len(raw) or digest!=sha(raw) or raw[:4]!=b"\x03\x02\x23\x07":
            issues.append(dict(opcode=op,reason="FILE_LENGTH" if size!=len(raw) else "FILE_SHA",
                               peBytes=size,fileBytes=len(raw),peSHA=digest,fileSHA=sha(raw)))
        demand(len(raw)==module["spvBytes"] and sha(raw)==module["spvSHA"] or bool(issues and issues[-1]["opcode"]==op),
               f"MANIFEST_MODULE:{op}")
    for group in data["graphics"].values():
        for m in group:
            raw=(assets/m["spv"]).read_bytes()
            demand(len(raw)==m["spvBytes"] and sha(raw)==m["spvSHA"] and raw[:4]==b"\x03\x02\x23\x07",
                   "GRAPHICS_MODULE")
    return dict(family=family,variant=variant,exeSHA=sha(image),tableOffset=offset,roles=len(table),
                sourceDispatches=dispatch,roleSetSHA=data["roleSetSHA"],issues=issues)


def words(raw):
    demand(len(raw)%4==0,"WORD_ALIGNMENT")
    return list(struct.unpack(f"<{len(raw)//4}I",raw))


def blob(values):
    return struct.pack(f"<{len(values)}I",*values)


def pair(values, at):
    return values[at]|values[at+1]<<32


def source_digest(values):
    digest = [2166136261^i for i in range(4)]
    for byte in blob(values[40:128]):
        digest = [((n^byte)*16777619)&0xffffffff for n in digest]
    return digest


def zero_pairs(values, offsets):
    for at in offsets:
        values[at:at+2]=[0,0]


# The two parent input/state layouts have identical offsets for these fields,
# but different magic, profile, typed Completion, and composite extensions.
PROFILES = {
    "dm2": dict(input=(843662407,843924551), tail=(843269191,2,3235776802,843924551), mask=65535,
                event=(843400263,3235776802)),
    "r1": dict(input=(826882375,826691667), tail=(1127297351,2,3235800177,1378955591), mask=851967,
               event=(826620231,3235776769)),
}
SCOPE = ("run","owner","context","inputEpoch","pageSession","permission","window","config","surface")
PLAN = ("nonce","plan","frame","control","geometry","deadlineNs","rootBeginNs","baseCommit","quanta","arenaGeneration")
MAPPED_INPUT = (4,6,8,10,12,14,16,18,20,43)
MAPPED_STATE = (4,6,8,14,16)
COMMAND_COMPARE = tuple(range(40,128))+(4,5,12,13,26,27,28,29,30,31,32,33)


def load_envelope(root, external_roles):
    envelope = json.loads((root/"envelope.json").read_bytes())
    demand(envelope["schema"]=="PC_SAND_RAW_ENVELOPE_V1","ENVELOPE_SCHEMA")
    family = envelope["family"].lower()
    profile = PROFILES[family]
    roles = canonical_roles(external_roles)
    variant = roles.get("roleVariant","baseline")
    demand((family, envelope["variant"], envelope["roleSetSHA"])==(roles["family"],variant,roles["roleSetSHA"]),
           "FOREIGN_ROLE_SET")
    demand(envelope["acceptedRoot"] is not None,"DIAGNOSTIC_NOT_ACCEPTED")
    demand(envelope["rootVkResult"]==0 and int(envelope["rootFence"])>0 and
           int(envelope["diagnosticGeneration"])==int(envelope["rootFenceGeneration"])+1,
           "ROOT_FENCE")
    files = {}
    for row in envelope["files"]:
        name = row["name"]
        demand(name not in files and Path(name).name==name,"FILE_NAME")
        raw = (root/name).read_bytes()
        demand(len(raw)==row["bytes"] and sha(raw)==row["sha256"],f"RAW_SHA:{name}")
        files[name] = raw
    for row in envelope["files"]:
        slot = row["slot"]
        if slot<0:
            continue
        demand(slot<8 and int(row["bufferGeneration"])>0,"BUFFER_GENERATION")
        slots = ("CURRENT_BASE.bin","BASE.bin","ACTUAL.bin","SELECTED.bin","REPLAY.bin",
                 "WORK.bin","AUTHORITY_ARENA.bin","SELECTED_ARENA.bin")
        offset = row["offset"]
        demand(offset>=0 and files[slots[slot]][offset:offset+row["bytes"]]==files[row["name"]],"RAW_RANGE")
    x = words(files["EXPECTED224.bin"])
    tail = words(files["COMPLETION256.bin"])
    p = {key:int(value) for key,value in envelope["permit"].items() if not key.endswith("SHA")}
    demand(len(x)==224 and len(tail)==256 and sha(files["EXPECTED224.bin"])==envelope["expectedSHA"],"EXPECTED_SHA")
    demand(x[:2]==[1480938055,1] and x[3]==p["kind"]==1 and p["nonce"]==int(envelope["acceptedRoot"]),"SOURCE_SCOPE")
    demand(all(pair(x,18+2*i)==p[k] for i,k in enumerate(PLAN)) and
           all(pair(x,44+2*i)==p[k] for i,k in enumerate(SCOPE)) and
           x[38:42]==[p[k] for k in ("baseBank","candidateBank","arena","ordinal")],"EXPECTED_PERMIT")
    deadline=p["deadlineNs"]
    demand(0<p["rootBeginNs"]<=p["submitEnterNs"]<=p["submitExitNs"]<=p["rootFenceSignalNs"]<=p["readExitNs"]<=
           p["gateEnterNs"]<=p["gateExitNs"]<=p["promoteExitNs"]<=deadline and
           deadline-p["rootBeginNs"]==250_000_000,"ACCEPT_DEADLINE")
    demand(0<p["pulseCallbackNs"]<=p["pulseReceiptNs"]<=p["pulseObservedNs"]<=deadline and
           p["pulseObservedNs"]-p["pulseReceiptNs"]<=33_333_333 and p["pulseFault"]==0,"PULSE_FRESHNESS")
    raw = words(files["INPUT128.bin"])
    fixture = words(files["FIXTURE128.bin"])
    demand(len(raw)==len(fixture)==128 and files["INPUT128.bin"]==files["GPU_INPUT128.bin"] and envelope["uploadMatches"] and
           sha(files["INPUT128.bin"])==envelope["inputSHA"] and blob(x[160:168]).hex()==envelope["inputSHA"],"UPLOAD_IDENTITY")
    demand((raw[0],raw[2])==profile["input"] and raw[1]==2 and raw[22:24]==[2,2] and raw[40]==1 and
           all(pair(raw,i)==p[k] for i,k in ((4,"run"),(6,"owner"),(8,"context"),(10,"nonce"),(12,"inputEpoch"))) and
           pair(raw,14)>0 and pair(raw,41)>0,"INPUT_SCOPE")
    stamp,receipt,freeze,command = (pair(raw,i) for i in (16,18,20,43))
    demand(p["rootBeginNs"]<=stamp<=receipt<=freeze<=p["submitEnterNs"] and
           0<command<=freeze and freeze-command<=250_000_000 and freeze-stamp<=250_000_000,"INPUT_CLOCK")
    demand(raw[40:43]==fixture[40:43] and raw[45:128]==fixture[45:128],"FIXTURE_BUSINESS_BODY")
    dispatches=len(roles["passes"]["Source"])
    demand((tail[0],tail[1],tail[3],tail[4])==profile["tail"] and x[4:9]==[tail[0],tail[1],tail[4],raw[2],tail[3]] and
           tail[2]==256 and tail[5]==1 and tail[39:43]==[dispatches]*3+[profile["mask"]] and x[10:12]==tail[39:43:3],
           "TYPED_HEADER")
    demand(all(pair(tail,8+2*i)==pair(x,k) for i,k in enumerate((18,44,46,48,60,26,58,50,52,20,32))) and
           tail[32:35]==x[38:41] and pair(tail,35)==pair(x,36),"TAIL_IDENTITY")
    demand(tail[64:72]==x[72:80]==raw[26:34] and tail[72:80]==x[88:96] and tail[80:88]==x[96:104] and
           tail[88:96]==x[160:168],"TAIL_HASHES")
    final_op=roles["passes"]["Source"][-1][0]
    final_module=next(row for row in roles["roles"] if row["op"]==final_op)
    demand(blob(x[96:104]).hex()==final_module["spvSHA"] and blob(x[120:128]).hex()==roles["roleSetSHA"] and
           blob(x[80:88]).hex()==sha(files["SEED.bin"]),"EXPECTED_PROGRAM_OR_SEED")
    demand(tail[6] in (1,2,3) and tail[7]==0 and not any(tail[48:54]) and not tail[63] and
           tail[43]==tail[44] and tail[45]==tail[46] and tail[44]<=x[16] and tail[46]<=x[17] and
           tail[37:39]==[0,0] and not tail[47] and not any(tail[120:156]),"TYPED_SOURCE")
    demand(pair(tail,30)==p["baseCommit"]+(tail[6]!=2) and pair(tail,54)==p["quanta"] and
           (0<tail[62]<=244666 if tail[6]==1 else tail[62]==0),"SOURCE_OUTCOME")
    if family=="r1":
        demand(len(files["VISUAL_MAPS.bin"])==524288 and len(files["VISUAL_MARKERS.bin"])==786432 and
               len(files["VISUAL_DEVICE_SEED.bin"])==271104,"R1_DEVICE_RANGES")
        demand(tail[156:160]==[1,x[183],x[7],x[6]] and
               all(tail[t:t+8]==x[e:e+8] for t,e in zip((160,168,176,184,192,200,216,224),
                                                     (152,128,136,104,112,120,80,144))) and
               tuple(pair(tail,i) for i in (208,210,212,214))==tuple(pair(x,i) for i in
                      (62,62 if tail[6]==2 else 66,64,64 if tail[6]==2 else 68)) and
               blob(tail[232:240]).hex()==envelope["expectedSHA"] and not any(tail[240:]) and
               blob(x[144:152]).hex()==sha(files["VISUAL_DEVICE_SEED.bin"]),"R1_COMPOSITE_CERT")
    else:
        demand(not any(tail[156:]),"DM2_RESERVED_TAIL")
    return family,p,raw,tail,files


def verified_source(envelope, roles):
    family,p,raw,tail,files = load_envelope(envelope,roles)
    base = words(files["BASE.bin"])
    actual = words(files["ACTUAL.bin"])
    replay = words(files["REPLAY.bin"])
    demand(len(base)==len(actual)==len(replay)==591872//4 and actual==replay,"STATE_REPLAY")
    origin = 329728//4
    for state in (base,actual):
        demand(all(pair(state,i)==p[k] for i,k in ((4,"run"),(6,"owner"),(8,"context"),(14,"inputEpoch"))),"STATE_SCOPE")
    duplicate = bool(base[104] and pair(base,28)==pair(raw,41))
    if duplicate:
        capsule=base[256:384]
        demand(all(capsule[i]==raw[i] for i in COMMAND_COMPARE),"SAME_SEQ_BODY_CONFLICT")
        demand(tail[6]==2 and base==actual,"DUPLICATE_MUTATED_STATE")
    else:
        demand(tail[6]!=2 and actual[256:384]==raw,"COMMAND_CAPSULE")
        demand(actual[19]==base[19]+(tail[6]==1),"ORIGIN_COUNT")
        if tail[6]==1:
            at=origin+16*base[19]
            demand(actual[at:at+10]==[base[19]+1,0,*raw[74:76],tail[62],0,*raw[41:43],*raw[12:14]] and
                   actual[at+10:at+14]==source_digest(raw),"SOURCE_ORIGIN_DIGEST")
    event=words(files["EVENT.bin"])
    path=files["PATH.bin"]
    demand(len(event)==tail[44]*64 and len(path)==tail[46]*64 and not path,"SOURCE_EVENT_PATH_RANGE")
    for i in range(0,len(event),64):
        demand(pair(event,i+4)==p["nonce"] and event[i+6]==i//64+1 and
               (event[i],event[i+63])==PROFILES[family]["event"] and event[i+1]==2,"SOURCE_EVENT_CERT")
    return family,p,raw,tail,files,base,actual,event,path,duplicate


def semantic_source(envelope, roles):
    family,p,raw,tail,files,base,actual,event,path,duplicate=verified_source(envelope,roles)
    normalized=raw.copy()
    zero_pairs(normalized,MAPPED_INPUT)
    # Cross-process certification currently covers one empty-origin seed and
    # one SOURCE. Duplicates are validated above using their exact raw body.
    demand(base[19]==0 and pair(base,10)==0 and not duplicate,"UNCLASSIFIED_HISTORICAL_ORIGINS")
    demand(pair(base,16)==0 and pair(actual,16)==p["nonce"],"STATE_NONCE")
    zero_pairs(base,MAPPED_STATE)
    zero_pairs(actual,MAPPED_STATE)
    actual[256:384]=normalized
    if tail[6]==1:
        origin=329728//4
        actual[origin+8:origin+10]=[0,0]
        actual[origin+10:origin+14]=source_digest(normalized)
    for i in range(0,len(event),64):
        if event[i+2]==1: # ROOT_BEGIN input sample low word.
            demand(event[i+18]==raw[14],"ROOT_BEGIN_SAMPLE")
            event[i+18]=0
        zero_pairs(event,(i+4,))
    output = dict(input=blob(normalized),base=blob(base),actual=blob(actual),event=blob(event),path=path,
                  result=blob([tail[6],tail[62]]))
    if family=="r1":
        output.update(maps=files["VISUAL_MAPS.bin"],markers=files["VISUAL_MARKERS.bin"])
    return family,output


def compare(a,b,roles_a,roles_b):
    family,left=semantic_source(a,roles_a)
    other,right=semantic_source(b,roles_b)
    demand(family==other,"CROSS_FAMILY")
    differences={}
    for name,raw in left.items():
        candidate=right[name]
        if raw!=candidate:
            at=next((i for i,(a,b) in enumerate(zip(raw,candidate)) if a!=b),min(len(raw),len(candidate)))
            differences[name]=dict(firstByte=at,leftSHA=sha(raw),rightSHA=sha(candidate))
    return dict(family=family,semanticEqual=not differences,differences=differences)


def timeline(rows, external_roles, detail):
    roles=canonical_roles(external_roles)
    boot,=[row for row in rows if row["stage"]=="BOOT"]
    root,=[row for row in rows if row["stage"]=="ROOT" and row["kind"]=="SOURCE"]
    timing,=[row for row in rows if row["stage"]=="ROOT_TIMELINE"]
    queries=[row for row in rows if row["stage"]=="ROOT_GPU_TIMESTAMPS"]
    expected=len(roles["passes"]["Source"])
    family=roles["family"].upper()
    demand(boot["family"]==family==root["family"]==timing["family"] and
           boot["roleSetSHA"]==timing["roleSetSHA"]==roles["roleSetSHA"],"FOREIGN_ROLE_SET")
    schedule="T913_SPLIT_V2" if roles.get("roleVariant")=="t913-split-v2" else "V1"
    demand(boot.get("sourceSchedule","V1")==schedule and
           boot.get("sourceDispatches",expected)==root["dispatches"]==expected,"SOURCE_DISPATCH_COUNT")
    keys=("run","nonce","plan","fence","fenceGeneration")
    demand(all(root[k]==timing[k] for k in keys) and root["deadlineNs"]==timing["deadlineNs"],"TIMELINE_IDENTITY")
    accepted=root["accepted"]
    demand(accepted==(timing["acceptedRoot"] is not None),"TIMELINE_ACCEPTANCE")
    begin,end=map(int,(timing["grantNs"],timing["deadlineNs"]))
    demand(end-begin==250_000_000,"ACCEPT_DEADLINE")
    if accepted:
        edges=[begin,*[int(timing[k]) for k in ("submitExitNs","readbackExitNs","gateEnterNs","gateExitNs",
                                                "promoteEnterNs","promoteExitNs")],end]
        demand(edges==sorted(edges) and str(timing["acceptedRoot"])==root["nonce"],"ACCEPT_DEADLINE")
    else:
        demand(not int(timing["promoteExitNs"]),"REJECTED_PROMOTED")
    expected_points=2*expected+3 if detail else 6 if family=="DM2" else 7
    if queries:
        query,=queries
        demand(all(str(query[k])==str(timing[k]) for k in keys),"QUERY_SCOPE")
        demand(len(query["points"])==expected_points,"QUERY_POINT_COUNT")
    return dict(family=family,accepted=accepted,dispatches=expected,queryPoints=expected_points if queries else None)


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest="action",required=True)
    pack=sub.add_parser("package")
    pack.add_argument("exe",type=Path)
    pack.add_argument("assets",type=Path)
    pack.add_argument("variant")
    diff=sub.add_parser("diff")
    for path in ("left","right","roles_left","roles_right"):
        diff.add_argument(path,type=Path)
    verify=sub.add_parser("verify")
    verify.add_argument("envelope",type=Path)
    verify.add_argument("roles",type=Path)
    trace=sub.add_parser("timeline")
    trace.add_argument("jsonl",type=Path)
    trace.add_argument("roles",type=Path)
    trace.add_argument("--detail",action="store_true")
    args=parser.parse_args()
    try:
        if args.action=="package":
            result=package(args.exe,args.assets,args.variant)
        elif args.action=="verify":
            checked=verified_source(args.envelope,args.roles)
            result=dict(family=checked[0],verifiedSource=True,duplicate=checked[-1])
        elif args.action=="diff":
            result=compare(args.left,args.right,args.roles_left,args.roles_right)
        else:
            result=timeline([json.loads(line) for line in args.jsonl.read_text(encoding="utf-8-sig").splitlines()],
                            args.roles,args.detail)
    except (ValueError,KeyError,OSError,struct.error) as error:
        print(json.dumps(dict(verdict="REJECTED",reason=str(error))))
        raise SystemExit(2)
    print(json.dumps(result,indent=2))
    raise SystemExit(int(bool(result.get("issues")) or result.get("semanticEqual") is False))
