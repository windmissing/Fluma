"""Seal T927 OFF1's accepted action 7 and real adjacent pulse evidence."""

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct


LOG = "b0b4209a1d28d739f8a396ce0f9ee81e2c12dce42796dc3660a145bd27ec1007"


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def seal(path, destination):
    raw = path.read_bytes()
    if sha(raw) != LOG:
        raise ValueError("Unexpected T927 OFF1 stdout")
    rows = [json.loads(line) for line in raw.splitlines()]
    reference, = [r for r in rows if r.get("m1OfferAction") == 7 and "m1SourceWords" in r]
    source, quantum = reference["m1Completion"], reference["m1QuantumCompletion"]
    p, cmd = reference["m1ProducerWords"], reference["m1SourceWords"]
    p_raw, cmd_raw = struct.pack("<25Q", *p),struct.pack("<32I", *cmd)
    left, right = (
        next(r for r in rows if r.get("stage") == "M1_D1_PULSE_OWNER" and r.get("actionId") == i)
        for i in (6, 8)
    )
    qpc0, qpc1 = int(left["callbackQpc"]), int(right["callbackQpc"])
    receipt0, receipt1 = int(left["nativeReceiptQpc"]), int(right["nativeReceiptQpc"])
    freq = p[12]
    elapsed = qpc1*10**9//freq - qpc0*10**9//freq
    x, y, qty = struct.unpack("<3d", cmd_raw[64:88])
    if (left["outcome"] != "ACCEPTED" or right["outcome"] != "ACCEPTED" or
            left["session"] != right["session"] or left["geometry"] != right["geometry"] or
            int(right["seq"]) != int(left["seq"]) + 1 or not (qpc0 <= receipt0 < qpc1 <= receipt1) or
            elapsed != p[23] or qpc1 != p[10] or not math.isclose(qty, 8.*elapsed/10**9, rel_tol=0,abs_tol=1e-15) or
            int(left["geometry"]) != p[4] or int(left["run"]) != p[0] or p_raw[19*8:21*8] != cmd_raw[64:80] or
            not (0 <= x < 128 and 0 <= y < 256) or cmd[2] != p[21] or
            sha(cmd_raw) != reference["m1SourceWordsSHA"] or sha(p_raw[:-8]) != reference["m1ProducerSHA"] or
            source[14:18] != [0, 0, 0, 0] or source[28] != 1 or source[61] != round(qty*(1<<20)) or
            quantum[27] != 2 or quantum[28] != 1 or quantum[26] != 1 or quantum[52] != 0 or quantum[107] != 0):
        raise ValueError("Reference inputs are not the frozen accepted empty-BASE SOURCE → Q")
    manifest = struct.pack("<8Q",qpc0,qpc1,receipt0,receipt1,freq,elapsed,int(left["session"]),source[61])
    packet = b"M1OFF933"+bytes.fromhex(LOG)+manifest+p_raw+cmd_raw+struct.pack("<512I",*source,*quantum)
    with destination.open("xb") as stream:
        stream.write(packet)
    print(json.dumps(dict(referenceSHA=sha(packet), logSHA=LOG, action=7, deltaNs=elapsed, admittedQ20=source[61])))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("out", type=Path)
    args = parser.parse_args()
    seal(args.log, args.out)
