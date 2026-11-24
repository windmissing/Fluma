"""Freeze T956 layouts and analytic fixtures, then compile the three Vulkan roles."""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct
import subprocess


ROOT = Path(__file__).resolve().parents[2]
ABI = ROOT / "docs/architecture-T956-abi-v1.json"


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def layouts(abi):
    g14 = abi["G14"]
    result = {}
    for slot in g14["storageBindings"]:
        result[f"B{slot['binding']}_BYTES"] = slot["bytes"]
        for name, row in slot.get("records", {}).items():
            prefix = name.upper()
            result.update({f"{prefix}_BASE": row["base"] // 4, f"{prefix}_STRIDE": row["stride"] // 4,
                           f"{prefix}_CAP": row["capacity"]})
    root = g14["storageBindings"][3]
    result.update(CHILD_BASE=g14["storageBindings"][0]["childBase"] // 4,
                  INPUT_STEP=g14["storageBindings"][0]["childStride"] // 4,
                  ARGS_BASE=g14["storageBindings"][0]["analyticArgsBase"] // 4,
                  SNAPSHOT_STEP=root["childStride"] // 4, SNAPSHOT_AUDIT=root["childAuditOffset"] // 4)
    assert sum(result[f"B{i}_BYTES"] for i in range(4)) == g14["capabilities"]["logicalDeviceBytes"]
    return result


def events(nx, ny):
    found = {}
    for i in range(nx + 1):
        for j in range(ny + 1):
            n, d = (5*j - 2*ny)*nx, 4*ny*(2*i-nx)
            if d:
                t = Fraction(n, d)
                if 0 < t < Fraction(1, 4):
                    found.setdefault(t, []).append((i, j))
    return found


def schedule(steps, set_of_events):
    for i in range(steps):
        bounds = [Fraction(i, 4*steps), Fraction(i+1, 4*steps)]
        bounds[1:1] = sorted(t for t in set_of_events if bounds[0] < t < bounds[-1])
        yield list(zip(bounds, bounds[1:]))


def make_fixture(kind, nx, ny, steps, flags, set_of_events):
    roots = []
    for root, intervals in enumerate(schedule(steps, set_of_events)):
        if kind != 3:
            intervals = [(Fraction(0), Fraction(1, 100))]
        assert 1 <= len(intervals) <= 4
        data = bytearray(4096)
        struct.pack_into("<12I", data, 0, 0x47313449, 1, kind, nx, ny, len(intervals), flags,
                         root+1, 0, 0, 0, 0)
        for i in range(nx+1):
            struct.pack_into("<2I", data, 256+i*32, i, nx)
        for child, (t0, t1) in enumerate(intervals):
            struct.pack_into("<6I", data, 1792+child*128,
                             t0.numerator, t0.denominator, t1.numerator, t1.denominator,
                             child, root+1+int(kind==2 and bool(flags&4)))
        length, height = (.02, .01) if kind == 2 else (.1, .025)
        struct.pack_into("<8d", data, 2304, length, height, 1000, .01, 1,
                         .20 if kind==1 else 0, 7 if kind==1 and flags&2 else 0, 0)
        roots.append(data)
    return b"".join(roots)


def fixture_pack(abi):
    for arm in abi["G14"]["offlineChecks"]["C_event_schedule"]:
        nx, ny, steps = arm["nx"], arm["ny"], int(Fraction(1, 4)/Fraction(arm["dtSeconds"]))
        known = events(nx, ny)
        assert sorted(known) == [Fraction(t) for t in arm["interiorEvents"]]
        assert [len(q) for q in schedule(steps, known)] == arm["intervalCounts"]
        yield f"C-{nx}x{ny}", make_fixture(3, nx, ny, steps, 0, known)
    for name, kind, nx, ny, flags in (
            ("A", 1, 20, 10, 0), ("A-reverse", 1, 20, 10, 1),
            ("A-gauge", 1, 20, 10, 2), ("A-bad-gauge", 1, 20, 10, 6),
            ("B", 2, 2, 1, 0), ("B-closed", 2, 2, 1, 1), ("B-wrong-sign", 2, 2, 1, 2),
            ("B-closed-flux", 2, 2, 1, 17),
            ("B-wrong-time", 2, 2, 1, 4), ("B-wrong-generation", 2, 2, 1, 8)):
        yield name, make_fixture(kind, nx, ny, 1, flags, {})


def build(out, compiler):
    wire = ABI.read_bytes()
    abi = json.loads(wire)
    limits = layouts(abi)
    out.mkdir(parents=True, exist_ok=True)
    manifest = {"schema": "T957_G14_ANALYTIC_V1", "abiSHA": sha(wire), "roles": [], "fixtures": []}
    shader = Path(__file__).with_name("g14_boundary.comp").read_text(encoding="utf8")
    glsl = "".join(f"#define {key} {value}u\n" for key, value in limits.items())
    for role, name in enumerate(("geometry", "transport", "audit")):
        source = shader.replace("/* GENERATED_LAYOUT */", f"#define ROLE {role}\n"+glsl).encode()
        assert len(source) <= 49152
        src, spv = out/f"{name}.comp", out/f"{name}.spv"
        src.write_bytes(source)
        check = subprocess.run([str(compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis",
                                "-o", str(spv), str(src)], capture_output=True, text=True)
        if check.returncode or "error:" in (check.stdout+check.stderr).lower() or "LocalSize 64 1 1" not in check.stdout:
            raise ValueError(check.stdout+check.stderr)
        manifest["roles"].append(dict(file=spv.name, sha=sha(spv.read_bytes()), bytes=spv.stat().st_size,
                                      sourceBytes=len(source), sourceSHA=sha(source)))
    for name, raw in fixture_pack(abi):
        file = out/f"{name}.g14"
        file.write_bytes(raw)
        manifest["fixtures"].append(dict(file=file.name, sha=sha(raw), bytes=len(raw)))
    data = (json.dumps(manifest, indent=2)+"\n").encode()
    (out/"modules.json").write_bytes(data)
    header = "#pragma once\nnamespace grs::pc::sand::g14 {\n"
    header += "".join(f"inline constexpr unsigned {key}={value};\n" for key, value in limits.items())
    header += f'inline constexpr char kManifestSHA[]="{sha(data)}", kLayoutSHA[]="{sha(wire)}";\n'
    header += "struct Asset { const char *name,*sha; unsigned bytes; };\n"
    for key, name in (("roles", "kModules"), ("fixtures", "kFixtures")):
        header += f"inline constexpr Asset {name}[]={{\n"
        for row in manifest[key]:
            header += f'  {{"{row["file"]}","{row["sha"]}",{row["bytes"]}}},\n'
        header += "};\n"
    (out/"g14_layout.h").write_text(header+"}\n", encoding="ascii")


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--compiler", type=Path)
    cli.add_argument("--out", type=Path)
    cli.add_argument("--check-fixtures", action="store_true")
    args = cli.parse_args()
    if args.check_fixtures:
        for name, raw in fixture_pack(json.loads(ABI.read_bytes())):
            print(name, len(raw)//4096, sha(raw))
    else:
        build(args.out, args.compiler)
