"""Supervise one Windows PC diagnostic child at a time under the T834 limits."""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import msvcrt
import re
from pathlib import Path
import subprocess
import time
import uuid


def times(function, handle):
    result = [wintypes.FILETIME() for _ in range(4)]
    if not function(handle, *(ctypes.byref(t) for t in result)):
        return None
    return [str((t.dwHighDateTime << 32) | t.dwLowDateTime) for t in result]


def qpc(kernel, function):
    value = ctypes.c_longlong()
    if not getattr(kernel, function)(ctypes.byref(value)):
        raise ctypes.WinError(ctypes.get_last_error())
    return value.value


def write_stack_dump(dbghelp, kernel, pid, path):
    # The handle is for the child owned by this supervisor. Do not ask for
    # full process memory or handle data.
    process = kernel.OpenProcess(0x0410, False, pid)
    if not process:
        return dict(win32Error=ctypes.get_last_error())
    try:
        with path.open("xb") as dump:
            ok = dbghelp.MiniDumpWriteDump(process, pid, msvcrt.get_osfhandle(dump.fileno()),
                                           0x1000, None, None, None)
            error = 0 if ok else ctypes.get_last_error()
    finally:
        kernel.CloseHandle(process)
    if not ok:
        path.unlink()
        return dict(win32Error=error or "UNKNOWN")
    return dict(file=path.name, bytes=path.stat().st_size, sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def supervise(exe, role, out, disable, stack_dumps=False, private_writes=False):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = kernel.OpenThread.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = kernel.OpenThread.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    for f in (kernel.GetThreadTimes, kernel.GetProcessTimes):
        f.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
        f.restype = wintypes.BOOL
    frequency = qpc(kernel, "QueryPerformanceFrequency")
    dbghelp = None
    if stack_dumps:
        dbghelp = ctypes.WinDLL("dbghelp.dll", winmode=0x800, use_last_error=True)
        dbghelp.MiniDumpWriteDump.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.HANDLE,
                                            wintypes.DWORD] + [ctypes.c_void_p] * 3
        dbghelp.MiniDumpWriteDump.restype = wintypes.BOOL
    stem = out / (role + ("-noopt" if disable else ""))
    stdout, stderr = stem.with_suffix(".jsonl"), stem.with_suffix(".stderr")
    invocation = [str(exe.resolve()), "--qualify-steady-role", role]
    if private_writes:
        flag = "--qualify-t854-writes" if role == "T854_PRIVATE_WRITES" else "--qualify-t849-writes"
        invocation = [str(exe.resolve()), flag, str(stem.with_suffix(".bin").resolve())]
    if disable:
        invocation.append("--disable-optimization")
    samples, rows = [], []
    dumps, remaining_dumps = [], [20, 45] if stack_dumps else []
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
                                if int(row["qpcFrequency"]) != frequency:
                                    raise ValueError("child and supervisor QPC frequency differ")
                                last_enter = int(row["qpc"]) if row["edge"] == "ENTER" else None
                    now = time.monotonic()
                    tick = qpc(kernel, "QueryPerformanceCounter")
                    if now >= next_sample:
                        thread = kernel.OpenThread(0x0800, False, owner) if owner else None
                        try:
                            samples.append(dict(elapsed=now-created, qpc=str(tick), ownerTid=owner, alive=alive,
                                                processTimes=times(kernel.GetProcessTimes, process) if process else None,
                                                ownerTimes=times(kernel.GetThreadTimes, thread) if thread else None))
                        finally:
                            if thread:
                                kernel.CloseHandle(thread)
                        next_sample = now + 5
                    if not alive:
                        break
                    if last_enter is not None and tick-last_enter >= 60*frequency:
                        reason = "CALL_UNRETURNED"
                        break
                    if now-created >= 120:
                        reason = "CHILD_WALL_LIMIT"
                        break
                    if last_enter is not None and remaining_dumps and tick-last_enter >= remaining_dumps[0]*frequency:
                        mark = remaining_dumps.pop(0)
                        dump = out / f"{stem.name}-{mark}s.dmp"
                        capture = write_stack_dump(dbghelp, kernel, child.pid, dump)
                        dumps.append(dict(targetSeconds=mark, beginQpc=str(tick),
                                          endQpc=str(qpc(kernel, "QueryPerformanceCounter")), ownerTid=owner, **capture))
                    time.sleep(0.1)
                stop_tick = qpc(kernel,"QueryPerformanceCounter")
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
    probes = [r for r in rows if r.get("schema") == "PC_G13_STEADY_OP66_V1"]
    probe = probes[0] if probes else None
    retired = not forced and final.get("state") in ("STEADY_ROLE_QUALIFIED","T849_PRIVATE_WRITES_CAPTURED",
                                                     "T854_PRIVATE_WRITES_CAPTURED")
    receipt = dict(attemptId=str(uuid.uuid4()), role=role, pid=child.pid, reason=reason, lastPipeline=build,
                   exeSHA=hashlib.sha256(exe.read_bytes()).hexdigest(), stdoutSHA=hashlib.sha256(stdout.read_bytes()).hexdigest(),
                   supervisorStopQpc=str(stop_tick), qpcFrequency=str(frequency), samples=samples, stackDumps=dumps,
                   probe=probe, forced=forced, exitCode=exit_code, ownerRetired=retired)
    if private_writes:
        receipt.update(probes=probes, privateWrites=final.get("t854PrivateWrites",final.get("t849PrivateWrites")))
    stem.with_suffix(".receipt.json").write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf8")
    return receipt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("roles", nargs="*")
    parser.add_argument("--t849-private-writes", action="store_true",
                        help="capture frozen diagnostic buffers for the qualified OP7 subphases and downstream OP8")
    parser.add_argument("--t854-private-writes", action="store_true",
                        help="capture three private OP17 cases after each instrumented phase qualifies")
    parser.add_argument("--disable-optimization", action="store_true")
    parser.add_argument("--stack-dumps", action="store_true",
                        help="take standard thread-stack minidumps of the owned child at 20s and 45s")
    args = parser.parse_args()
    private_writes = args.t849_private_writes or args.t854_private_writes
    if private_writes:
        if args.roles or args.disable_optimization or (args.t849_private_writes and args.t854_private_writes):
            parser.error("private write set uses the pinned roles and default flags")
        args.roles = ["T854_PRIVATE_WRITES" if args.t854_private_writes else "T849_PRIVATE_WRITES"]
    elif not args.roles:
        parser.error("provide at least one role")
    args.out.mkdir(parents=True, exist_ok=True)
    failed = False
    for role in args.roles:
        if not private_writes and not re.fullmatch(
                r"OP(?:_[0-9]+(?:_[0-9]+)*|7_(?:FIELD|DRIVE|MOTION|MMS_FORCE|REDUCE)_ONLY|"
                r"17_(?:LEDGER_ONLY|DRIVE_CONTEXT|MOTION_CONTEXT|MMS_CONTEXT|PRIVATE_MOTION|PRIVATE_MMS|PRIVATE_LEDGER)|"
                r"6|8|12_14|15|16|17)", role):
            parser.error("invalid role")
        receipt = supervise(args.exe, role, args.out, args.disable_optimization, args.stack_dumps, private_writes)
        print(role, receipt["reason"], receipt["exitCode"], receipt["ownerRetired"], flush=True)
        failed |= receipt["exitCode"] != 0 or not receipt["ownerRetired"]
    raise SystemExit(int(failed))
