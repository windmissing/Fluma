"""Independent PC evidence gate for the T828 pure fluid cases."""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/g13"))
from read_archive import exact_id, read


def load_module(path):
    spec = importlib.util.spec_from_file_location("t721_checker", ROOT/path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


T721 = load_module("test/t721/check.py")
CASES = {case["id"]: case for case in T721.load(ROOT/"docs/design-G13-pure-fluid-fixtures-v1.json")["cases"]}
LAYOUT = T721.load(ROOT/"docs/architecture-T607-g13-layout-v1.json")
MAGIC = LAYOUT["magic"]
SLOTS = ("viscosityRMS", "viscosityMax", "divergenceRMS", "divergenceMax",
         "pressureCertificate", "projectionVelocityCertificate")
PHYSICAL = {"F01", "F03", "F04", "F05", "F06", "F07", "F08", "F09", "F10", "F12", "F15", "F16"}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def enforce(condition, error):
    if not condition:
        raise ValueError(error)


def float_word(word):
    return struct.unpack("<f", struct.pack("<I", word))[0]


def identity(words, offset):
    return words[offset] | words[offset+1] << 32


def tail_from_archive(path, receipt):
    result = read(path, receipt)
    enforce(result["state"] == "STRUCTURALLY_COMPLETE", "forensic/partial archive")
    # Validate the exact file SHA again while extracting bounded windows.
    checksum = hashlib.sha256()
    config, tail = bytearray(), bytearray()
    with path.open("rb") as source:
        first = source.readline(16385)
        header = json.loads(first)["words"]
        enforce(header[4] == 2, "FULL capture required for GridView")
        profile = next(p for p in LAYOUT["profiles"] if (p["Nx"], p["Ny"]) == tuple(header[96:98]))
        ctrl = next(d["byteOffset"] for d in profile["FULLColdDirectory"] if d["name"] == "CTRL")
        candidate = next(d["byteOffset"] for d in profile["FULLColdDirectory"] if d["name"] == "CANDIDATE")
        raw_fields = {f["name"]: bytearray() for f in profile["state"] if f["name"] in ("u","v","piQ32")}
        spans = [(ctrl,512,config), (ctrl+73728,1024,tail)]
        spans.extend((candidate+f["byteStart"], f["bytes"], raw_fields[f["name"]])
                     for f in profile["state"] if f["name"] in raw_fields)
        checksum.update(first)
        for line in source:
            checksum.update(line)
            page = json.loads(line)
            if page.get("state") != "PAGE":
                continue
            at = exact_id(page["rawByteOffset"])
            data = struct.pack(f"<{len(page['words'])}I", *page["words"])
            for start, size, target in spans:
                lo, hi = max(start, at), min(start+size, at+len(data))
                if lo < hi:
                    target.extend(data[lo-at:hi-at])
    enforce(checksum.hexdigest() == result["fileSHA256"], "archive changed")
    enforce(all(len(target) == size for _, size, target in spans), "archive field range")
    fields = {name: [item[0] for item in struct.iter_unpack("<q" if name == "piQ32" else "<I", values)]
              for name, values in raw_fields.items()}
    return bytes(config), bytes(tail), result["fileSHA256"], fields


def units(config, view):
    l, rho, mu = struct.unpack_from("<3d", config, 48)
    t, pref = view["T0"], view["pRef"]
    enforce(all(math.isfinite(v) and v > 0 for v in (l, rho, mu, t)) and math.isfinite(pref), "units domain")
    nu = mu*t/(rho*l*l)
    enforce(struct.pack("<f", nu) == config[72:76], "Config128 viscosity/unit mismatch")
    enforce(view["pressureUnit"] == "Pa" and view["velocityUnit"] == "m/s", "units labels")
    return nu, l/t, rho*(l/t)**2


def field_view(view, config):
    nx, ny = struct.unpack_from("<2I", config, 16)
    enforce(view["shape"] == [nx, ny] and 0 < nx*ny <= 128*128, "GridView grid")
    values = {}
    for label, size in (("u32", (nx+1)*ny), ("v32", nx*(ny+1)), ("piQ32", nx*ny), ("phi", nx*ny)):
        raw = view[label]
        enforce(len(raw) == size, "GridView extent " + label)
        if label in ("u32", "v32"):
            enforce(all(type(v) is int and 0 <= v < 2**32 for v in raw), "invalid velocity word")
            raw = [float_word(v) for v in raw]
        if label == "piQ32":
            enforce(all(type(v) is int and -(2**63) <= v < 2**63 for v in raw), "invalid Q32")
            raw = [v / 2**32 for v in raw]
        enforce(all(math.isfinite(v) for v in raw), "nonfinite GridView " + label)
        values[label] = raw
    nu, uscale, pscale = units(config, view)
    phi_mean = math.fsum(values["phi"]) / (nx*ny)
    pressure = [(p + phi - phi_mean + view["pRef"])*pscale
                for p, phi in zip(values["piQ32"], values["phi"], strict=True)]
    enforce(all(map(math.isfinite, pressure)), "nonfinite SI pressure")
    summary = dict(nu=nu, velocityScale=uscale, pressureScale=pscale,
                   maxVelocitySI=max(map(abs, values["u32"]+values["v32"]))*uscale,
                   pressureRangeSI=[min(pressure), max(pressure)])
    return values, summary


def relative_errors(observed, expected):
    residuals = [v-w for v,w in zip(observed, expected, strict=True)]
    peak = max(map(abs, expected))
    square = math.fsum(x*x for x in expected)
    enforce(peak > 0 and square > 0, "zero reference normalization")
    return (math.sqrt(math.fsum(x*x for x in residuals)/square),
            max(map(abs,residuals))/peak)


def field_reference(case, fields, view, nu, accepted_quantum):
    nx, ny = view["shape"]
    if case == "F10":
        enforce(all(w == .25 for w in fields["u32"]) and all(w == -.125 for w in fields["v32"])
                and not any(fields["piQ32"]), "F10 uniform periodic field")
        return dict(uniformField=True)
    if case not in ("F04", "F05", "F07") or nx != ny:
        return {}
    # T605/T631 normalizes the reference on the fixed unit square. Restrict
    # these comparisons to the frozen fixtures and their terminal time.
    if (nx not in (32, 64, 128) or accepted_quantum != 30 or nu != 1/64):
        return {}
    h = 1/nx
    if case == "F07":
        def expected(y):
            return y*(1-y)
        max_error = max(abs(u-expected((j+.5)*h)) for j in range(ny)
                        for u in fields["u32"][j*(nx+1):(j+1)*(nx+1)])
        flux = h*math.fsum(fields["u32"][j*(nx+1)+nx//2] for j in range(ny))
        return dict(velocityMaxError=max_error, flux=flux,
                    velocityPass=max_error <= h*h/4+2**-12, fluxPass=abs(flux-1/6)/(1/6) <= 1/32)
    sin, cos, exp = math.sin, math.cos, math.exp
    if case == "F04":
        k = 2*math.pi
        a = .25*exp(-2*nu*k*k*.25)
        def u_ref(x,y):
            return a*sin(k*x)*cos(k*y)
        def v_ref(x,y):
            return -a*cos(k*x)*sin(k*y)
        def p_ref(x,y):
            return a*a/4*(cos(2*k*x)+cos(2*k*y))
    else:
        k = math.pi
        a = .25*exp(-.25)
        def u_ref(x,y):
            return a*sin(k*x)**2*sin(2*k*y)
        def v_ref(x,y):
            return -a*sin(2*k*x)*sin(k*y)**2
        def p_ref(x,y):
            return .125*exp(-.5)*cos(k*x)*cos(k*y)

    # Remove the duplicate periodic face only in the declared periodic case.
    periodic = case == "F04"
    u = [fields["u32"][j*(nx+1)+i] for j in range(ny) for i in range(nx+int(not periodic))]
    v = fields["v32"][:nx*(ny+int(not periodic))]
    expected_u = [u_ref(i*h,(j+.5)*h) for j in range(ny) for i in range(nx+int(not periodic))]
    expected_v = [v_ref((i+.5)*h,j*h) for j in range(ny+int(not periodic)) for i in range(nx)]
    expected_p = [p_ref((i+.5)*h,(j+.5)*h) for j in range(ny) for i in range(nx)]
    vrms, vmax = relative_errors(u+v, expected_u+expected_v)
    prms, _ = relative_errors(fields["piQ32"], expected_p)
    return dict(velocityRelRMS=vrms, velocityRelMax=vmax, pressureRelRMS=prms,
                finestThresholdsSatisfied=(vrms <= 1/32 and vmax <= 1/16 and prms <= 1/16) if nx == 128 else None)


def inspect(e, config, raw, synthetic=False):
    enforce(e.get("synthetic") is synthetic, "evidence mode")
    case = e["case"]
    enforce(case in CASES, "fixture identity")
    if not e.get("acceptedRoot"):
        return dict(state="MISSING_EVIDENCE", missing=["nonempty accepted root"])
    enforce(case in PHYSICAL, "diagnostic case cannot consume physical publication")
    enforce(len(raw) == 1024 and sha(raw) == e["completionSHA"], "Completion256 SHA/size")
    tail = struct.unpack("<256I", raw)
    enforce(tail[:4] == (MAGIC["tail"], 1, MAGIC["profile"], 256) and tail[255] == 0xc0de1301,
            "Completion256 header/seal")
    enforce(tail[4:6] == (2, 1) and tail[7] == 0 and tail[42:44] == (1, 3) and
            tail[39] == 820 and (tail[37] & tail[38]) == tail[37], "QUANTUM acceptance/audit")
    for name, offset in (("nonce",8), ("plan",10), ("owner",14), ("context",16),
                         ("surface",18), ("geometry",20), ("configGeneration",22),
                         ("commit",26), ("acceptedQuantum",34)):
        value = exact_id(e[name])
        enforce(value != 0 and value == identity(tail, offset), "completion identity " + name)
    enforce(identity(tail, 28) <= identity(tail, 32) <= identity(tail, 30)
            <= identity(tail, 28)+250_000_000, "completion deadline")
    probes = e["op66"]
    roles = e["roles"]
    enforce(isinstance(roles, dict) and roles and set(roles) == {p["role"] for p in probes}
            and len(roles) == len(probes), "OP66 role coverage")
    for row in probes:
        enforce(row["probeWords"] == T721.EXPECTED*2 and row["mismatchMasks"] == [0,0] and
                row["queueSubmit"] == row["wait"] == 0 and row["moduleSHA"] == roles[row["role"]],
                "same-role OP66 words or submission")
    view = e["view"]
    enforce(view["acceptedRoot"] == e["acceptedRoot"] and
            all(view[k] == e[k] for k in ("geometry", "commit", "nonce")), "GridView generation")
    numeric = T721.numerical_gates(tail[36], dict(zip(SLOTS, map(float_word, tail[104:110]), strict=True)))
    if numeric != "THRESHOLDS_SATISFIED":
        return dict(state=numeric)
    fields, result = field_view(view, config)
    if case in ("F01", "F09"):
        enforce(not any(view["u32"] + view["v32"] + view["piQ32"]), "sealed rest exact zero words")
    reference = field_reference(case, fields, view, result["nu"], identity(tail,34))
    return dict(state="SYNTHETIC_PASS" if synthetic else "CASE_REFERENCE_PENDING", numerical=numeric,
                units=result, reference=reference, expected=CASES[case]["expected"])


def selftest():
    config = bytearray(512)
    struct.pack_into("<2I", config, 16, 2, 2)
    struct.pack_into("<3df", config, 48, 2, 1000, 4, .001)
    words = [0]*256
    words[:6] = [MAGIC["tail"], 1, MAGIC["profile"], 256, 2, 1]
    words[36:44] = [1, 7, 7, 820, 820, 0, 1, 3]
    words[255] = 0xc0de1301
    e = dict(synthetic=True, case="F01", acceptedRoot="fixture")
    for name, offset in (("nonce",8), ("plan",10), ("owner",14), ("context",16), ("surface",18),
                         ("geometry",20), ("configGeneration",22), ("commit",26), ("acceptedQuantum",34)):
        e[name] = str(offset)
        words[offset] = offset
    raw = struct.pack("<256I", *words)
    e.update(completionSHA=sha(raw), roles={"test":"hash"},
             op66=[dict(role="test", moduleSHA="hash", probeWords=T721.EXPECTED*2, mismatchMasks=[0,0],
                        queueSubmit=0, wait=0)],
             view=dict(acceptedRoot="fixture", geometry=e["geometry"], commit=e["commit"], nonce=e["nonce"],
                       shape=[2,2], T0=1, pRef=0, pressureUnit="Pa", velocityUnit="m/s",
                       u32=[0]*6, v32=[0]*6, piQ32=[0]*4, phi=[0]*4))
    assert inspect(e, config, raw, True)["state"] == "SYNTHETIC_PASS"
    assert inspect(dict(e, acceptedRoot=None), config, raw, True)["state"] == "MISSING_EVIDENCE"
    for change in (dict(geometry="77"), dict(view=dict(e["view"], T0=2)), dict(synthetic=False)):
        try:
            inspect(dict(e, **change), config, raw, True)
        except ValueError:
            continue
        raise AssertionError(change)
    n = 32
    poisson = [(y+.5)/n*(1-(y+.5)/n) for y in range(n) for _ in range(n+1)]
    result = field_reference("F07", dict(u32=poisson), dict(shape=[n,n]), 1/64, 30)
    assert result["velocityPass"] and result["fluxPass"]
    return dict(state="SYNTHETIC_PASS", checks=6, cases=sorted(CASES))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="cmd", required=True)
    commands.add_parser("selftest")
    commands.add_parser("cases")
    check = commands.add_parser("check")
    for name in ("manifest", "archive", "receipt"):
        check.add_argument(name, type=Path)
    args = parser.parse_args()
    try:
        if args.cmd == "selftest":
            result = selftest()
        elif args.cmd == "cases":
            result = dict(state="SPEC_ONLY", cases=CASES,
                          revisions=T721.load(ROOT/"test/t606/coverage-table.json")["rulings_applied"])
        else:
            manifest = T721.load(args.manifest)
            config, raw, archive_sha, fields = tail_from_archive(args.archive, args.receipt)
            enforce(manifest["archiveSHA"] == archive_sha, "manifest archive SHA")
            view = manifest["view"]
            enforce(all(view[dest] == fields[src] for dest, src in
                        (("u32","u"), ("v32","v"), ("piQ32","piQ32"))), "GridView raw field mismatch")
            result = inspect(manifest, config, raw)
    except (ValueError, TypeError, OSError, KeyError, IndexError, StopIteration, struct.error) as error:
        result = dict(state="INVALID", error=str(error))
    print(json.dumps(result, indent=2))
    sys.exit(int(result["state"] not in ("SYNTHETIC_PASS", "SPEC_ONLY")))
