"""T557 goal 10 only: finite arithmetic and evidence qualification."""

from fractions import Fraction
from itertools import product
from pathlib import Path
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[2]
D = ROOT / "docs"
R = ROOT / "test/reports"
N, Q = 128, 2**20
L = N * Q


def read(path):
    return json.loads(path.read_bytes())


def sha(data):
    return hashlib.sha256(data).hexdigest()


def mirror(frame):
    return {
        "gravity": [-frame["gravity"][0], frame["gravity"][1]],
        "walls": frame["walls"][::-1],
        "columns": [dict(row, p=-row["p"]) for row in reversed(frame["columns"])],
        "packets": [dict(row, x=L-1-row["x"], vx=-row["vx"]) for row in frame["packets"]],
        "commands": [dict(row, x=L-1-row["x"], jx=-row["jx"]) for row in frame["commands"]],
    }


def qualify(a, b):
    fields = ("profile", "algorithm", "parameters", "layout", "K", "coordinates",
              "startTick", "startSerial", "tick", "serial", "J")
    issues = [k for k in fields if a.get(k) is None or a.get(k) != b.get(k)]
    if not a.get("run") or not b.get("run") or a["run"] == b["run"]:
        issues.append("independentRun")
    if any(row.get("accepted") is not True for row in (a, b)):
        issues.append("acceptedProof")
    for k in ("seedSHA", "inputSHA"):
        if any(not row.get(k) for row in (a, b)):
            issues.append(k)
    if a.get("input") is None or b.get("input") is None or mirror(a["input"]) != b["input"]:
        issues.append("mirrorInput")
    return issues


def quotas(mass, amount):
    exact = [Fraction(amount * m, sum(mass)) for m in mass]
    counts = [int(x) for x in exact]
    order = sorted(range(len(mass)), key=lambda i: (counts[i]-exact[i], i))
    for i in order[:amount-sum(counts)]:
        counts[i] += 1
    return counts


def q_diff(a, b):
    residual = [abs(x+y) for x, y in zip(a, reversed(b), strict=True)]
    return dict(l1=sum(residual), max=max(residual), rawDenominator=sum(map(abs, a)))


def arithmetic():
    out = {}
    def save(case, result, expected, observed):
        out[case] = dict(result=result, expected=expected, observed=observed)

    columns = [dict(bed=[], origins={}, p=0) for _ in range(N)]
    columns[7] = dict(bed=[["A", 11], ["B", 7]], origins={"A": 5}, p=-3)
    columns[20] = dict(bed=[["A", 9]], origins={"A": 2, "B": 4}, p=6)
    packet = dict(x=7*Q+Q//2, vx=-4, vy=3, y2=71, id=9, source="B", mass=8)
    command = dict(x=7*Q+Q//2, jx=6, jy=-7, tick=4, mass=2, radius=Q, dedup="A:4", source="A")
    left = dict(gravity=[-5, 9], walls=["a", "b"], columns=columns, packets=[packet], commands=[command])
    right = mirror(left)
    assert right["columns"][120] == dict(columns[7], p=3)
    assert right["gravity"] == [5, 9] and right["walls"] == ["b", "a"] and mirror(right) == left
    samples = [c*Q+d for c in range(N) for d in (0, 1, Q//2-1, Q//2, Q-1)]
    assert all((L-1-x)//Q == N-1-x//Q and L-1-(L-1-x) == x for x in samples)
    assert all(N-e-1 == (N-2)-(e-1) for e in range(1, N))
    def mass(row):
        return sum(m for _, m in row["bed"]) + sum(row["origins"].values())
    def moment(frame):
        return sum((c*Q+Q//2)*mass(row) for c, row in enumerate(frame["columns"]))
    assert moment(left)+moment(right) == L*sum(map(mass, columns))
    assert packet["mass"]*(packet["x"]+right["packets"][0]["x"]) == (L-1)*8
    zeros = [0]*(N+1)
    assert q_diff(zeros, zeros) == dict(l1=0, max=0, rawDenominator=0)
    save("A01", "EXACT_IDENTITY", "supplied zero field keeps zero denominator", q_diff(zeros, zeros))
    a, b = zeros.copy(), zeros.copy()
    a[8], b[120] = 5, -5
    assert q_diff(a, b) == dict(l1=0, max=0, rawDenominator=5)
    save("A02", "EXACT_IDENTITY", "cell/face/signed field and distinct field/packet moments reflect",
         dict(positionSamples=len(samples), interiorFaces=N-1, faceQ=q_diff(a, b),
              fieldMoments=[moment(left), moment(right)]))
    shear = [str(Fraction(p, 12)**2) for p in (-5, 5)]
    assert shear == ["25/144"]*2
    save("A04", "EXACT_IDENTITY", "local u squared is even", shear)
    for case, m, q, expected, l1 in (
            ("A05", [1, 1], 1, [[1, 0], [0, 1]], 4),
            ("A06", [4, 4], 2, [[1, 1], [1, 1]], 0)):
        left_share = quotas(m, q)
        right_share = quotas([x-y for x, y in zip(m, left_share)], q)
        shares = [left_share, right_share]
        symmetric = [v for v in product(*(range(x//2+1) for x in m)) if sum(v) == q]
        residue = [abs(x-y) for f, g in zip(shares, shares[::-1]) for x, y in zip(f, g)]
        assert shares == expected and [sum(s) for s in shares] == [q, q]
        assert sum(residue) == l1 and bool(symmetric) == (l1 == 0)
        save(case, "KNOWN_INTEGER_TIE" if l1 else "EXACT_IDENTITY",
             dict(sharesEachRun=expected, totalQResidual=0, originL1=l1),
             dict(sharesEachRun=shares, totalQResidual=0, originL1=sum(residue),
                  originMax=max(residue), originDenominator=2*q, symmetricAllocations=symmetric))

    assert right["commands"][0] == dict(command, x=120*Q+Q//2-1, jx=-6)
    save("A07", "EXACT_IDENTITY", "strict capsule input keeps source key, tick and vertical component",
         right["commands"][0])
    capacity = {}
    for slots, expected in ((2, 0), (1, 6)):
        requests_l, requests_r = [20, 100], [27, 107]
        served_l, served_r = requests_l[:slots], requests_r[:slots]
        mapped = [N-1-c for c in served_l]
        error = 3*len(set(mapped) ^ set(served_r))
        assert error == expected
        capacity[str(slots)] = dict(servedL=served_l, servedR=served_r, mirrorL=mapped,
                                    deniedMassEach=3*(2-slots), packetMassMapL1=error)
    save("A08", "POLICY_COUNTEREXAMPLE", "equal requests, ample slots mirror; one slot favors smaller column", capacity)
    assert right["packets"][0] == dict(packet, x=120*Q+Q//2-1, vx=4)
    save("A09", "EXACT_IDENTITY", "strict mirror is one position quantum below independent center",
         dict(strictX=right["packets"][0]["x"], centerX=120*Q+Q//2, delta=-1))
    ties = [Fraction(n, 2) for n in (-5,-3,-1,1,3,5)]
    assert all(round(x) == -round(-x) for x in ties)
    x = Fraction(21, 2)
    diff = round(L-1-x) - (L-1-round(x))
    assert diff == -1
    save("A13", "KNOWN_INTEGER_TIE", "signed RNE is odd while an odd reflection intercept need not commute",
         dict(rounded=list(map(round, ties)), coordinateResidual=diff))
    def f32(x):
        return struct.unpack("<f", struct.pack("<f", x))[0]
    def decode(p):
        h, lo = divmod(p, 2**32)
        return f32(f32(h*2**32)+f32(lo))
    actual = {p: decode(p) for p in (-129, -128, -1, 0, 1, 128, 129)}
    assert list(actual.values()) == [-256, 0, 0, 0, 1, 128, 129]
    save("A14", "EXPRESSION_COUNTEREXAMPLE", "binary32 high/low-word addition need not preserve sign",
         dict(decoded=actual, imageResidual=None))

    basis = dict(profile="FIXTURE", algorithm="synthetic", parameters="synthetic", layout="synthetic",
                 K=192, coordinates="L-1-x", accepted=True, startTick=0, startSerial=0, tick=6, serial=2, J=3)
    def bind(run, frame):
        def encoded(value):
            return sha(json.dumps(value, sort_keys=True).encode())
        return dict(basis, run=run, input=frame,
                    seedSHA=encoded([frame["columns"], frame["packets"]]),
                    inputSHA=encoded([frame["gravity"], frame["commands"], frame["walls"]]))
    l, r = bind("L", left), bind("R", right)
    assert not qualify(l, r)
    controls = [("tick", 5, "tick"), ("serial", 9, "serial"), ("J", 1, "J"),
                ("accepted", False, "acceptedProof"), ("run", "L", "independentRun"),
                ("input", left, "mirrorInput"), ("profile", "OTHER", "profile"), ("seedSHA", None, "seedSHA")]
    rejections = {key: qualify(l, dict(r, **{key: value})) for key, value, _ in controls}
    assert rejections == {key: [reason] for key, _, reason in controls}
    save("A11", "COMPARISON_GATE", "qualify reflected input and same accepted interval before comparing",
         dict(rejections=rejections,
              pair=[{k: v for k, v in row.items() if k != "input"} for row in (l, r)]))
    return out

def existing_evidence():
    seen = []
    def record(path, family, excluded, **facts):
        seen.append(dict(path=path.relative_to(ROOT).as_posix(), sha256=sha(path.read_bytes()),
                         family=family, qualifies=False, exclusion=excluded, **facts))

    path = R/"t670/fixtures.json"
    profiles = read(path)
    record(path, "G9_R1_AND_DM_V2", "INPUT_FIXTURES_ONLY",
           inputs=[dict(family=p["family"], K=p["K"], angleIndex=p["angleIndex"],
                        seedSHA=p["files"]["seed.u32"]["sha256"]) for p in profiles if p.get("schema") == "T670_EXISTING_FORMAL_SEED_V1"])
    path = R/"t491/t712b/v4-t718.json"
    record(path, "G9_DM_V2", "CREATE_PENDING", observed=read(path)["execution"]["createResult"])
    dm_arms = sorted((R/"t491/t809").glob("x-*.jsonl"))
    assert {p.stem for p in dm_arms} == {"x-B","x-op8","x-op9","x-copies","x-op11"}
    for path in dm_arms:
        rows = [json.loads(line) for line in path.read_bytes().splitlines() if line]
        root = next(x for x in rows if x.get("stage") == "SOURCE_ROOT")
        terminal = next(x for x in rows if x.get("stage") == "SOURCE_TERMINAL")
        assert not root["committed"] and str(terminal["commit"]) == str(terminal["quanta"]) == "0"
        record(path, "G9_DM_V2", "TIMING_OR_PREFIX_WITHOUT_ACCEPT",
               root={k: root[k] for k in ("pid","scope","inputSHA","seedSHA","layoutSHA","parameterSHA","sourceSHA")},
               terminal={k: terminal[k] for k in ("exitCode","submitted","commit","quanta")})
    for task in ("t802", "t816"):
        path = R/f"t491/{task}/sidecar-terminal.json"
        status = read(path)["status"]
        source = status["source"]
        assert not source["committed"]
        record(path, "RX_SOURCE", "UNCOMMITTED_SOURCE_AND_NO_PROVEN_MIRROR",
               pid=status["pid"], processStartTick=status["processStartTick"],
               facts={k: source[k] for k in ("sourceSHA","parameterSHA","seedSHA","inputSHA","fault","committed")})
    path = R/"t491/t711b/v2-fourarm.json"
    record(path, "M1_M2", "FEATURE_SWITCH_ARMS_NO_MIRROR", arms=list(read(path)["arms"]))
    path = R/"t491/t711b/m1to.jpeg"
    record(path, "M1_IMAGE", "SINGLE_ARCHIVED_FRAME_NO_MIRROR_SIDECAR")
    path = R/"t485/pixels.json"
    pixels = read(path)
    assert pixels["device"] == "NOT_RUN"
    record(path, "M1_M2_IMAGE", "COLD_WARM_OFF_COMPARISON_NO_MIRROR",
           modes=sorted({run["mode"] for run in pixels["runs"]}))
    path = R/"t722/fixture.json"
    assert read(path)["fixtureOnly"]
    record(path, "GOAL_11_ARCHIVE", "OUT_OF_SCOPE_FIXTURE_ONLY")
    return seen


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    source_evidence = read(D/"design-T557-T558-evidence-v1.json")
    fixtures = read(D/"design-T557-T558-fixtures-v1.json")
    assert fixtures["final_numeric_tolerances"] is None
    assert (fixtures["coordinates"]["N"], fixtures["coordinates"]["Q"], fixtures["coordinates"]["L"]) == (N,Q,L)
    inputs = []
    for name in ("algorithm-G10-anisotropy-analysis-v1.md", "retro-T557-T558-v1.md",
                 "design-T557-T558-checks-v1.py", "design-T557-T558-fixtures-v1.json"):
        raw = (D/name).read_bytes()
        raw_hash = sha(raw)
        frozen = source_evidence["sha256"][name]
        assert frozen in (raw_hash, sha(raw.replace(b"\r\n", b"\n"))), name
        inputs.append(dict(file=name, checkedOutSHA256=raw_hash, frozenSHA256=frozen,
                           match="RAW" if raw_hash == frozen else "LF_CHECKOUT"))
    local = arithmetic()
    cases = []
    missing = {
        "A01":"matched accepted REST and raw mass/fields", "A02":"mirrored slope/gx roots and actual signed q/p",
        "A03":"separate route-matched tilt roots", "A04":"actual predictor/corrector q and capacity",
        "A05":"accepted per-face origin shares", "A06":"accepted per-face origin shares",
        "A07":"raw UI event/tick association and resulting impulse",
        "A08":"actual launch/denied mass rows and independent ID bijection",
        "A09":"wall and no-wall contact roots and captures", "A10":"same-profile 64/65-origin WAIT and inventories",
        "A11":"long equal accepted interval and per-J raw q", "A12":"paired M1 full-cycle/phase fields",
        "A13":"M2 merge and subsequent contact originals", "A14":"same-token mirrored viewports and real pixels",
        "A15":"paired repeat/stale/reject/cancel scene and accepted receipts",
    }
    for case in (c for c in fixtures["cases"] if c["id"].startswith("A")):
        name = case["id"]
        cases.append(dict(id=name, designExpectation=case["expect"], finite=local.get(name),
                          runtime="NOT_OBSERVED", missing=missing[name]))
    assert [c["id"] for c in cases] == [f"A{i:02d}" for i in range(1,16)]
    sources = []
    for name in ("surface_flux.glsl", "origin_map.glsl", "touch_launch.glsl", "scene.frag"):
        path = ROOT/"entry/src/main/cpp/gpu_resident/g9spf"/name
        sources.append(dict(path=path.relative_to(ROOT).as_posix(),
                            sha256LF=sha(path.read_bytes().replace(b"\r\n", b"\n"))))
    candidates = existing_evidence()
    result = dict(schema="T814_G10_MIRROR_OFFLINE_V1", device="NOT_RUN", product="NOT_OBSERVED",
                  eligiblePairs=0, observedTotalQ=None, observedOriginQ=None, observedImage=None,
                  inputs=inputs, sourceReferences=sources, candidates=candidates, cases=cases)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=False)+"\n", encoding="utf-8")
    print(f"T814: {len(cases)} A cases, {len(local)} finite checks, {len(candidates)} candidates, 0 eligible pairs")


if __name__ == "__main__":
    main()
