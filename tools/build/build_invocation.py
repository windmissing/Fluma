"""Record a serialized native build invocation and its byte evidence."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from uuid import uuid4

SCHEMA = "GRS_BUILD_INVOCATION_V1"
TARGET = "grsT666Consumer"
BROKER = "grs_cold_slots"
FILES = (
    "CMakeCache.txt", "compile_commands.json", "build.ninja",
    "CMakeFiles/rules.ninja", ".ninja_log", "grs-link-policy.json",
    "grs_owned_toolchain.cmake", "grs-sdk-flags.cmake",
    "grs_cold_slots.link.json", "grs_cold_slots.patch-state.json",
    "grs_cold_slots.full.rsp", "grs_cold_slots.link-receipt.json",
    "grs_cold_slots.driver-diagnostic.log", "grs_cold_slots.link.stdout",
    "grs_cold_slots.link.stderr", "lib/libgrs_cold_slots.so", "libgrsT666Consumer.so",
)

def load(path):
    return json.loads(Path(path).read_text(encoding="utf8"))

def describe(path):
    path = Path(path).resolve(strict=True)
    data = path.read_bytes()
    return {"path": str(path), "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest()}

def atomic_json(path, value):
    path = Path(path)
    temp = path.with_suffix(".tmp")
    temp.write_text(json.dumps(value, indent=2) + "\n", encoding="utf8")
    os.replace(temp, path)

def contained(path, root):
    path = Path(path).resolve()
    if not path.is_relative_to(root):
        raise ValueError("BUILD_SCOPE_MISMATCH " + str(path))
    return path

def snapshot(build):
    result = {}
    for name in FILES:
        path = contained(build / name, build)
        if path.is_file():
            result[name] = describe(path)
    manifest = build / "grs_cold_slots.link.json"
    if manifest.is_file():
        for name in load(manifest)["objects"]:
            path = contained(name, build)
            if path.is_file():
                result[path.relative_to(build).as_posix()] = describe(path)
    # Include consumer and all other actually generated native object outputs.
    for path in (build / "CMakeFiles").rglob("*.o"):
        path = contained(path, build)
        result[path.relative_to(build).as_posix()] = describe(path)
    return result

def command(policy_file, policy, build, stage):
    if stage in {"configure", "reconfigure"}:
        return ([sys.executable, str(Path(__file__).with_name("owned_build.py")),
                 "configure", str(policy_file), str(build)],
                Path(policy["source"]).resolve())
    if stage in {"prepare-link", "reprepare-link"}:
        return ([sys.executable, str(Path(__file__).with_name("owned_build.py")),
                 "prepare-link", str(policy_file), str(build), BROKER], build)
    return ([str(Path(policy["ninja"]).resolve()), "-C", str(build), "-v", TARGET], build)

def record(policy_file, build, incremental):
    policy_file = Path(policy_file).resolve(strict=True)
    policy = load(policy_file)
    build = Path(build).resolve()
    build.mkdir(parents=True, exist_ok=True)
    guard = build / ".grs-invocation.lock"
    with guard.open("x", encoding="ascii") as lock:
        invocation = uuid4().hex
        lock.write(invocation)
    try:
        return record_locked(policy_file, policy, build, incremental, invocation)
    finally:
        guard.unlink()

def record_locked(policy_file, policy, build, incremental, invocation):
    output = build / "grs-invocations" / invocation
    contained(output, build).mkdir(parents=True, exist_ok=False)
    plan = ["configure", "prepare-link", "initial"]
    if incremental:
        plan += ["reconfigure", "reprepare-link", "no-work", "rsp-relink", "repeat"]
    manifest = {
        "schema": SCHEMA, "invocation": invocation, "buildRoot": str(build),
        "target": TARGET, "plan": plan, "steps": [],
        "inputs": {name: describe(policy[name]) for name in ("cmake", "ninja", "sdkToolchain")},
        "policy": describe(policy_file), "state": "INCOMPLETE",
    }
    manifest_path = output / "invocation.json"
    atomic_json(manifest_path, manifest)
    for index, stage in enumerate(plan):
        argv, cwd = command(policy_file, policy, build, stage)
        step_id = uuid4().hex
        step = {"id": step_id, "parent": invocation, "buildRoot": str(build),
                "stage": stage, "target": TARGET, "argv": argv, "cwd": str(cwd),
                "tool": describe(argv[0]), "before": snapshot(build)}
        if stage == "rsp-relink":
            path = contained(build / "grs_cold_slots.full.rsp", build)
            os.utime(path, None)
        env = dict(os.environ, GRS_INVOCATION_PARENT=invocation,
                   GRS_INVOCATION_STEP=step_id, GRS_INVOCATION_ROOT=str(build))
        step["beginNs"] = time.time_ns()
        stdout_path = output / f"{index:02d}-{step_id}.stdout"
        stderr_path = output / f"{index:02d}-{step_id}.stderr"
        with stdout_path.open("xb") as stdout, stderr_path.open("xb") as stderr:
            try:
                process = subprocess.run(argv, cwd=cwd, env=env, stdout=stdout, stderr=stderr)
                code = process.returncode
            except OSError as error:
                stderr.write(str(error).encode("utf8"))
                code = -1
        step.update(endNs=time.time_ns(), returnCode=code,
                    stdout=describe(stdout_path), stderr=describe(stderr_path),
                    after=snapshot(build))
        child = build / "grs_cold_slots.link-receipt.json"
        if child.is_file():
            link = load(child)
            if link.get("step") == step_id:
                (output / f"{index:02d}-{step_id}.link.json").write_bytes(child.read_bytes())
                step["link"] = describe(output / f"{index:02d}-{step_id}.link.json")
                step["linkStreams"] = {}
                for stream in ("stdout", "stderr"):
                    archive = output / f"{index:02d}-{step_id}.link.{stream}"
                    archive.write_bytes(Path(link[stream]["path"]).read_bytes())
                    step["linkStreams"][stream] = describe(archive)
        native = output / f"{step_id}.native.json"
        if native.is_file():
            step["native"] = describe(native)
        manifest["steps"].append(step)
        if code:
            manifest["state"] = "FAILED"
            break
        atomic_json(manifest_path, manifest)
    else:
        manifest["state"] = "SEALED"
    atomic_json(manifest_path, manifest)
    print(manifest_path)
    return 0 if manifest["state"] == "SEALED" else 1

def native_call(argv, cwd):
    """Capture a direct CMake or compdb child, preserving bytes and argv."""
    begin = time.time_ns()
    result = subprocess.run(argv, cwd=cwd, capture_output=True, shell=False)
    end = time.time_ns()
    step = os.environ.get("GRS_INVOCATION_STEP")
    if step:
        parent = os.environ["GRS_INVOCATION_PARENT"]
        build = Path(os.environ["GRS_INVOCATION_ROOT"]).resolve()
        folder = contained(build / "grs-invocations" / parent, build)
        streams = {}
        for name, data in [("stdout", result.stdout), ("stderr", result.stderr)]:
            with (folder / f"{step}.native.{name}").open("xb") as file:
                file.write(data)
            streams[name] = describe(file.name)
        atomic_json(folder / f"{step}.native.json", {
            "parent": parent, "step": step, "buildRoot": str(build), "argv": argv,
            "tool": describe(argv[0]), "cwd": str(Path(cwd).resolve()),
            "beginNs": begin, "endNs": end, "returnCode": result.returncode, **streams})
    return result

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("policy")
    parser.add_argument("build")
    parser.add_argument("--incremental", action="store_true")
    args = parser.parse_args()
    raise SystemExit(record(args.policy, args.build, args.incremental))
