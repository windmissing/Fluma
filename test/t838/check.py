"""T827 PC exact mirror input and independent S/F/P, Q, origin and pixel comparisons."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]


def local_module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


T814 = local_module("t814_arithmetic", "test/t814/check.py")
G9 = local_module("g9_cold", "tools/g9rx/read_cold.py")
N = T814.N


def demand(value, text):
    if not value:
        raise ValueError(text)


def input_sha(value):
    return T814.sha(json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode())


def mirror_inputs(left):
    demand(len(left["columns"]) == N and len(left["walls"]) == len(left["gravity"]) == 2, "layout")
    for row in left["packets"]+left["commands"]:
        demand(type(row["x"]) is int and 0 <= row["x"] < T814.L, "input outside Q20 lattice")
    right = T814.mirror(left)
    return dict(A=left, B=right, A_SHA=input_sha(left), B_SHA=input_sha(right))


def optional_pair(a, b, key, transform):
    return None if a.get(key) is None or b.get(key) is None else transform(a[key], b[key])


def magnitudes(a, b, size, signed=False):
    demand(len(a) == len(b) == size and all(type(i) is int for i in a+b), "integer array extent")
    if signed:
        return T814.q_diff(a, b)
    demand(all(i >= 0 for i in a+b), "negative inventory")
    errors = [abs(x-y) for x, y in zip(a, reversed(b), strict=True)]
    return dict(l1=sum(errors), max=max(errors), rawDenominator=sum(a), totalA=sum(a), totalB=sum(b))


def phase(a, b):
    return {p: magnitudes(a[p], b[p], N) for p in ("S", "F", "P")}


def origins(a, b):
    demand(set(a) == set(b) and bool(a), "origin labels differ")
    return {key: magnitudes(a[key], b[key], N) for key in sorted(a)}


def pixels(a, b):
    meta = ("format", "width", "height", "optics", "backend")
    demand(all(a[k] == b[k] and a[k] is not None for k in meta) and a["format"] == "RGBA8", "pixel identity")
    w, h = a["width"], a["height"]
    demand(type(w) is type(h) is int and 0 < w*h <= 3840*2160 and min(w, h) > 0, "viewport extent")
    left, right = bytes.fromhex(a["pixels"]), bytes.fromhex(b["pixels"])
    demand(len(left) == len(right) == w*h*4 and
           T814.sha(left) == a["imageSHA"] and T814.sha(right) == b["imageSHA"], "pixel payload")
    differing, maximum, l1 = 0, 0, 0
    for y in range(h):
        for x in range(w):
            i, j = (y*w+x)*4, (y*w+w-1-x)*4
            errors = [abs(v-u) for v, u in zip(left[i:i+4], right[j:j+4], strict=True)]
            differing += any(errors)
            l1 += sum(errors)
            maximum = max(maximum, *errors)
    return dict(pixels=w*h, different=differing, max=maximum, l1=l1)


def accepted(row):
    def wide(label):
        value = row[label]
        demand(isinstance(value, str) and value.isascii() and value.isdecimal() and
               str(int(value)) == value and int(value) < 2**64, label)
        return int(value)
    demand(wide("startTick") <= wide("tick") and wide("startSerial") <= wide("serial"), "accepted interval")
    demand(row.get("acceptedRoot") and row.get("fence") == 0 and row["accepted"] is True, "native acceptance")
    for name in ("phases", "faceQ", "originQ", "image"):
        metric = row.get(name)
        if metric is not None:
            demand(metric["acceptedRoot"] == row["acceptedRoot"] and metric["geometry"] == row["geometry"],
                   "metric token/generation " + name)


def payload(row, key):
    item = row.get(key)
    return None if item is None else item["value"]


def judge(left, right, directory, synthetic=False):
    try:
        demand(left.get("synthetic") is right.get("synthetic") is synthetic, "evidence mode")
        for row in (left, right):
            accepted(row)
            demand(row["inputSHA"] == input_sha(row["input"]), "canonical input SHA")
        mirrored = mirror_inputs(left["input"])
        issues = T814.qualify(left, right)
        if issues or right["input"] != mirrored["B"] or left["acceptedRoot"] == right["acceptedRoot"]:
            return dict(state="UNPAIRED", issues=issues or ["mirror or independent token"])
        archives = []
        if not synthetic:
            for row in (left, right):
                arc = row.get("archive")
                if not arc:
                    return dict(state="UNOBSERVED", eligiblePairs=0, missing=["original cold archive"])
                decoded = G9.decode(directory/arc, directory/row["archiveSourceManifest"] if row.get("archiveSourceManifest") else None)
                demand(decoded["archiveSHA"] == row["archiveSHA"] and decoded["acceptedStreamReplay"],
                       "archive SHA/replay")
                archives.append(decoded["archiveSHA"])
            demand(archives[0] != archives[1], "same archive used twice")
        metrics = {}
        for key, transform in (("phases", phase), ("faceQ", lambda a,b: magnitudes(a,b,N+1,True)),
                               ("originQ", origins), ("image", pixels)):
            metrics[key] = optional_pair({key: payload(left,key)}, {key: payload(right,key)}, key, transform)
        phases = metrics["phases"]
        metrics["totalQ"] = (None if phases is None else
                             (sum(p["totalB"] for p in phases.values()) - sum(p["totalA"] for p in phases.values())))
        return dict(state="SYNTHETIC_PASS" if synthetic else "METRICS_COMPUTED",
                    eligiblePairs=0 if synthetic else 1, archiveSHA=archives, **metrics)
    except (KeyError, IndexError, OSError, TypeError, ValueError) as error:
        return dict(state="INVALID", error=str(error), eligiblePairs=0)


def selftest():
    source = dict(gravity=[1, -1], walls=["left", "right"], columns=[dict(p=0) for _ in range(N)],
                  packets=[dict(x=0,vx=1)], commands=[dict(x=1,jx=0)])
    inputs = mirror_inputs(source)
    first, last = [9]+[0]*(N-1), [0]*(N-1)+[9]
    common = dict(profile="fixture", algorithm="a", parameters="p", layout="l", coordinates="Q20", K=192,
                  startTick="0", startSerial="0", tick="9007199254740993", serial="1", J=1,
                  accepted=True, seedSHA="fixture", geometry="1", fence=0, synthetic=True)
    sides = []
    for side, cells, flow, px in (("A", first, [1]+[0]*N, b"12345678"),
                                 ("B", last, [0]*N+[-1], b"56781234")):
        frame = dict(common, run=side, acceptedRoot=side, input=inputs[side], inputSHA=inputs[side+"_SHA"])
        for name, values in (("phases", dict(S=cells,F=cells,P=cells)), ("faceQ", flow),
                             ("originQ", {"touch":cells}), ("image", dict(format="RGBA8", width=2, height=1,
                              optics="fixture", backend="fixture", pixels=px.hex(), imageSHA=T814.sha(px)))):
            frame[name] = dict(acceptedRoot=side, geometry="1", value=values)
        sides.append(frame)
    a, b = sides
    good = judge(a, b, ROOT, True)
    assert good["totalQ"] == good["faceQ"]["l1"] == good["image"]["different"] == 0
    assert judge(a, b, ROOT)["state"] == "INVALID"
    assert judge(a, dict(b, tick="9007199254740992"), ROOT, True)["state"] == "UNPAIRED"
    assert judge(a, dict(b, image=None), ROOT, True)["image"] is None
    wrong = copy.deepcopy(b)
    wrong["phases"]["geometry"] = "2"
    assert judge(a, wrong, ROOT, True)["state"] == "INVALID"
    historical = T814.arithmetic()
    return dict(state="SYNTHETIC_PASS", checks=5, historicalArithmeticReplayed=historical is not None)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    cmd = parser.add_subparsers(dest="cmd", required=True)
    cmd.add_parser("selftest")
    emit = cmd.add_parser("emit")
    emit.add_argument("input", type=Path)
    check = cmd.add_parser("check")
    check.add_argument("pair", type=Path)
    args = parser.parse_args()
    if args.cmd == "selftest":
        result = selftest()
    elif args.cmd == "emit":
        result = mirror_inputs(T814.read(args.input))
    else:
        pair = T814.read(args.pair)
        result = judge(pair["A"], pair["B"], args.pair.parent)
    print(json.dumps(result, indent=2))
    sys.exit(int(result.get("state") in ("INVALID","UNPAIRED","UNOBSERVED")))
