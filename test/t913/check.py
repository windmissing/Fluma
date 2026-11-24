"""Check T913 compiled role contracts and deterministic ordering primitives."""
import argparse
import hashlib
import json
from pathlib import Path
import random


DM_ROLES = {
    "G9_OP_SOURCE_VALIDATE_INPUT", "G9_OP_SOURCE_ORIGIN_CERTIFICATE", "G9_OP_SOURCE_AUDIT_REPLAY",
    "G9_OP_SOURCE_COMPARE", "G9_OP_SOURCE_PACK_SCENE",
}
R1_ROLES = {"G9_OP_SOURCE_COMPARE", "G11_OP_VIS_PLAN_APPLY"}
CAP = 1 << 28


def manifests(base, candidate):
    a, b = (json.loads((folder / "roles.json").read_bytes()) for folder in (base, candidate))
    assert a["family"] == b["family"] and b["roleVariant"] == "t913-v1"
    assert a["roleSetSHA"] != b["roleSetSHA"] and all(b["complete"].values())
    for key in ("parentSHA", "constants", "commonBytes", "push", "passes", "masks", "graphics"):
        assert a[key] == b[key], key
    changes = set()
    for x, y in zip(a["roles"], b["roles"], strict=True):
        assert x["name"] == y["name"] and x["op"] == y["op"] and y["state"] == "STATIC_SPIRV"
        for folder, row in ((base, x), (candidate, y)):
            assert row["sourceBytes"] <= 49152
            assert hashlib.sha256((folder / row["spv"]).read_bytes()).hexdigest() == row["spvSHA"]
        if x["spvSHA"] != y["spvSHA"]:
            changes.add(y["name"])
    wanted = DM_ROLES if a["family"] == "dm2" else R1_ROLES
    assert changes == wanted, changes
    return list(sorted(changes))


def checks():
    random_gen = random.Random(913)
    for words in (0, 1, 63, 64, 65, 147968):
        changed = {random_gen.randrange(words) for _ in range(123)} if words else set()
        first = min((next((i for i in range(lane, words, 64) if i in changed), words) for lane in range(64)), default=words)
        assert first == min(changed, default=words)
    for _ in range(100):
        slots = [(random_gen.randrange(135), random_gen.choice((0, 1 << 20, CAP, CAP + 1))) for _ in range(192)]
        reduced = [0] * 128
        for col, mass in slots:
            if col < 128:
                reduced[col] = min(CAP + 1, reduced[col] + mass)
        assert reduced == [min(CAP + 1, sum(m for c, m in slots if c == col)) for col in range(128)]
    # Empty-row corruption stays in the sparse scan, in the same first-slot order.
    plan = [[0] * 8 for _ in range(65536)]
    for slot, word in ((31, 7), (32, 0), (32768, 0), (49152, 0), (65535, 5)):
        plan[slot][word] = 1
    sparse = []
    for start in range(0, len(plan), 32):
        mask = sum(1 << bit for bit, row in enumerate(plan[start:start+32]) if any(row))
        while mask:
            bit = (mask & -mask).bit_length() - 1
            sparse.append(start + bit)
            mask &= mask - 1
    assert sparse == [i for i, row in enumerate(plan) if any(row)]
    assert next(i for i in sparse if plan[i][0] == 0) == 31


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    checks()
    print(json.dumps({"changedRoles": manifests(args.base, args.candidate), "orderChecks": "PASS"}))
