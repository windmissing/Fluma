"""PC pointer and gravity fixture. A driver consumes the emitted relative timeline."""
import argparse
import copy
import json
import sys
from pathlib import Path
from time import monotonic, sleep


TIMELINE = (
    (0, "EMPTY", 1, []),
    (50, "DOWN", 1, [320, 120]),
    (150, "MOVE", 1, [340, 120]),
    (350, "UP", 1, [340, 120]),
    (500, "RESIZE", 2, [800, 500]),
    (550, "STALE_DOWN", 1, [340, 120]),
    (600, "GRAVITY", 2, [0, -1]),
    (850, "GRAVITY", 2, [1, 0]),
)


def timeline():
    return [dict(ms=ms, kind=kind, generation=gen, args=args)
            for ms, kind, gen, args in TIMELINE]


def judge(e, synthetic=False):
    if e.get("synthetic") is not synthetic:
        return dict(state="INVALID", errors=["evidence mode"])
    needed = ("frequency", "startQpc", "width", "height", "actions", "roots", "frames")
    if any(k not in e for k in needed):
        return dict(state="MISSING_EVIDENCE", missing=[k for k in needed if k not in e])
    errors, missing = [], []
    frequency, start = e["frequency"], int(e["startQpc"])
    if frequency <= 0 or e["width"] < 341 or e["height"] < 121:
        return dict(state="INVALID", errors=["clock or viewport"])
    actions = e["actions"]
    if len(actions) != len(TIMELINE):
        return dict(state="MISSING_EVIDENCE", missing=["complete action receipt"])
    ticks = [int(a["qpc"]) for a in actions]
    first, second = actions[0]["geometry"], actions[4]["geometry"]
    if not first or not second or first == second or ticks != sorted(set(ticks)):
        errors.append("generation or monotonic action clock")
    for a, tick, (ms, kind, gen, args) in zip(actions, ticks, TIMELINE, strict=True):
        nominal = start + ms * frequency // 1000
        if (a["kind"], a["geometry"], a["args"]) != (kind, first if gen == 1 else second, args):
            errors.append(f"{kind}: wrong input or generation")
        if tick < nominal:
            errors.append(f"{kind}: earlier than scheduled")
    if actions[5].get("admitted") is not False:
        errors.append("stale generation was not rejected")

    roots = {}
    for root in e["roots"]:
        token = root.get("acceptedRoot")
        if not token or root.get("accepted") is not True or token in roots:
            errors.append("missing, duplicate or rejected root")
            continue
        roots[token] = root
        tick, mass = int(root["qpc"]), root["sourceQ"]
        if type(mass) is not int or mass < 0 or root.get("geometry") != (first if tick < ticks[4] else second):
            errors.append("source or root generation")
        if mass and not (ticks[1] <= int(root["inputQpc"]) < ticks[3]
                         and int(root["inputQpc"]) <= tick < ticks[4]):
            errors.append("source outside accepted held input")

    seen = set()
    empty, dual, released = False, False, False
    for frame in e["frames"]:
        token = frame.get("acceptedRoot")
        if not token:
            continue  # A diagnostic frame is never product evidence.
        root = roots.get(token)
        tick = int(frame["presentQpc"])
        if (root is None or root["geometry"] != frame["geometry"] or tick < int(root["qpc"])
                or frame["geometry"] != (first if tick < ticks[4] else second)
                or frame["present"] not in ("VK_SUCCESS", "VK_SUBOPTIMAL_KHR")):
            errors.append("unmatched, stale or failed product Present")
            continue
        seen.add(token)
        empty |= tick < ticks[1] and all(frame.get(k) == 0 for k in ("S", "F", "P"))
        dual |= all(type(frame.get(k)) is int and frame[k] > 0 for k in ("bedPixels", "particlePixels"))
        released |= tick >= ticks[3] and root["sourceQ"] == 0

    for present, label in ((empty, "empty product frame"), (dual, "bed + particle pixels in same frame"),
                           (released, "released product frame"),
                           (sum(r["sourceQ"] > 0 and t in seen for t, r in roots.items()) >= 2,
                            "continuous held source roots")):
        if not present:
            missing.append(label)
    for index in (6, 7):
        if not any(t in seen and int(r["qpc"]) >= ticks[index] and
                   r.get("gravity") == actions[index]["args"] and
                   r.get("gravityInputQpc") == actions[index]["qpc"] for t, r in roots.items()):
            missing.append(f"accepted gravity sample {index - 5}")
    state = "INVALID" if errors else "MISSING_EVIDENCE" if missing else "SYNTHETIC_PASS" if synthetic else "PASS"
    return dict(state=state, errors=errors, missing=missing)


def read(path):
    return json.loads(path.read_bytes())


def selftest():
    sample = dict(synthetic=True, startQpc="1000", frequency=1000, width=640, height=480,
                  actions=[dict(a, qpc=str(1000+a["ms"]), geometry=str(a["generation"]),
                                admitted=a["kind"] != "STALE_DOWN") for a in timeline()], roots=[], frames=[])
    for tick, mass, gen, gravity_tick, gravity in (
            (1001, 0, "1", None, None), (1100, 7, "1", None, None),
            (1220, 7, "1", None, None), (1400, 0, "1", None, None),
            (1610, 0, "2", "1600", [0, -1]), (1860, 0, "2", "1850", [1, 0])):
        token = f"root-{tick}"
        sample["roots"].append(dict(accepted=True, acceptedRoot=token, qpc=str(tick), geometry=gen,
                                    sourceQ=mass, inputQpc="1050", gravityInputQpc=gravity_tick, gravity=gravity))
        sample["frames"].append(dict(acceptedRoot=token, presentQpc=str(tick+1), geometry=gen,
                                     present="VK_SUCCESS", S=mass, F=0, P=mass,
                                     bedPixels=mass, particlePixels=mass))
    assert judge(sample, True)["state"] == "SYNTHETIC_PASS"
    assert judge(sample)["state"] == "INVALID"
    for data, index, field, value in (("actions", 5, "admitted", True),
                                     ("roots", 1, "inputQpc", "1350"),
                                     ("frames", 1, "geometry", "2"),
                                     ("frames", 2, "acceptedRoot", "unknown")):
        bad = copy.deepcopy(sample)
        bad[data][index][field] = value
        assert judge(bad, True)["state"] == "INVALID"
    bad = copy.deepcopy(sample)
    bad["frames"].clear()
    assert judge(bad, True)["state"] == "MISSING_EVIDENCE"
    return dict(state="SYNTHETIC_PASS", checks=7)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    cmd = parser.add_subparsers(dest="cmd", required=True)
    cmd.add_parser("emit")
    cmd.add_parser("drive", help="stream timed actions as JSONL to the PC test input adapter")
    cmd.add_parser("selftest")
    check = cmd.add_parser("check")
    check.add_argument("receipt", type=Path)
    args = parser.parse_args()
    if args.cmd == "drive":
        start = monotonic()
        for action in timeline():
            sleep(max(0, start + action["ms"]/1000 - monotonic()))
            print(json.dumps(action), flush=True)
        return 0
    try:
        result = timeline() if args.cmd == "emit" else selftest() if args.cmd == "selftest" else judge(read(args.receipt))
    except (ValueError, TypeError, KeyError, OSError) as err:
        result = dict(state="INVALID", errors=[str(err)])
    print(json.dumps(result, indent=2))
    return int(isinstance(result, dict) and result["state"] not in ("PASS", "SYNTHETIC_PASS"))


if __name__ == "__main__":
    sys.exit(main())
