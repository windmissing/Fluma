"""Summarize actual unsigned T701 App builds for developer handoff."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from zipfile import ZipFile


def sha(value):
    return hashlib.sha256(value).hexdigest()


def filesha(path):
    return sha(path.read_bytes())


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def only(paths):
    paths = list(paths)
    if len(paths) != 1:
        raise ValueError(f"expected exactly one artifact, found {paths}")
    return paths[0]


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def role_sources(route, app, native):
    if route == "M1":
        path = native / "gpu_resident/full_m1_kernels.h"
    else:
        path = app / "entry/src/main/cpp/gpu_resident/m2route/full_kernel.cpp"
    matches = re.findall(rb'R"([A-Za-z0-9_]*)\(([\s\S]*?)\)\1"', path.read_bytes())
    require(len(matches) == 2, f"missing role shader source in {path}")
    return [(role, match[1]) for role, match in zip(("SOURCE", "FINAL_Q"), matches)]


def resource_layout(route, app, folder):
    path = app / "entry/src/main/cpp/gpu_resident" / folder
    candidate = (path / "quantum_candidate.cpp").read_text(encoding="utf8")
    final = (path / "full_transport.cpp").read_text(encoding="utf8")
    extension = 8192 if route == "M2" else 0
    require("water=16*K;column=water+16*Kw;header=column+2048;words=header+256" in candidate and
            "workWords=sourceEvents+4096*16;eventCapacity=4*(48*K+16*Kw+254)+K" in candidate and
            "expanded=replayFrozen+256" + ("+8192" if extension else "") + ";" in final,
            "compiled route resource formulas differ from frozen parent")
    layout = []
    for sand in (192, 512):
        for water in (64, 128):
            words = 16 * sand + 16 * water + 2048 + 256
            pad = 256 if sand == 192 else 512
            work = 32 + pad + 2 * water + 127 * 16 + 512 + sand + water + 61444 + 4096 * 16
            layout.append({"K": sand, "Kw": water, "candidateBytes": words * 4,
                           "finalWorkspaceBytes": (work + words + 256 + extension) * 4,
                           "oneQEventArenaBytes": (4 * (48 * sand + 16 * water + 254) + sand) * 256})
    return layout


def evidence(matrix, arm, log):
    app = matrix / arm
    manifest = read(app / "t701-arm.json")
    route = manifest["route"]
    record = log.read_bytes()
    for stage in (b"BuildNativeWithNinja", b"CompileArkTS", b"PackageHap"):
        require(b"Finished :entry:default@" + stage in record, f"missing {stage} in {log}")
    require(b"BUILD SUCCESSFUL" in record, f"build failed: {log}")
    native = only((app / "entry/.cxx").rglob("compile_commands.json")).parent
    cfg = (native / "CMakeCache.txt").read_text(encoding="utf8")
    for key, value in (("CREATE_VARIANT", manifest["variant"]), ("COMPUTE_CACHE", manifest["cache"])):
        require(f"GRS_FULL_{key}:STRING={value}" in cfg, "arm CMake mismatch")
    hap = only((app / "entry/build/default/outputs").rglob("*-unsigned.hap"))
    with ZipFile(hap) as bundle:
        native_entry = only(name for name in bundle.namelist() if name.endswith("/libgrsm1.so"))
        abc_entry = only(name for name in bundle.namelist() if name.endswith("/modules.abc"))
        packaged = bundle.read(native_entry)
        abc = bundle.read(abc_entry)
    sdk = re.search(r"^CMAKE_TOOLCHAIN_FILE:[^=]+=(.*)$", cfg, re.M).group(1)
    readelf = Path(sdk).parents[2] / "llvm/bin/llvm-readelf.exe"
    # Keep the exact packaged ELF next to this build's evidence.
    intermediate = app / "entry/build/default/intermediates"
    binary = only((intermediate / "stripped_native_libs").rglob("libgrsm1.so"))
    require(binary.read_bytes() == packaged, "packaged ELF does not match stripped build output")
    linked = only((intermediate / "cmake").rglob("libgrsm1.so"))
    def build_id(file):
        notes = subprocess.check_output([str(readelf), "-n", str(file)], text=True)
        return re.search(r"Build ID: ([0-9a-f]+)", notes).group(1)
    linked_id = build_id(linked)
    require(linked_id == build_id(binary), "packaged BuildId differs from freshly linked ELF")
    source_digests = {}
    for role, source in role_sources(route, app, native):
        source = source.replace(b"\r\n", b"\n")
        digest = sha(source)
        require(digest == manifest["sourceSha" if role == "SOURCE" else "finalSha"] and
                source + b"\0" in packaged, f"{role} canonical bytes missing in {hap}")
        source_digests[role] = digest
    folder = "m1" if route == "M1" else "m2route"
    names = {"source_transport.cpp", "quantum_candidate.cpp", "full_transport.cpp", "full_kernel.cpp"}
    commands = read(native / "compile_commands.json")
    objects = {}
    for row in commands:
        source = Path(row["file"])
        command = row["command"]
        route_unit = source.parent.name == folder and source.name in names
        app_unit = source.name == "app_bridge.cpp" and "-DGRS_APP_M1" in command
        if not route_unit and not app_unit:
            continue
        output = re.search(r' -o ("[^"]+"|\S+) -c ', command).group(1).strip('"')
        obj = Path(row["directory"]) / output
        objects[source.name] = {"sourceSHA256": filesha(source), "objectSHA256": filesha(obj),
                                "argvSHA256": sha(command.encode()), "object": output}
    require(set(objects) == names | {"app_bridge.cpp"}, "missing actual App entry or role compile commands")
    declaration_keys = ["route", "variant", "cache", "roles", "role1", "independentQ", "sourceSha", "finalSha"]
    if "firstFrameControl" in manifest:
        declaration_keys.append("firstFrameControl")
    declaration_text = json.dumps({key: manifest[key] for key in declaration_keys},
        separators=(",", ":")).encode()
    require(declaration_text + b"\0" in packaged, "linked arm declaration missing")
    return {
        "arm": arm, "variant": manifest["variant"], "cache": manifest["cache"],
        "firstFrameControl": manifest.get("firstFrameControl"),
        "roles": manifest["roles"], "role1": manifest["role1"],
        "independentQ": manifest["independentQ"], "parent": manifest["parentDigest"],
        "roleSources": source_digests, "objects": objects, "abi": "arm64-v8a",
        "resourceLayout": resource_layout(route, app, folder),
        "cmake": filesha(native / "CMakeCache.txt"),
        "compileCommands": filesha(native / "compile_commands.json"),
        "ninja": filesha(native / "build.ninja"), "logSHA256": sha(record), "exit": 0,
        "hap": str(hap), "hapSHA256": filesha(hap), "elfSHA256": sha(packaged),
        "buildId": linked_id, "linkedELFSHA256": filesha(linked), "abcSHA256": sha(abc),
        "duration": re.search(rb"BUILD SUCCESSFUL in (.*)", record).group(1).decode().strip(),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("matrix", type=Path)
    parser.add_argument("logs", type=Path)
    parser.add_argument("out", type=Path)
    args = parser.parse_args()
    matrix = args.matrix.resolve()
    generation = read(matrix / "t701-generation.json")
    route = generation["route"]
    selected = ("THREE_OFF", "TWO_OFF", "THREE_ON", "TWO_ON")
    if "firstFrameControlSHA256" in generation:
        selected = ("THREE_OFF",) if route == "M1" else ("TWO_ON",)
    arms = [evidence(matrix, arm, args.logs / f"t701-{route}-{arm}.log") for arm in selected]
    require(len({json.dumps(arm["roleSources"]) for arm in arms}) == 1, "role GLSL differs across arms")
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps({
        "schema": "T701_APP_COMPILE_V1", "route": route,
        "generationManifestSHA256": filesha(matrix / "t701-generation.json"),
        "generatorCommit": generation["inputCommit"], "arms": arms, "unsigned": True,
        "productExecuted": False, "deviceUsed": False, "testsRun": False,
        "actualGain": "NOT_MEASURED",
    }, indent=2) + "\n", encoding="utf8")


if __name__ == "__main__":
    main()
