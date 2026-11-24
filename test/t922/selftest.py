"""Synthetic boundary cases plus independent checks of the six frozen T914 packages."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory

from check import (PLAN, PROFILES, SCOPE, blob, canonical_roles, compare, demand, package,
                   sha, source_digest, timeline, verified_source, words)


def put(a,i,value):
    a[i:i+2]=[value&0xffffffff,value>>32]


def digest_words(raw):
    return words(bytes.fromhex(sha(raw)))


def fixture(out, roles_path, serial, *, body=25, case="normal"):
    roles=canonical_roles(roles_path)
    family=roles["family"]
    profile=PROFILES[family]
    out.mkdir()
    p=dict(zip(SCOPE,(1000+serial,11+serial,21+serial,31+serial,1,1,1,1,1)))
    start=1_000_000_000+serial*400_000_000
    p.update(nonce=1+serial,plan=2+serial,frame=3+serial,control=4+serial,geometry=1,
             rootBeginNs=start,deadlineNs=start+250_000_000,baseCommit=0,quanta=0,arenaGeneration=1,
             baseBank=0,candidateBank=2,arena=0,ordinal=0,kind=1,promoteExitNs=start+120_000_000,
             submitEnterNs=start+3_000_000,submitExitNs=start+4_000_000,rootFenceSignalNs=start+117_000_000,
             readExitNs=start+118_000_000,gateEnterNs=start+118_100_000,gateExitNs=start+118_200_000,
             pulseObservedNs=start+119_000_000,pulseCallbackNs=start+112_000_000,pulseReceiptNs=start+112_100_000,
             pulseFault=0)
    duplicate=case.startswith("duplicate")
    if duplicate:
        p["baseCommit"]=p["arenaGeneration"]=1
    raw=[0]*128
    raw[:3]=[profile["input"][0],2,profile["input"][1]]
    for at,key in ((4,"run"),(6,"owner"),(8,"context"),(10,"nonce"),(12,"inputEpoch")):
        put(raw,at,p[key])
    put(raw,14,serial+1)
    for at in (16,18,20,43):
        put(raw,at,start+2_000_000)
    raw[22:26]=[2,2,0,6]
    raw[26:34]=digest_words(b"fixture parameters")
    raw[40:43]=[1,5,0]
    raw[46]=body
    raw[74]=2
    base=[0]*(591872//4)
    for at,key in ((4,"run"),(6,"owner"),(8,"context"),(14,"inputEpoch")):
        put(base,at,p[key])
    if duplicate:
        base[10]=1
        base[28]=5
        base[104]=1
        base[256:384]=raw
        if case=="duplicate-conflict":
            base[256+46]^=1
    actual=base.copy()
    admitted=0 if duplicate else body
    if not duplicate:
        actual[10]=1
        put(actual,16,p["nonce"])
        actual[19]=1
        actual[28]=5
        actual[104]=1
        actual[256:384]=raw
        at=329728//4
        actual[at:at+14]=[1,0,2,0,admitted,0,5,0,*raw[12:14],*source_digest(raw)]
        if case=="wrong-digest":
            actual[at+10]^=1
    x=[0]*224
    tail=[0]*256
    magic,revision,seal,kind_profile=profile["tail"]
    n=len(roles["passes"]["Source"])
    x[:12]=[1480938055,1,2 if family=="dm2" else 4,1,magic,revision,kind_profile,raw[2],seal,256,n,profile["mask"]]
    x[16:18]=[100,200]
    for i,k in enumerate(PLAN):
        put(x,18+2*i,p[k])
    for i,k in enumerate(SCOPE):
        put(x,44+2*i,p[k])
    x[38:42]=[p[k] for k in ("baseBank","candidateBank","arena","ordinal")]
    x[72:80]=raw[26:34]
    x[80:88]=digest_words(blob(base))
    final=roles["passes"]["Source"][-1][0]
    x[96:104]=words(bytes.fromhex(next(m["spvSHA"] for m in roles["roles"] if m["op"]==final)))
    x[120:128]=words(bytes.fromhex(roles["roleSetSHA"]))
    x[160:168]=digest_words(blob(raw))
    tail[:8]=[magic,revision,256,seal,kind_profile,1,2 if duplicate else 1,0]
    for i,at in enumerate((18,44,46,48,60,26,58,50,52,20,32)):
        put(tail,8+2*i,pair(x,at))
    tail[32:35]=x[38:41]
    put(tail,35,p["arenaGeneration"])
    tail[39:43]=[n,n,n,profile["mask"]]
    if not duplicate:
        tail[43:45]=[1,1]
    tail[30]=1
    tail[62]=admitted
    tail[64:96]=x[72:80]+x[88:96]+x[96:104]+x[160:168]
    event=[0]*64 if not duplicate else []
    if event:
        event[:3]=[profile["event"][0],2,1]
        put(event,4,p["nonce"])
        event[6]=1
        event[18]=raw[14]
        event[63]=profile["event"][1]
    if family=="r1":
        visual_seed=b"\0"*271104
        x[144:152]=digest_words(visual_seed)
        tail[156:160]=[1,x[183],x[7],x[6]]
        for t,e in zip((160,168,176,184,192,200,216,224),(152,128,136,104,112,120,80,144)):
            tail[t:t+8]=x[e:e+8]
        tail[232:240]=digest_words(blob(x))
    if case=="late":
        p["promoteExitNs"]=p["deadlineNs"]+1
    files = {"EXPECTED224.bin":blob(x),"COMPLETION256.bin":blob(tail),"INPUT128.bin":blob(raw),
             "GPU_INPUT128.bin":blob(raw),"FIXTURE128.bin":blob(raw),"BASE.bin":blob(base),
             "ACTUAL.bin":blob(actual),"REPLAY.bin":blob(actual),"SEED.bin":blob(base),
             "EVENT.bin":blob(event),"PATH.bin":b""}
    if family=="r1":
        files.update({"VISUAL_MAPS.bin":b"\0"*524288,"VISUAL_MARKERS.bin":b"\0"*786432,
                      "VISUAL_DEVICE_SEED.bin":visual_seed})
    rows=[]
    for name,raw in files.items():
        (out/name).write_bytes(raw)
        rows.append(dict(name=name,bytes=len(raw),sha256=sha(raw),slot=-1))
    receipt=dict(schema="PC_SAND_RAW_ENVELOPE_V1",family=family.upper(),
                 variant=roles.get("roleVariant","baseline"),roleSetSHA=roles["roleSetSHA"],
                 acceptedRoot=str(p["nonce"]),rootVkResult=0,rootFence="1",rootFenceGeneration="2",
                 diagnosticGeneration="3",uploadMatches=True,files=rows,expectedSHA=sha(blob(x)),
                 inputSHA=sha(files["INPUT128.bin"]),permit=p)
    (out/"envelope.json").write_text(json.dumps(receipt),encoding="utf-8")
    if case=="hash-tamper":
        (out/"ACTUAL.bin").write_bytes(b"tampered")


def pair(a,i):
    return a[i]+(a[i+1]<<32)


def fail_with(action, reason):
    try:
        action()
    except ValueError as error:
        demand(str(error).startswith(reason),f"expected {reason}, got {error}")
    else:
        raise AssertionError(f"missing rejection {reason}")


def run(local,published):
    output={}
    for family in ("dm2","r1"):
        paths={}
        for variant,count in (("baseline",0),("t913-v1",0),("t913-split-v2",3 if family=="dm2" else 7)):
            old=local/f"t913-{family}-{variant}"
            exe=old/f"pc_sand_{family}_host.exe"
            assets=old/"sand_assets"
            result=package(exe,assets,variant)
            demand(len(result["issues"])==count,"FROZEN_PACKAGE_RESULT")
            if count:
                demand(result["issues"][0]["opcode"]==0x380 and result["issues"][0]["reason"]=="FILE_LENGTH",
                       "FROZEN_MIXED_ROLE")
            output[f"{family}:{variant}"]=result
            paths[variant]=assets/"roles.json"
        roles=paths["baseline"]
        v1=paths["t913-v1"]
        with TemporaryDirectory() as tmp:
            d=Path(tmp)
            fixture(d/"a",roles,1)
            fixture(d/"b",v1,2)
            fixture(d/"different",roles,3,body=26)
            demand(compare(d/"a",d/"b",roles,v1)["semanticEqual"],"CROSS_PROCESS_POSITIVE")
            demand("input" in compare(d/"a",d/"different",roles,roles)["differences"],"BUSINESS_DIFFERENCE")
            fail_with(lambda: verified_source(d/"a",v1),"FOREIGN_ROLE_SET")
            for i,(case,error) in enumerate((("wrong-digest","SOURCE_ORIGIN_DIGEST"),("late","ACCEPT_DEADLINE"),
                                            ("duplicate-conflict","SAME_SEQ_BODY_CONFLICT"),("hash-tamper","RAW_SHA")),4):
                fixture(d/case,roles,i,case=case)
                fail_with(lambda: verified_source(d/case,roles),error)
            fixture(d/"duplicate",roles,9,case="duplicate")
            demand(verified_source(d/"duplicate",roles)[-1],"TRUE_DUPLICATE")
            original=Path(__file__).resolve().parents[1]/"reports/t914/runs"/f"{family}-baseline-detail-prewarm.jsonl"
            rows=[json.loads(line) for line in original.read_text(encoding="utf-8-sig").splitlines()]
            demand(timeline(rows,roles,True)["queryPoints"]==(27 if family=="dm2" else 41),"FROZEN_QUERY_COUNT")
            fail_with(lambda: timeline(rows,roles,False),"QUERY_POINT_COUNT")
            if family=="dm2":
                timing=next(row for row in rows if row["stage"]=="ROOT_TIMELINE")
                timing["promoteExitNs"]=str(int(timing["deadlineNs"])+1)
                fail_with(lambda: timeline(rows,roles,True),"ACCEPT_DEADLINE")
            fresh=published/f"{family}-split-v2"
            if fresh.is_dir():
                exe=fresh/f"pc_sand_{family}_host.exe"
                assets=d/"assets"
                shutil.copytree(fresh/"sand_assets",assets)
                demand(not package(exe,assets,"t913-split-v2")["issues"],"FRESH_PACKAGE")
                fail_with(lambda: package(exe,assets,"t913-v1"),"WRONG_VARIANT")
                role=assets/"role_00000380.spv"
                original=role.read_bytes()
                for fault,modified in (("FILE_LENGTH",original[:-4]),
                                       ("FILE_SHA",original[:-1]+bytes([original[-1]^1]))):
                    role.write_bytes(modified)
                    bad=package(exe,assets,"t913-split-v2")
                    child=subprocess.run([str(exe),"--check-package",str(assets)],capture_output=True)
                    stop=json.loads(child.stdout.splitlines()[-1])
                    demand(bad["issues"][0]["reason"]==fault and child.returncode==1 and stop["opcode"]==0x380 and
                           stop["phase"]==fault and stop["moduleVkResult"] is None,"MUTATED_ROLE_REJECTION")
                output[f"{family}:freshNegativeControls"]=2
        output[f"{family}:synthetic"]=dict(crossProcess=True,payloadDifference=True,trueDuplicate=True,rejections=5)
    return output


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--local",type=Path,default=Path(".codex/local-test"))
    parser.add_argument("--published",type=Path,default=Path(".codex/local-test/t921-release"))
    args=parser.parse_args()
    print(json.dumps(run(args.local,args.published),indent=2))
