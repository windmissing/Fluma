"""Supervise one Windows PC diagnostic child at a time under the T834 limits."""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import subprocess
import time
import uuid


def times(function, handle):
    result = [wintypes.FILETIME() for _ in range(4)]
    if not function(handle, *(ctypes.byref(t) for t in result)):
        return None
    return [str((t.dwHighDateTime << 32) | t.dwLowDateTime) for t in result]


def supervise(exe, role, out, disable):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = kernel.OpenThread.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = kernel.OpenThread.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    for f in (kernel.GetThreadTimes, kernel.GetProcessTimes):
        f.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
        f.restype = wintypes.BOOL
    stem = out / (role + ("-noopt" if disable else ""))
    stdout, stderr = stem.with_suffix(".jsonl"), stem.with_suffix(".stderr")
    invocation = [str(exe.resolve()), "--qualify-steady-role", role]
    if disable:
        invocation.append("--disable-optimization")
    samples, rows = [], []
    build = None
    reason = "CHILD_EXIT"
    created = time.monotonic()
    offset, next_sample = 0, 0
    with stdout.open("xb") as output, stderr.open("xb") as error:
        with subprocess.Popen(invocation, stdout=output, stderr=error) as child:
            process = kernel.OpenProcess(0x1000, False, child.pid)
            try:
                owner = None
                last_enter = None
                while True:
                    alive = child.poll() is None
                    with stdout.open("rb") as reader:
                        reader.seek(offset)
                        for line in reader:
                            if not line.endswith(b"\n"):
                                break
                            offset += len(line)
                            row = json.loads(line)
                            rows.append(row)
                            owner = row.get("ownerTid", owner)
                            if row.get("call") == "vkCreateComputePipelines":
                                build = row
                                last_enter = (int(row["qpc"])/int(row["qpcFrequency"])
                                              if row["edge"] == "ENTER" else None)
                    now = time.monotonic()
                    if now >= next_sample:
                        thread = kernel.OpenThread(0x0800, False, owner) if owner else None
                        try:
                            samples.append(dict(elapsed=now-created, ownerTid=owner, alive=alive,
                                                processTimes=times(kernel.GetProcessTimes, process) if process else None,
                                                ownerTimes=times(kernel.GetThreadTimes, thread) if thread else None))
                        finally:
                            if thread:
                                kernel.CloseHandle(thread)
                        next_sample = now + 5
                    if not alive:
                        break
                    if last_enter is not None and time.perf_counter() - last_enter >= 60:
                        reason = "CALL_UNRETURNED"
                        break
                    if now-created >= 120:
                        reason = "CHILD_WALL_LIMIT"
                        break
                    time.sleep(0.1)
                forced = child.poll() is None
                if forced:
                    child.kill()
                exit_code = child.wait(timeout=10)
            finally:
                if process:
                    kernel.CloseHandle(process)
                if child.poll() is None:
                    child.kill()
    final = rows[-1] if rows else {}
    probe = next((r for r in rows if r.get("schema") == "PC_G13_STEADY_OP66_V1"), None)
    retired = not forced and final.get("state") == "STEADY_ROLE_QUALIFIED"
    receipt = dict(attemptId=str(uuid.uuid4()), role=role, pid=child.pid, reason=reason, lastPipeline=build,
                   exeSHA=hashlib.sha256(exe.read_bytes()).hexdigest(), stdoutSHA=hashlib.sha256(stdout.read_bytes()).hexdigest(),
                   samples=samples, probe=probe, forced=forced, exitCode=exit_code, ownerRetired=retired)
    stem.with_suffix(".receipt.json").write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf8")
    return receipt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("roles", nargs="+")
    parser.add_argument("--disable-optimization", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    for role in args.roles:
        if not role.startswith("OP_") or not all(x.isdecimal() for x in role[3:].split("_")):
            parser.error("invalid role")
        receipt = supervise(args.exe, role, args.out, args.disable_optimization)
        print(role, receipt["reason"], receipt["exitCode"], receipt["ownerRetired"], flush=True)
