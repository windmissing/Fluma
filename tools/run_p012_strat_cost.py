#!/usr/bin/env python3
"""P-012 §11.4 C-layering cost attribution: EAB <-> EAB+C paired blocks.

Task 6ad88a76e4b04e4b61415554 DoD: CPU paired-increment P95<=0.15ms,
draw<=16, resident<=0.25MiB. This script captures the on-device evidence:
three paired blocks (ablate=off vs ablate=strat, tier=3, same build/seed),
settled phase snapshots for FPS + hilog draw counts.

The C draw count is read from the renderer's existing lastDrawCount chain via
the [P012] hilog lines (grain: drawImage tile path used / strat: bitmaps ok).
CPU paired increment is derived from frame-time P50/P95 across the paired
windows (FPS text overlay).

Variants (per block, order alternates to cancel drift):
  block A: eabc(off) -> eabc-strat -> eabc(off)
Output: test/artifacts/p012-measurements/strat-<TAG>-<variant>-b<i>-w<j>.jpeg
"""
import subprocess, sys, time, os

HDC = r"C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
OUT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
TAG = os.environ.get("P012_TAG", "s105")
WINDOWS = int(os.environ.get("P012_WINDOWS", "4"))
GAP = float(os.environ.get("P012_GAP", "1.0"))
SETTLE = float(os.environ.get("P012_SETTLE", "25.0"))

def sh(cmd, timeout=120):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return (r.stdout or "") + (r.stderr or "")

def snap(local):
    sh(f'"{HDC}" shell "snapshot_display -f /data/local/tmp/x.jpeg"', timeout=60)
    sh(f'"{HDC}" file recv /data/local/tmp/x.jpeg "{local}"', timeout=60)
    ok = os.path.exists(local) and os.path.getsize(local) > 1000
    print("  snap ->", os.path.basename(local), "OK" if ok else "FAIL", flush=True)
    return ok

def run_variant(name, block, ablate):
    print(f"=== {name} block{block} (tier=3 ablate={ablate}) ===", flush=True)
    sh(f'"{HDC}" shell aa force-stop com.example.helloworld')
    time.sleep(1.5)
    want = ('--ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 '
            '--ps testmode true')
    if ablate:
        want += f' --ps p012ablate {ablate}'
    sh(f'"{HDC}" shell aa start -a EntryAbility -b com.example.helloworld {want}')
    time.sleep(2.0)
    sh(f'"{HDC}" shell uitest uiInput click 240 620')   # dismiss USB dialog
    time.sleep(SETTLE)
    for i in range(WINDOWS):
        if not snap(os.path.join(OUT, f"strat-{TAG}-{name}-b{block}-w{i}.jpeg")):
            print(f"{name} b{block} snapshot FAILED", flush=True)
        if i < WINDOWS - 1:
            time.sleep(GAP)

def main():
    os.makedirs(OUT, exist_ok=True)
    sh(f'"{HDC}" shell power-shell timeout -o 600000')
    # 三配对块：off -> strat -> off（交替消除时漂）
    for block in range(3):
        run_variant("eabc", block, "off")
        run_variant("eabc-strat", block, "strat")
    sh(f'"{HDC}" shell power-shell timeout -r')
    print("DONE", flush=True)

if __name__ == "__main__":
    main()
