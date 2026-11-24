"""Capture comparable OP_48 calls within the T915 G13 diagnostic contract."""

import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import shutil

from qualify_role import supervise


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def file_version(path):
    version = ctypes.WinDLL("version", use_last_error=True)
    size = version.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return None
    data = ctypes.create_string_buffer(size)
    if not version.GetFileVersionInfoW(str(path), 0, size, data):
        return None
    address = ctypes.c_void_p()
    length = wintypes.UINT()
    if not version.VerQueryValueW(data, "\\", ctypes.byref(address), ctypes.byref(length)) or length.value < 16:
        return None
    signature, _, high, low = ctypes.cast(address, ctypes.POINTER(wintypes.DWORD))[:4]
    return ".".join(map(str, (high >> 16, high & 65535, low >> 16, low & 65535))) if signature == 0xFEEF04BD else None


class LoadedImages:
    def __init__(self):
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        self.kernel.OpenProcess.restype = wintypes.HANDLE
        self.kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        self.psapi = ctypes.WinDLL("psapi", use_last_error=True)
        self.psapi.EnumProcessModulesEx.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE),
                                                   wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.DWORD]
        self.psapi.GetModuleFileNameExW.argtypes = [wintypes.HANDLE, wintypes.HMODULE,
                                                   wintypes.LPWSTR, wintypes.DWORD]
        self.samples = []
        self.next_tick = 0
        from qualify_role import qpc
        self.frequency = qpc(self.kernel, "QueryPerformanceFrequency")

    def __call__(self, pid, tick):
        if tick < self.next_tick:
            return
        self.next_tick = tick + self.frequency * (1 if len(self.samples) < 3 else 5)
        process = self.kernel.OpenProcess(0x0410, False, pid)
        if not process:
            self.samples.append(dict(qpc=str(tick), win32Error=ctypes.get_last_error()))
            return
        try:
            modules = (wintypes.HMODULE * 1024)()
            amount = wintypes.DWORD()
            if not self.psapi.EnumProcessModulesEx(process, modules, ctypes.sizeof(modules), ctypes.byref(amount), 3):
                self.samples.append(dict(qpc=str(tick), win32Error=ctypes.get_last_error()))
                return
            names, missing = [], 0
            path = ctypes.create_unicode_buffer(32768)
            for module in modules[:min(len(modules), amount.value // ctypes.sizeof(wintypes.HMODULE))]:
                if self.psapi.GetModuleFileNameExW(process, module, path, len(path)):
                    names.append(path.value)
                else:
                    missing += 1
            self.samples.append(dict(qpc=str(tick), paths=sorted(names), missing=missing,
                                     truncated=amount.value > ctypes.sizeof(modules)))
        finally:
            self.kernel.CloseHandle(process)

    def receipts(self):
        images = {}
        for sample in self.samples:
            for name in sample.get("paths", []):
                path = Path(name)
                lower = path.name.lower()
                if name in images or not (lower.startswith(("vulkan", "ig", "intel", "nv", "amd", "ati"))
                                          and lower.endswith(".dll")):
                    continue
                try:
                    images[name] = dict(sha256=digest(path), version=file_version(path))
                except OSError as error:
                    images[name] = dict(error=str(error))
        return dict(samples=self.samples, loaderAndDriverCandidates=images)


def compare(args):
    out = args.out.resolve()
    out.mkdir(parents=True)
    cwd = out / "same-path"
    cwd.mkdir()
    exe = cwd / "hello_world_pc.exe"
    output = cwd / "f02.bin"
    config, state = args.config.resolve(strict=True), args.state.resolve(strict=True)
    fixture_argv = ["--g13-candidate", str(output), "--g13-fixture", "F02", str(config), str(state),
                    "--g13-diagnostics", "hot"]
    build = args.exe.resolve(strict=True)
    other = args.other.resolve(strict=True) if args.other else build
    builds = {"A": build, "B": other}
    launch = {"A": fixture_argv, "B": fixture_argv}
    if args.arm == "snapshot":
        launch["B"] = [*fixture_argv, "--g13-slot-snapshot"]
    elif args.arm == "scope":
        launch["A"] = ["--qualify-steady-role", "OP_48"]
    if args.compile_trace:
        launch = {key: [*value, "--g13-compile-trace"] for key, value in launch.items()}
    run_info = dict(comparison=args.arm, executionOrder="ABBA", inputs={str(p): digest(p) for p in (config, state)},
                    sourceImages={key: dict(path=str(path), sha256=digest(path)) for key, path in builds.items()},
                    driverCacheIsolation="UNKNOWN",
                    environment={key: os.environ.get(key) for key in (
                        "VK_DRIVER_FILES", "VK_ICD_FILENAMES", "VK_ADD_DRIVER_FILES", "VK_LOADER_DRIVERS_SELECT",
                        "VK_LOADER_DRIVERS_DISABLE", "VK_INSTANCE_LAYERS", "VK_LAYER_PATH", "VK_ADD_LAYER_PATH",
                        "VK_LOADER_DEBUG")})
    (out / "comparison.json").write_text(json.dumps(run_info, indent=2) + "\n", encoding="utf8")
    for i, key in enumerate("ABBA", 1):
        arm = out / f"{i:02}-{key}"
        arm.mkdir()
        shutil.copyfile(builds[key], exe)
        if digest(exe) != run_info["sourceImages"][key]["sha256"]:
            raise ValueError("candidate executable changed during comparison")
        row = supervise(exe, "OP_48", arm, False, stack_dumps=True, argv=launch[key],
                        cwd=cwd, observe_modules=LoadedImages(), watched_role="OP_48")
        if output.is_file():
            output.rename(arm / output.name)
        print(key, row["observation"], row["reason"], row["exitCode"], flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("arm", choices=("pe", "snapshot", "scope"))
    parser.add_argument("exe", type=Path, help="frozen A executable, or the shared executable")
    parser.add_argument("config", type=Path)
    parser.add_argument("state", type=Path)
    parser.add_argument("out", type=Path, help="a new evidence directory")
    parser.add_argument("--other", type=Path, help="frozen B executable for pe comparison")
    parser.add_argument("--compile-trace", action="store_true", help="enable expanded receipts on the T916 PE")
    arguments = parser.parse_args()
    if (arguments.arm == "pe") != bool(arguments.other) or (arguments.arm == "pe" and arguments.compile_trace):
        parser.error("pe requires --other and uses the common CLI; the other arms run one PE")
    compare(arguments)
