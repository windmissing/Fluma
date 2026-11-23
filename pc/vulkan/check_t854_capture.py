"""Join the T854 Vulkan cold capture to the independent T857 checker."""

import argparse
import importlib.util
import json
from pathlib import Path
import struct


def verifier():
    path = Path(__file__).resolve().parents[2] / "test" / "t857" / "check.py"
    spec = importlib.util.spec_from_file_location("t857", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(receipt_path, jsonl_path, archive_path):
    judge = verifier()
    receipt = json.loads(receipt_path.read_bytes())
    judge.need(0 < archive_path.stat().st_size <= 32*1024*1024, "ARCHIVE_SIZE")
    jsonl, archive = jsonl_path.read_bytes(), archive_path.read_bytes()
    judge.need(judge.sha(jsonl) == receipt["stdoutSHA"], "JSONL_SHA")
    terminal = json.loads(jsonl.splitlines()[-1])
    meta = terminal["t854PrivateWrites"]
    judge.need(terminal["state"] == "T854_PRIVATE_WRITES_CAPTURED"
               and meta == receipt["privateWrites"] and receipt["exitCode"] == 0
               and receipt["ownerRetired"], "PRIVATE_TERMINAL")
    judge.need(meta["format"] == "G13_T854_PRIVATE_V1" and meta["acceptedRoot"] is None
               and not meta["productionEquivalent"]
               and meta["queueSubmit"] == meta["fence"] == 0
               and meta["ownerTid"] == terminal["ownerTid"], "PRIVATE_ORIGIN")
    judge.need([p["role"] for p in receipt["probes"]] ==
               ["OP17_PRIVATE_MOTION", "OP17_PRIVATE_MMS", "OP17_PRIVATE_LEDGER"] and
               all(p["mismatchMasks"] == [0, 0] and p["queueSubmit"] == p["wait"] == 0
                   and p["ownerTid"] == meta["ownerTid"] and p["processStartTick"] == terminal["processStartTick"]
                   for p in receipt["probes"]), "OP66_QUALIFICATION")
    judge.need(len(archive) == meta["fileBytes"] and judge.sha(archive) == meta["sha256"], "ARCHIVE_SHA")
    segments, sizes = meta["segments"], meta["segmentsBytes"]
    size = sum(sizes)
    judge.need(tuple(segments) == judge.BUFFERS and len(sizes) == 4 and all(n % 4 == 0 and n > 0 for n in sizes)
               and meta["initialSnapshots"] == 1 and meta["stages"] == ["BEFORE","MOTION","MMS","LEDGER"]
               and len(archive) == size * (1 + len(meta["cases"]) * 4), "SNAPSHOT_LAYOUT")
    bindings = {name: dict(object=handle, offset=0, bytes=length)
                for name, handle, length in zip(segments, meta["descriptorObjects"], sizes, strict=True)}

    def snapshot(sample):
        at = sample*size
        result = {}
        for name, length in zip(segments, sizes, strict=True):
            raw = archive[at:at+length]
            result[name] = dict(words=list(struct.unpack(f"<{length//4}I", raw)), sha256=judge.sha(raw))
            at += length
        return result

    results = []
    for i, case in enumerate(meta["cases"]):
        ident = dict(pid=str(terminal["pid"]), processStart=terminal["processStartTick"], owner=str(meta["ownerTid"]),
                     device=str(terminal["selectedDevice"]), generation=terminal["geometry"],
                     permit=f"private-diagnostic:{terminal['ticket']}", nonce=str(int(meta["firstNonce"])+i))
        events = []
        start = 1+4*i
        first = snapshot(start)
        prior = first["CTRL"]["words"]

        def event(stage, **values):
            events.append(dict(stage=stage, identity=ident, bindings=bindings, push=meta["push"], **values))

        for phase, stage in enumerate(meta["stages"][1:], 1):
            if phase > 1:
                event("BARRIER", dependency="COMPUTE_WRITE_TO_COMPUTE_READ_WRITE")
            after = snapshot(start+phase)
            ctrl = after["CTRL"]["words"]
            bits = [v for v in (1,2,4) if (ctrl[751] & ~prior[751]) & v]
            if not prior[514] and ctrl[514] and ctrl[750] in bits:
                bits.remove(ctrl[750])
                bits.insert(0,ctrl[750])
            event(stage, after=after, mathBits=bits)
            prior = ctrl
        event("FENCE", vkResult=meta["fence"])
        row = dict(synthetic=False, push=meta["push"], identity=ident, bindings=bindings, layoutWords=meta["layoutWords"],
                   privateWord=meta["privateWord"], privateTag=0x54383534, before=first, events=events, reference=None,
                   revoked=False, publicationBefore=None, publicationAfter=None, completionPublished=False,
                   submitVkResult=meta["queueSubmit"])
        results.append(dict(case=case, result=judge.check(row), firstError=prior[513:518],
                            logicCounts=prior[522:524], mathMask=prior[751]))
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for path in ("receipt", "jsonl", "archive"):
        parser.add_argument(path, type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(run(args.receipt,args.jsonl,args.archive),indent=2))
    except (OSError, KeyError, ValueError, IndexError, struct.error) as error:
        parser.exit(1,f"{error}\n")
