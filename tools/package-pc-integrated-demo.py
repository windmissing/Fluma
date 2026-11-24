"""Create one pinned Windows Demo candidate for T935."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil


MODULES = {
    "full_source.spv": "e9c7c123747c77350545b9d11abcfbd9b0b2e85948cde854f2592b97957f292c",
    "quantum_candidate.spv": "bf024ac5c580be0549aee02369a58bdb7b500ea3165b2588922936331edb285f",
    "quantum_commit.spv": "64ba25f41818b8c55fc117ce44fce52b5a55bdd0ebc5e3d458a9bb1380f43a01",
}
EXE = "hello_world_pc.exe"
FLAGS = ["--m1-compact-demo", "--m1-integrated", "--m1-dual-source"]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def package(exe, shaders, out, revision="T942"):
    expected = {name: shaders / name for name in MODULES}
    for name, path in expected.items():
        if sha(path) != MODULES[name]:
            raise ValueError(f"Integrated module mismatch: {name}")
    with exe.open("rb") as source:
        if source.read(2) != b"MZ":
            raise ValueError("Expected a Windows PE executable")
    view_receipt = exe.parent / "g13_assets" / "pc_view_assets.json"
    embedded = None
    if revision in ("T942", "T960"):
        view = json.loads(view_receipt.read_bytes())
        embedded = {name: view[name] for name in ("vert", "m1_dry_scene.frag", "m1_full_scene.frag")}
    out.mkdir()
    files = {EXE: exe, **expected}
    for name, source in files.items():
        shutil.copyfile(source, out / name)
    flags = FLAGS if revision == "T960" else [*FLAGS, "--m1-source-trace"]
    mode = (
        'if not "%~2"=="" exit /b 2\r\n'
        'set "DEMO_OBSERVE="\r\n'
        'if "%~1"=="" goto launch\r\n'
        'if /i not "%~1"=="observe" exit /b 2\r\n'
        'set DEMO_OBSERVE=--m1-observe-birth 8 "%DEMO_LOG%\\birth"\r\n'
        ':launch\r\n'
    ) if revision == "T960" else ""
    cmd = "@echo off\r\n" + (
        "setlocal\r\n"
        "cd /d \"%~dp0\" || exit /b 1\r\n"
        "set \"DEMO_LOG=%TEMP%\\HelloWorld-Demo-%RANDOM%-%RANDOM%\"\r\n"
        "mkdir \"%DEMO_LOG%\" || exit /b 1\r\n"
        "powershell.exe -NoProfile -Command \"$ErrorActionPreference='Stop'; "
        "$m=Get-Content -Raw -LiteralPath 'manifest.json' | ConvertFrom-Json; "
        "if($m.files.Count -ne 5){exit 2}; foreach($f in $m.files){"
        "if((Get-FileHash -Algorithm SHA256 -LiteralPath $f.name).Hash -ne $f.sha256){exit 2}}\""
        " || (echo Demo files do not match manifest.json. & pause & exit /b 2)\r\n"
        "copy /y manifest.json \"%DEMO_LOG%\\manifest.json\" >nul\r\n"
        "echo Log: %DEMO_LOG%\r\n" + mode +
        f"\"%~dp0{EXE}\" --m1-window " + " ".join(f'"%~dp0{name}"' for name in MODULES) +
        " " + " ".join(flags) + (" %DEMO_OBSERVE%" if revision == "T960" else "") +
        ' >"%DEMO_LOG%\\demo.jsonl" 2>"%DEMO_LOG%\\stderr.txt"\r\n'
        "exit /b %ERRORLEVEL%\r\n"
    )
    (out / "Start Demo.cmd").write_bytes(cmd.encode("ascii"))
    document = {
        "schema": f"{revision}_DEMO_INTEGRATED_V1",
        "qualification": "SAME_PACKAGE_PC_PENDING" if revision == "T960" else "T935_PENDING",
        "dispatchesPerQuantum": 35,
        "argv": [EXE, "--m1-window", *MODULES, *flags],
        "files": [{"name": name, "bytes": (out / name).stat().st_size, "sha256": sha(out / name)}
                  for name in (*files, "Start Demo.cmd")],
    }
    if embedded is not None:
        document["embeddedViewSPIRV"] = embedded
    if revision == "T960":
        document["controlledObservation"] = {
            "launcherArg": "observe", "argvSuffix": ["--m1-observe-birth", "8", "<log-directory>/birth"]}
    (out / "manifest.json").write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("shaders", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("--revision", choices=("T933", "T942", "T960"), default="T942")
    args = parser.parse_args()
    package(args.exe, args.shaders, args.out, args.revision)
