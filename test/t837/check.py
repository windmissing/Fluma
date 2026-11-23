"""Independent PC evidence gate for the T828 pure fluid cases."""
import argparse
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/g13"))
from read_archive import exact_id, read, tagged_receipt


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


def candidate_archive(path, receipt):
    result = read(path, receipt)
    enforce(result["state"] == "STRUCTURALLY_COMPLETE", "forensic/partial archive")
    # Validate the exact file SHA again while extracting bounded windows.
    checksum = hashlib.sha256()
    config, tail = bytearray(), bytearray()
    with path.open("rb") as source:
        first = source.readline(16385)
        header = json.loads(first)["words"]
        kind = {1: "ROOT", 2: "FULL"}[header[4]]
        profile = next(p for p in LAYOUT["profiles"] if (p["Nx"], p["Ny"]) == tuple(header[96:98]))
        directory = {d["name"]: d for d in profile[f"{kind}ColdDirectory"]}
        ctrl = directory["CTRL"]["byteOffset"]
        candidate = directory["CANDIDATE"]["byteOffset"]
        raw_fields = {n: bytearray() for n in ("u", "v", "piQ32", "motion")} if kind == "FULL" else {}
        packs = {n: bytearray() for n in ("PACK0", "PACK1")} if kind == "FULL" else {}
        native_raw = bytearray()
        permit_raw = bytearray()
        spans = [(ctrl,512,config), (ctrl+73728,1024,tail), (ctrl+512,512,permit_raw),
                 (5504*4,1024,native_raw)]
        spans.extend((candidate+f["byteStart"], f["bytes"], raw_fields[f["name"]])
                     for f in profile["state"] if f["name"] in raw_fields)
        if raw_fields:
            spans.append((candidate+30*4, 4, raw_fields["motion"]))
        spans.extend((directory[name]["byteOffset"], directory[name]["byteLength"], data)
                     for name, data in packs.items())
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
    native = tagged_receipt(io.BytesIO(native_raw), 0, 256, 6)
    permit = struct.unpack("<128I", permit_raw)
    return dict(kind=kind, config=bytes(config), tail=bytes(tail), sha=result["fileSHA256"], fields=fields,
                packs=packs, native=native, permit=permit, header=header)


def tail_from_archive(path, receipt):
    """Compatibility helper for consumers that require a FULL field."""
    a = candidate_archive(path, receipt)
    enforce(a["kind"] == "FULL", "FULL capture required for GridView")
    return a["config"], a["tail"], a["sha"], {k: a["fields"][k] for k in ("u", "v", "piQ32")}


def pack_view(a):
    tail = struct.unpack("<256I", a["tail"])
    config = struct.unpack("<128I", a["config"])
    nx, ny = config[4:6]
    matches = []
    for data in a["packs"].values():
        words = struct.unpack(f"<{len(data)//4}I", data)
        if (len(words) == 64+16*nx*ny and words[:2] == (1446195527,1)
                and words[4:6] == (nx,ny) and words[7] == 1 and words[63] == 0xc0de1301
                and identity(words,18) == identity(tail,8) and identity(words,20) == identity(tail,26)
                and identity(words,14) == identity(tail,20) and words[24:32] == config[76:84]):
            matches.append(words)
    enforce(len(matches) == 1, "same-root GPU PACK required")
    words = matches[0]
    return dict(phi=[words[64+16*i+3] for i in range(nx*ny)], pRef=words[48], T0=config[11],
                motion={0:"REST",1:"REST",2:"RAMP_SPIN",3:"CONSTANT_SPIN"}[a["fields"]["motion"][0]])


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
    if case in ("F03","F06") and nx==ny and nx in (32,64,128):
        h=1/nx
        if case=="F03" and accepted_quantum==30 and nu in (1/16,1/8):
            sin_mode=[math.sin(2*math.pi*(j+.5)*h) for j in range(ny)]
            u=fields["u32"]
            amplitude=math.fsum(u[j*(nx+1)+i]*sin_mode[j]
                               for j in range(ny) for i in range(nx))*2/(nx*ny)
            eigenvalue=4*math.sin(math.pi*h)**2/h**2
            return dict(amplitude=amplitude,discreteByJ={
                str(j):.25*(1+nu*eigenvalue/(120*j))**(-30*j) for j in (1,2,4)})
        if case=="F06" and nu==1/64 and accepted_quantum>=1:
            time=accepted_quantum/120
            a=nu*math.pi**2*time
            tail=.25*math.exp(-a*128**2)/(math.pi*a*128**2)
            ys=[(j+.5)*h for j in range(ny)]
            analytic=[.25*y+.5/math.pi*math.fsum((-1)**k*math.sin(k*math.pi*y)*
                         math.exp(-a*k*k)/k for k in range(1,129)) for y in ys]
            measured=fields["u32"]
            return dict(startupTailBound=tail,startupMax=max(
                abs(measured[j*(nx+1)+i]-analytic[j]) for j in range(ny) for i in range(nx+1)),
                steadyMax=max(abs(measured[j*(nx+1)+i]-.25*ys[j])
                              for j in range(ny) for i in range(nx+1)))
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


def completion(e, raw, *, archive=None):
    enforce(len(raw) == 1024 and sha(raw) == e["completionSHA"], "Completion256 SHA/size")
    tail = struct.unpack("<256I", raw)
    enforce(tail[:4] == (MAGIC["tail"], 1, MAGIC["profile"], 256) and tail[255] == 0xc0de1301,
            "Completion256 header/seal")
    opcode = tail[4]
    expected_count = {0: 820, 50: 16}.get(opcode)
    enforce(expected_count is not None and tail[5] == 1 and tail[7] == 0 and tail[42:44] == (1, 3)
            and tail[39] == expected_count and tail[40]+tail[41] == tail[39]
            and (tail[37] & tail[38]) == tail[37], "accepted opcode/counts/audit")
    for name, offset in (("nonce",8), ("plan",10), ("owner",14), ("context",16),
                         ("surface",18), ("geometry",20), ("configGeneration",22),
                         ("commit",26), ("acceptedQuantum",34)):
        value = exact_id(e[name])
        enforce((value != 0 or opcode == 50 and name == "acceptedQuantum")
                and value == identity(tail, offset), "completion identity " + name)
    grant, deadline, gpu_accept = (identity(tail, i) for i in (28, 30, 32))
    if archive is not None and archive["native"].get("backend") == "VULKAN_PC":
        native = archive["native"]
        permit = archive["permit"]
        accepted_ns = exact_id(e["nativeAcceptedNs"])
        enforce(e["native"] == native and gpu_accept == 0 and native["gpuTailAvailable"] is True
                and native["nativeError"] == 0 and native["nativeOutcome"] == 1
                and e["acceptedRoot"] == e["nonce"] == native["nonce"]
                and opcode == permit[1] and permit[92] == 0 and tail[125:127] == (1,0)
                and tail[120:122] == (permit[3],permit[5]) and tail[37] == permit[7]
                and tail[8:24] == permit[8:24]
                and grant == identity(permit,36) and deadline == identity(permit,38)
                and accepted_ns == exact_id(native["acceptedNs"]) == identity(archive["header"],12),
                "native accepted epoch or permit")
    else:
        enforce("nativeAcceptedNs" not in e, "native receipt missing from archive")
        accepted_ns = gpu_accept
    enforce(0 < grant <= accepted_ns <= deadline <= grant+250_000_000, "completion deadline")
    return tail


def inspect(e, config, raw, synthetic=False, *, archive=None):
    enforce(e.get("synthetic") is synthetic, "evidence mode")
    case = e["case"]
    enforce(case in CASES, "fixture identity")
    if not e.get("acceptedRoot"):
        return dict(state="MISSING_EVIDENCE", missing=["nonempty accepted root"])
    enforce(case in PHYSICAL, "diagnostic case cannot consume physical publication")
    tail = completion(e, raw, archive=archive)
    enforce(tail[4] == 0, "physical QUANTUM required")
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


def inspect_archive(manifest, archive, *, synthetic=False):
    enforce(manifest["synthetic"] is synthetic, "evidence mode")
    enforce(manifest["archiveSHA"] == archive["sha"], "manifest archive SHA")
    if not manifest.get("acceptedRoot"):
        return dict(state="MISSING_EVIDENCE", missing=["nonempty accepted root"])
    tail = completion(manifest, archive["tail"], archive=archive)
    if tail[4] == 50 or archive["kind"] == "ROOT":
        enforce("view" not in manifest, "GridView requires FULL physical QUANTUM")
        return dict(state="SOURCE_ABI_ONLY" if tail[4] == 50 else "ROOT_ABI_ONLY",
                    acceptedRoot=manifest["acceptedRoot"], archiveKind=archive["kind"])
    view = manifest["view"]
    enforce(all(view[dest] == archive["fields"][src] for dest, src in
                (("u32","u"), ("v32","v"), ("piQ32","piQ32"))), "GridView raw field mismatch")
    if archive["native"].get("backend") == "VULKAN_PC":
        pack = pack_view(archive)
        bits = lambda x: struct.unpack("<I", struct.pack("<f", x))[0]
        enforce(view["phiSource"] == "SAME_ROOT_GPU_PACK_FIELD_3"
                and [bits(x) for x in view["phi"]] == pack["phi"]
                and bits(view["pRef"]) == pack["pRef"] and bits(view["T0"]) == pack["T0"]
                and view["motion"] == pack["motion"], "GridView GPU PACK provenance")
    return inspect(manifest, archive["config"], archive["tail"], synthetic, archive=archive)


def selftest():
    config = bytearray(512)
    struct.pack_into("<2I", config, 16, 2, 2)
    struct.pack_into("<3df", config, 48, 2, 1000, 4, .001)
    words = [0]*256
    words[:6] = [MAGIC["tail"], 1, MAGIC["profile"], 256, 0, 1]
    words[28:34] = [100, 0, 200, 0, 150, 0]
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
    archive = commands.add_parser("archive", help="verify native ROOT or FULL structure only")
    archive.add_argument("archive", type=Path)
    archive.add_argument("receipt", type=Path)
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
        elif args.cmd == "archive":
            result = read(args.archive, args.receipt)
        else:
            manifest = T721.load(args.manifest)
            result = inspect_archive(manifest, candidate_archive(args.archive, args.receipt))
    except (ValueError, TypeError, OSError, KeyError, IndexError, StopIteration, struct.error) as error:
        result = dict(state="INVALID", error=str(error))
    print(json.dumps(result, indent=2))
    sys.exit(int(result["state"] not in ("SYNTHETIC_PASS", "SPEC_ONLY", "STRUCTURALLY_COMPLETE",
                                        "SOURCE_ABI_ONLY", "ROOT_ABI_ONLY")))
