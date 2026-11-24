"""Resolve dump owner RIPs to module RVAs and correlate process CPU receipts."""

import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct


def owner_context(data, tid):
    if data[:4] != b"MDMP":
        raise ValueError("MINIDUMP_SIGNATURE")
    count, directory = struct.unpack_from("<II", data, 8)
    streams = {}
    for i in range(count):
        stream, size, offset = struct.unpack_from("<III", data, directory + 12*i)
        if offset + size > len(data):
            raise ValueError("MINIDUMP_STREAM_BOUNDS")
        streams[stream] = offset
    if struct.unpack_from("<H", data, streams[7])[0] != 9:
        raise ValueError("EXPECTED_AMD64_DUMP")
    modules = []
    offset = streams[4]
    for i in range(struct.unpack_from("<I", data, offset)[0]):
        base, length, _, timestamp, name = struct.unpack_from("<QIIII", data, offset+4+108*i)
        n = struct.unpack_from("<I", data, name)[0]
        path = data[name+4:name+4+n].decode("utf-16le")
        modules.append((base, length, path, timestamp))

    def resolve(address):
        for base, length, path, timestamp in modules:
            if base <= address < base+length:
                return dict(module=path.replace("\\", "/").split("/")[-1].lower(), path=path,
                            rva=f"0x{address-base:x}", imageTimeDateStamp=timestamp)
        return dict(address=f"0x{address:x}")

    threads = streams[3]
    for i in range(struct.unpack_from("<I", data, threads)[0]):
        offset = threads+4+48*i
        if struct.unpack_from("<I", data, offset)[0] != tid:
            continue
        start, size, stack, ctx_bytes, ctx = struct.unpack_from("<QIIII", data, offset+24)
        if ctx_bytes<256 or ctx+ctx_bytes>len(data) or stack+size>len(data):
            raise ValueError("MINIDUMP_CONTEXT_BOUNDS")
        rsp = struct.unpack_from("<Q", data, ctx+152)[0]
        rip = struct.unpack_from("<Q", data, ctx+248)[0]
        if not start<=rsp<=start+size:
            raise ValueError("OWNER_STACK_BOUNDS")
        # These are addresses found in the captured stack, not unwound frames.
        slots = []
        for address in range(rsp,min(rsp+256,start+size-7),8):
            pointer = struct.unpack_from("<Q", data, stack+address-start)[0]
            target = resolve(pointer)
            if "module" in target:
                slots.append(dict(offset=address-rsp,**target))
        return dict(ip=resolve(rip), firstStackReferences=slots)
    raise ValueError("OWNER_THREAD_MISSING")


def analyze(directory):
    result = []
    for path in sorted(directory.glob("g13-*/*/OP_48.receipt.json")):
        row = json.loads(path.read_text(encoding="utf-8"))
        build = row["lastPipeline"]
        tid = build["ownerTid"]
        enter = int(build["qpc"])
        samples = [s for s in row["samples"] if s["ownerTid"]==tid and int(s["qpc"])>=enter and
                   s["ownerTimes"] and s["processTimes"]]
        if not samples:
            raise ValueError(f"No same-owner CPU samples: {path}")
        if build["role"]!="OP_48" or build["edge"]!="ENTER" or row["reason"]!="CALL_UNRETURNED":
            raise ValueError(f"Wrong comparison arm: {path}")
        first,last = samples[0],samples[-1]
        delta = lambda name: sum(int(last[name][i])-int(first[name][i]) for i in (2,3))
        dumps = []
        for capture in row["stackDumps"]:
            dump = path.parent/capture["file"]
            raw = dump.read_bytes()
            if sha256(raw).hexdigest()!=capture["sha256"] or tid!=capture["ownerTid"]:
                raise ValueError(f"Dump digest or thread does not match: {dump}")
            dumps.append(dict(seconds=capture["targetSeconds"],sha256=capture["sha256"],**owner_context(raw,tid)))
        images = row["modules"]["loaderAndDriverCandidates"]
        result.append(dict(arm=str(path.relative_to(directory).parent), pid=row["pid"],tid=tid,
                           observedSeconds=(int(last["qpc"])-int(first["qpc"]))/int(row["qpcFrequency"]),
                           ownerCpuSeconds=delta("ownerTimes")/10_000_000,
                           processCpuSeconds=delta("processTimes")/10_000_000,
                           loaded={k:v for k,v in images.items() if "vulkan-1" in k.lower() or "igc64" in k.lower()
                                   or "igvk64" in k.lower()},dumps=dumps))
    if not result:
        raise ValueError("NO_COMPARISON_RECEIPTS")
    counts = Counter(d["ip"].get("module","unresolved") for row in result for d in row["dumps"])
    return dict(arms=len(result),ownerIpModules=dict(counts),captures=result)


if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory",type=Path)
    parser.add_argument("output",type=Path)
    args=parser.parse_args()
    args.output.write_text(json.dumps(analyze(args.directory),indent=2)+"\n",encoding="utf-8")
