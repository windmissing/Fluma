#!/usr/bin/env python3
"""P-012 §10.4 ablation want-param on-device spot check.

Purpose (task 6ad85628e4b04e4b613c080a DoD):
  p012ablate=grain (EAB tier) must reproduce tier2-level FPS — confirming the
  grain layer is the only remaining cost source above EA (formal evidence
  replacing the substitute evidence in test-report-P012-v1.md §3).

Variants (each: force-stop -> launch with want params -> 25s settle ->
P012_WINDOWS snapshots reading the on-screen FPS overlay):
  ablate-off-eab : tier=3 ablate=off     (EAB full, baseline)
  ablate-grain   : tier=3 ablate=grain   (grain layer skipped) -> expect ~tier2
  tier2-ea       : tier=2                (EA reference level)
  fillbox-full   : tier=3 ablate=off fillbox=full (v1 whole-box tiling vs tight)

Usage:
  python tools/run_p012_ablate_check.py            # all four variants
  P012_VARIANTS=grain,tier2 python ...             # subset
Output: test/artifacts/p012-measurements/ablate-<TAG>-<variant>-w<i>.jpeg
"""
import subprocess, sys, time, os

HDC = r"C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
OUT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
TAG = os.environ.get("P012_TAG", "s102")
WINDOWS = int(os.environ.get("P012_WINDOWS", "4"))
GAP = float(os.environ.get("P012_GAP", "1.0"))
SETTLE = float(os.environ.get("P012_SETTLE", "25.0"))

# (variant-name, tier, ablate, fillbox)
VARIANTS = [
    ("ablate-off-eab", 3, "off", ""),
    ("ablate-grain",   3, "grain", ""),
    ("tier2-ea",       2, "", ""),
    ("fillbox-full",   3, "off", "full"),
]
_only = os.environ.get("P012_VARIANTS", "")
if _only:
    _keep = set(x.strip() for x in _only.split(",") if x.strip())
    VARIANTS = [v for v in VARIANTS if v[0] in _keep]

def sh(cmd, timeout=120):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return (r.stdout or "") + (r.stderr or "")

def snap(local):
    sh(f'"{HDC}" shell "snapshot_display -f /data/local/tmp/x.jpeg"', timeout=60)
    sh(f'"{HDC}" file recv /data/local/tmp/x.jpeg "{local}"', timeout=60)
    ok = os.path.exists(local) and os.path.getsize(local) > 1000
    print("  snap ->", os.path.basename(local), "OK" if ok else "FAIL", flush=True)
    return ok

def main():
    os.makedirs(OUT, exist_ok=True)
    sh(f'"{HDC}" shell power-shell timeout -o 600000')
    for name, tier, ablate, fillbox in VARIANTS:
        print(f"=== {name} (tier={tier} ablate={ablate or '-'} "
              f"fillbox={fillbox or '-'}) ===", flush=True)
        sh(f'"{HDC}" shell aa force-stop com.example.helloworld')
        time.sleep(1.5)
        want = (f'--ps rendermode 4 --ps p012tier {tier} '
                f'--ps p012seed 20261020 --ps testmode true')
        if ablate:
            want += f' --ps p012ablate {ablate}'
        if fillbox:
            want += f' --ps p012fillbox {fillbox}'
        sh(f'"{HDC}" shell aa start -a EntryAbility '
           f'-b com.example.helloworld {want}')
        time.sleep(2.0)
        sh(f'"{HDC}" shell uitest uiInput click 240 620')   # dismiss USB dialog
        time.sleep(SETTLE)
        for i in range(WINDOWS):
            if not snap(os.path.join(OUT, f"ablate-{TAG}-{name}-w{i}.jpeg")):
                print(f"{name} snapshot FAILED", flush=True)
            if i < WINDOWS - 1:
                time.sleep(GAP)
    sh(f'"{HDC}" shell power-shell timeout -r')
    print("DONE", flush=True)

if __name__ == "__main__":
    main()
