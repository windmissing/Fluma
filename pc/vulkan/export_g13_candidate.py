"""Project T833 Vulkan capture bytes into the T607 archive and a GPU-backed GridView."""

import argparse
import hashlib
import json
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[2]
SEAL = 0xc0de1301
LAYOUT = json.loads((ROOT / "docs/architecture-T607-g13-layout-v1.json").read_bytes())
ENGINEERING = json.loads((ROOT / "docs/architecture-T607-g13-engineering-v1.json").read_bytes())
SEGMENTS = ("BASE", "CANDIDATE", "REPLAY", "WORK", "CTRL", "PACK0", "PACK1")
MAX_RAW = 16 * 1024 * 1024


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def words(data):
    return struct.unpack(f"<{len(data)//4}I", data)


def pack(items):
    return struct.pack(f"<{len(items)}I", *items)


def pair(items, offset):
    return items[offset] | items[offset + 1] << 32


def put(items, offset, value):
    items[offset:offset + 2] = [value & 0xffffffff, value >> 32]


def f32(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def encoded(obj):
    return json.dumps(obj, separators=(",", ":"), allow_nan=False).encode() + b"\n"


def tag(prefix, offset, length, kind, obj):
    payload = encoded(obj).rstrip(b"\n")
    require(len(payload) <= 4 * (length - 16), "TAG_CAPACITY")
    header = [kind, 1, 1, length, len(payload), 0, *words(bytes.fromhex(sha(payload))), 0, SEAL]
    offset *= 4
    prefix[offset:offset + 64] = pack(header)
    prefix[offset + 64:offset + 64 + len(payload)] = payload


def split_capture(raw, meta):
    require(meta["schema"] == "PC_G13_T833_RAW_V2" and len(raw) == meta["fileBytes"]
            and sha(raw) == meta["sha256"], "RAW_FILE_IDENTITY")
    require(tuple(meta["segments"]) == SEGMENTS, "RAW_SEGMENT_ORDER")
    require(meta["slotCount"] in (0, 14) and meta["slotBytes"] == 168
            and meta["fieldOffset"] == 1024 + meta["slotCount"] * meta["slotBytes"], "RAW_SLOT_RANGE")
    require(len(meta["segmentsBytes"]) == len(SEGMENTS), "RAW_SEGMENT_COUNT")
    at, pieces = meta["fieldOffset"], {}
    for name, length in zip(SEGMENTS, meta["segmentsBytes"], strict=True):
        require(type(length) is int and length > 0 and length % 4 == 0
                and at + length <= len(raw), "RAW_SEGMENT_BOUNDS")
        pieces[name] = raw[at:at + length]
        at += length
    require(at == len(raw), "RAW_TRAILING_BYTES")
    require(len(pieces["CTRL"]) == 74752 and raw[:1024] == pieces["CTRL"][73728:]
            and words(raw[:1024]) == tuple(meta["completion"]), "RAW_COMPLETION_JOIN")
    return pieces


def archive_parts(pieces, meta, kind):
    ctrl = words(pieces["CTRL"])
    config, permit, tail = ctrl[:128], ctrl[128:256], ctrl[18432:]
    nx, ny, boundary = config[4:7]
    profile = next((p for p in LAYOUT["profiles"] if (p["Nx"], p["Ny"]) == (nx, ny)), None)
    require(profile is not None, "GRID_PROFILE")
    nonce = pair(permit, 8)
    require(permit[0] == 1 and permit[127] == SEAL and nonce > 0, "PERMIT_IDENTITY")
    require(sha(meta["baseToken"].encode()) == pack(permit[80:88]).hex(), "BASE_TOKEN")
    require(tail[:4] == (LAYOUT["magic"]["tail"], 1, LAYOUT["magic"]["profile"], 256)
            and tail[255] == SEAL and pair(tail, 8) == nonce, "TAIL_IDENTITY")
    grant, deadline, accept = pair(permit, 36), pair(permit, 38), int(meta["acceptedNs"])
    accepted = meta["acceptedRoot"] is not None
    diagnostic = meta["privateProjectComplete"]
    if accepted:
        require(meta["acceptedRoot"] == str(nonce) and tail[5] == 1 and tail[7] == 0
                and tail[42] == 1 and grant <= accept <= deadline, "NATIVE_ACCEPTANCE")
    if diagnostic:
        require(not accepted and not accept and tail[5] == 7 and tail[126] == 1
                and tail[42] == 0, "DIAGNOSTIC_ACCEPTANCE")
    require(meta["nativeError"] == 0 if accepted or diagnostic else meta["nativeError"] != 0,
            "NATIVE_OUTCOME")
    directory = profile[f"{kind}ColdDirectory"]
    h = [0] * 256
    h[:8] = [LAYOUT["magic"]["cold"], 1, LAYOUT["magic"]["profile"], 256,
             2 if kind == "FULL" else 1, int(not (accepted or diagnostic)), len(directory), 0]
    total = sum(d["byteLength"] for d in directory)
    h[7] = (total + 4095) // 4096
    for offset, value in ((8, nonce), (10, nonce), (12, accept or grant), (14, pair(permit, 12)),
                          (16, pair(permit, 16)), (18, pair(permit, 18)), (20, pair(permit, 20)),
                          (22, pair(tail, 26) if accepted else pair(tail, 24)), (24, nonce), (26, total)):
        put(h, offset, value)
    for offset, source in ((32, 36), (40, 44), (48, 52), (64, 76), (72, 60), (88, 68)):
        h[offset:offset + 8] = config[source:source + 8]
    h[56:64] = words(bytes.fromhex(ENGINEERING["hashes"]["mathSHA"]))
    h[80:88] = permit[72:80]
    h[96:104] = [nx, ny, boundary, permit[1], tail[5], tail[7], 1024, permit[92]]
    prefix = bytearray(65536)
    prefix[:1024] = pack(h)
    out = [prefix]
    banks = {"BASE": meta["baseBank"], "CANDIDATE": meta["candidateBank"], "REPLAY": 2,
             "PACK0": 0, "PACK1": 1}
    require({banks["BASE"], banks["CANDIDATE"]} == {0, 1}, "STATE_BANK_ALIASES")
    for i, d in enumerate(directory):
        name = d["name"]
        # The Vulkan-only 128 byte OP17 latch remains in the raw source artifact.
        raw_segment = prefix if i == 0 else pieces[name]
        if name == "WORK":
            require(len(raw_segment) == d["byteLength"] + 128, "PRIVATE_WORK_SIZE")
            raw_segment = raw_segment[:-128]
        require(len(raw_segment) == d["byteLength"], "SEGMENT_PROFILE_SIZE")
        desc = [0] * 32
        desc[:6] = [i + 1, 1, i, 0, banks.get(name, 0), d["strideWords"]]
        put(desc, 6, d["byteOffset"])
        put(desc, 8, len(raw_segment))
        desc[10:13] = [len(raw_segment)//4] * 3
        put(desc, 13, pair(words(raw_segment[:72]), 16) if name in banks else pair(h, 22))
        put(desc, 15, nonce)
        desc[26], desc[31] = 1, SEAL
        if i:
            desc[17:25] = words(bytes.fromhex(sha(raw_segment)))
            out.append(raw_segment)
        prefix[(256 + 32*i)*4:(288 + 32*i)*4] = pack(desc)
    for start, end, part in ((1280, 1408, pack(config)), (1408, 2432, pack(ctrl[1024:2048])),
                             (4736, 4992, pack(ctrl[256:512])), (5760, 5888, pack(permit))):
        prefix[start*4:end*4] = part
    first, aggregate, detail = ctrl[750], ctrl[751], ctrl[766]
    observed = ctrl[513] < 64 and detail == 0xc0de1311 and pair(ctrl, 733) == nonce and (
        ctrl[514], ctrl[513], ctrl[515], ctrl[516], ctrl[517]) == (tail[7], tail[6], *tail[122:125])
    scalar = bool(observed and first)
    error = meta["nativeError"]
    cause = ("MATH_DETAIL_NOT_OBSERVED" if not observed else "NUMERIC_DOMAIN" if first
             else "NUMERIC_OVERFLOW") if error == 17 else "NON_MATH_FIRST_ERROR" if error else "NONE"
    native = dict(nonce=str(nonce), deadlineNs=str(deadline), nativeError=error, backend="VULKAN_PC",
                  nativeOutcome=7 if diagnostic else 1 if accepted else 4 if error == 11 else 3,
                  acceptedNs=str(accept), gpuTailAvailable=True, firstFaultObserved=observed,
                  firstMathStatus=first if observed else None, aggregateMathStatus=aggregate,
                  firstFaultDetailSeal=detail, firstFaultTuple=[*ctrl[733:735], ctrl[513], *ctrl[515:518], ctrl[514]],
                  firstCause=cause, aliasInjection=list(ctrl[828:836]))
    tag(prefix, 2432, 512, 1, {"backend": "VULKAN_PC"})
    tag(prefix, 2944, 512, 2, {"qualificationWords": ctrl[2816:2944],
                              "firstFaultScalarObserved": scalar,
                              "firstFaultScalarWords": ctrl[733:750] if scalar else []})
    tag(prefix, 3456, 1024, 3, {"candidate": "T861_SPLIT", "captureSHA": meta["sha256"]})
    tag(prefix, 4480, 256, 4, {"state": "GPU_PACK_CAPTURE", "present": "NOT_OBSERVED"})
    tag(prefix, 4992, 512, 5, {"backend": "VULKAN_PC", "buffers": meta["descriptorBuffers"]})
    tag(prefix, 5504, 256, 6, native)
    tag(prefix, 5888, 128, 7, {"acceptedBaseToken": meta["baseToken"]})
    prefix[248*4:256*4] = bytes.fromhex(sha(prefix))
    return out, profile, config, tail, native


def grid_view(pieces, meta, profile, config, tail):
    require(meta["acceptedRoot"] is not None and tail[4] == 0, "PHYSICAL_QUANTUM_REQUIRED")
    render = words(pieces[f'PACK{meta["publicationSlot"]}'])
    nx, ny = config[4:6]
    require(len(render) == 64 + 16*nx*ny and render[0] == 1446195527 and render[1] == 1
            and render[4:6] == (nx, ny) and render[7] == 1 and render[63] == SEAL
            and pair(render, 18) == pair(tail, 8) and pair(render, 20) == pair(tail, 26)
            and pair(render, 14) == pair(tail, 20) and render[24:32] == config[76:84],
            "GPU_PACK_ROOT_JOIN")
    candidate = pieces["CANDIDATE"]
    field = {}
    for d in profile["state"]:
        if d["name"] in ("u", "v", "piQ32"):
            data = candidate[d["byteStart"]:d["byteStart"] + d["bytes"]]
            codec = "<q" if d["name"] == "piQ32" else "<I"
            field[d["name"]] = [t[0] for t in struct.iter_unpack(codec, data)]
    return dict(shape=[nx, ny], u32=field["u"], v32=field["v"], piQ32=field["piQ32"],
                phi=[f32(render[64 + 16*i + 3]) for i in range(nx*ny)],
                phiSource="SAME_ROOT_GPU_PACK_FIELD_3", pRef=f32(render[48]), T0=f32(config[11]),
                motion={0: "REST", 1: "REST", 2: "RAMP_SPIN", 3: "CONSTANT_SPIN"}[
                    struct.unpack_from("<I", candidate, 30*4)[0]],
                velocityUnit="m/s", pressureUnit="Pa", acceptedRoot=meta["acceptedRoot"],
                geometry=str(pair(tail, 20)), commit=str(pair(tail, 26)), nonce=str(pair(tail, 8)))


def write_new(path, raw):
    with path.open("xb") as output:
        try:
            output.write(raw)
        except OSError:
            output.close()
            path.unlink()
            raise


def page_archive(parts, trace):
    pages, offset = [], 0
    for segment, data in enumerate(parts):
        for start in range(0, len(data), 4096):
            pages.append(encoded(dict(route=13, schema=1, state="PAGE", id=trace, segment=segment,
                                      wordOffset=str(start//4), rawByteOffset=str(offset + start),
                                      words=words(data[start:start + 4096]))))
        offset += len(data)
    prefix = parts[0]
    return b"".join(pages) + encoded(dict(schema="G13_ARCHIVE_V1", id=trace, rawBytes=offset,
        pages=len(pages), payloadRawSHA=sha(b"".join(parts)), pagesFileSHA=sha(b"".join(pages)),
        canonicalPrefixSHA=prefix[248*4:256*4].hex(), rawPrefixSHA=sha(prefix)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("raw", type=Path)
    parser.add_argument("receipts", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--mode", choices=("ROOT", "FULL"), default="FULL")
    args = parser.parse_args()
    require(0 < args.raw.stat().st_size <= MAX_RAW, "RAW_FILE_SIZE")
    raw = args.raw.read_bytes()
    digest = sha(raw)
    meta = None
    probes = {}
    with args.receipts.open("rb") as source:
        for line in source:
            row = json.loads(line)
            if row.get("schema") == "PC_G13_STEADY_OP66_V1":
                probes[(row["pid"], row["processStartTick"], row["ownerTid"], row["role"])] = row
            for name in ("t833SourceArchive", "t833QuantumArchive"):
                if row.get(name, {}).get("sha256") == digest:
                    meta, receipt = row[name], row
    require(meta is not None, "RAW_RECEIPT_MISSING")
    pieces = split_capture(raw, meta)
    parts, profile, config, tail, native = archive_parts(pieces, meta, args.mode)
    trace = native["nonce"]
    archive = page_archive(parts, trace)
    roles = {r["role"]: r["sha256"] for r in receipt["candidateModules"]}
    op66 = [probes[(receipt["pid"], receipt["processStartTick"], receipt["ownerTid"], role)] for role in roles]
    require(all(r["moduleSHA"] == roles[r["role"]] for r in op66), "OP66_ROLE_IDENTITY")
    manifest = dict(synthetic=False, case=receipt["candidateInput"], archiveSHA=sha(archive),
                    completionSHA=sha(raw[:1024]), roles=roles, op66=op66,
                    nativeAcceptedNs=meta["acceptedNs"], native=native)
    for key, at in (("nonce", 8), ("plan", 10), ("owner", 14), ("context", 16),
                    ("surface", 18), ("geometry", 20), ("configGeneration", 22), ("commit", 26),
                    ("acceptedQuantum", 34)):
        manifest[key] = str(pair(tail, at))
    manifest["acceptedRoot"] = meta["acceptedRoot"]
    if args.mode == "FULL" and meta["acceptedRoot"] is not None and tail[4] == 0:
        manifest["view"] = grid_view(pieces, meta, profile, config, tail)
    write_new(args.output, archive)
    write_new(Path(str(args.output) + ".receipt.json"),
              encoded(dict(schema="G13_FILE_RECEIPT_V1", sha256=sha(archive))))
    write_new(Path(str(args.output) + ".grid.json"), encoded(manifest))


if __name__ == "__main__":
    main()
