"""Rebuild the T928 little-endian cold seed streams from the reviewed ABI."""
import argparse
import hashlib
import json
import math
from itertools import combinations
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[2]
ABI = ROOT / "docs/architecture-T928-abi-v1.json"
DISK = struct.Struct("<4I9f3I")


def occupied_centers(seed):
    for row in seed["rows"]:
        y, first_x, pitch, columns = row
        for column in columns:
            yield (first_x + pitch * column, y)


def initial_sweep(seed, half_extent, centers):
    """Check the zero-angle cold seed with the full permitted translation pad."""
    cell = seed["bucketUm"]
    radius = seed["radiusUm"]
    move = radius // 10
    epsilon = min(50, radius//25)
    max_distance = 2*radius + 2*move + epsilon
    assert cell >= 2*radius
    assert seed["bucketWidthHeight"] == [2*extent//cell for extent in half_extent]
    buckets = {}
    for i,(x,y) in enumerate(centers):
        key = ((x+half_extent[0])//cell, (y+half_extent[1])//cell)
        buckets.setdefault(key, []).append(i)

    def near(i,j):
        a,b = centers[i], centers[j]
        return (a[0]-b[0])**2+(a[1]-b[1])**2 <= max_distance**2

    reference = {(i,j) for i,j in combinations(range(len(centers)),2) if near(i,j)}
    discovered = []
    for i,(x,y) in enumerate(centers):
        lower = [(coord+extent-max_distance)//cell for coord,extent in zip((x,y),half_extent)]
        upper = [(coord+extent+max_distance)//cell for coord,extent in zip((x,y),half_extent)]
        for cy in range(lower[1],upper[1]+1):
            for cx in range(lower[0],upper[0]+1):
                discovered.extend((i,j) for j in buckets.get((cx,cy),[]) if i < j and near(i,j))
    assert set(discovered) == reference and len(discovered) == len(reference)
    degree = [0]*len(centers)
    for i,j in reference:
        degree[i] += 1
        degree[j] += 1
    walls = 0
    for i,(x,y) in enumerate(centers):
        hx,hy = half_extent
        live = sum(gap <= move+epsilon for gap in (hy+y-radius,hy-y-radius,hx+x-radius,hx-x-radius))
        degree[i] += live
        walls += live
    assert len(reference)+walls <= 8192 and max(degree) <= 16
    return {"coldDiskCandidates": len(reference), "coldWallCandidates": walls, "coldMaximumDegree": max(degree)}


def pack_seed(seed, half_extent, material):
    centers = list(occupied_centers(seed))
    assert len(centers) == seed["n"] and len(set(centers)) == seed["n"]
    assert seed["materialId"] == material["id"]
    r_um = seed["radiusUm"]
    assert all(abs(x) + r_um <= half_extent[0] and abs(y) + r_um <= half_extent[1]
               for x, y in centers)
    min_pair_sq = min((a[0]-b[0])**2 + (a[1]-b[1])**2 for a,b in combinations(centers, 2))
    assert min_pair_sq >= (2*r_um)**2
    radius = r_um / 1_000_000
    mass = seed["massUg"] / 1_000_000_000
    inertia = mass * radius**2 / 2
    records = []
    for i, (x,y) in enumerate(centers, 1):
        records.append(DISK.pack(i, 1, i, material["id"], x/1_000_000, y/1_000_000,
                                 0, 0, 0, 0, radius, mass, inertia, 1, 0, 0))
    # Verify actual FP32 payload geometry independently of the integer recipe.
    raw = b"".join(records)
    words = list(DISK.iter_unpack(raw))
    assert all(abs(x[4])+x[10] <= half_extent[0]/1_000_000 and
               abs(x[5])+x[10] <= half_extent[1]/1_000_000 for x in words)
    assert all(math.hypot(a[4]-b[4], a[5]-b[5]) >= a[10]+b[10]
               for a,b in combinations(words, 2))
    return raw, {
        "bytes": len(raw),
        "uploadSHA256": hashlib.sha256(raw).hexdigest(),
        "massUg": len(centers) * seed["massUg"],
        "areaMm2": round(len(centers) * math.pi * (r_um/1000)**2, 9),
        "comUm": [round(sum(c[axis] for c in centers)/len(centers), 9) for axis in (0,1)],
        "minWallClearanceUm": min(min(half_extent[0]-abs(x),half_extent[1]-abs(y))-r_um for x,y in centers),
        "minPairClearanceUm": round(math.sqrt(min_pair_sq)-2*r_um, 9),
        "heightFromBottomUm": max(y for _,y in centers) + r_um + half_extent[1],
        **initial_sweep(seed, half_extent, centers),
    }


def check_word_map(fields, words):
    widths = {"u32": 1, "u64le": 2, "f32": 1, "sha256Bytes": 8}
    occupied = set()
    for offset, name, kind in fields:
        used = set(range(offset, offset + widths[kind]))
        assert offset >= 0 and max(used) < words and not occupied.intersection(used), name
        occupied.update(used)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, help="write the reviewed seed streams to this directory")
    args = parser.parse_args()
    data = json.loads(ABI.read_bytes())
    buffers = data["bindings"]
    assert [b["binding"] for b in buffers] == list(range(4))
    assert sum(b["bytes"] for b in buffers) == data["logicalResidentBytes"] == 2_050_048
    for buffer in buffers:
        at = 0
        for name, offset, size in buffer["ranges"]:
            assert at == offset and size > 0 and size%16 == 0, name
            at += size
        assert at == buffer["bytes"]
    check_word_map(data["frozenConfig"]["config128"], 128)
    check_word_map(data["completion"]["fields"], 256)
    pose = data["pose"]["fixture"]
    assert hashlib.sha256((ROOT/pose["file"]).read_bytes()).hexdigest() == pose["sha256"]
    for receipt in data["isolatedReceipts"].values():
        assert hashlib.sha256(bytes.fromhex(receipt["inputPackedHex"])).hexdigest() == receipt["inputSHA"]
        output = (ROOT/receipt["output"]).read_bytes()
        assert hashlib.sha256(output).hexdigest() == receipt["outputSHA"]
        state, toi, *_ = struct.unpack("<I8f", output)
        assert (state,toi) == (receipt["status"],receipt["gpuTOI"])
    summary = {}
    for seed in data["seeds"]:
        raw, proof = pack_seed(seed, data["boxHalfExtentUm"], data["material"])
        assert proof == seed["proof"], (seed["id"], proof)
        if args.out:
            args.out.mkdir(parents=True, exist_ok=True)
            (args.out / f'{seed["id"]}.disk64').write_bytes(raw)
        summary[seed["id"]] = proof
    assert summary["G16-B13"]["massUg"] == summary["G16-B52"]["massUg"] == 130_000_000
    assert summary["G16-B13"]["areaMm2"] == summary["G16-B52"]["areaMm2"]
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
