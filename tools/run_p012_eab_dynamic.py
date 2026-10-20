#!/usr/bin/env python3
"""P-012 EAB dynamic no-sticker check (s96): tilt gesture + frame burst.

Proves the B-grain texture follows material motion (no static screen sticker):
  1) launch EAB (tier 3, seed 0, testmode), settle 35s
  2) burst 8 static frames (0.4s apart) -- static stability
  3) full-width swipe (100->1200) to tilt ~6-20deg (uitest nonlinear attenuation)
  4) burst 16 frames during the tilt response -- dynamic follow
  5) hilog offsets (offH/offL/offM) around the tilt window
Outputs: test/artifacts/p012-measurements/dyn-s96-{static,tilt}-{i}.jpeg
         dyn-s96-hilog.txt, dyn-s96-motion.gif (box crop, 1/4 scale)
"""
import subprocess, sys, time, os

HDC = r"C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
OUT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"

def sh(cmd, timeout=120):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return (r.stdout or "") + (r.stderr or "")

def snap(local):
    sh(f'"{HDC}" shell "snapshot_display -f /data/local/tmp/x.jpeg"', timeout=60)
    sh(f'"{HDC}" file recv /data/local/tmp/x.jpeg "{local}"', timeout=60)
    ok = os.path.exists(local) and os.path.getsize(local) > 1000
    print("  snap ->", os.path.basename(local), "OK" if ok else "FAIL", flush=True)
    return ok

def burst(tag, n, gap=0.4):
    oks = []
    for i in range(n):
        oks.append(snap(os.path.join(OUT, f"dyn-s96-{tag}-{i}.jpeg")))
        if i < n - 1:
            time.sleep(gap)
    return all(oks)

def main():
    os.makedirs(OUT, exist_ok=True)
    sh(f'"{HDC}" shell power-shell timeout -o 600000')
    sh(f'"{HDC}" shell aa force-stop com.example.helloworld')
    time.sleep(1.5)
    sh(f'"{HDC}" shell aa start -a EntryAbility -b com.example.helloworld '
       f'--ps rendermode 4 --ps p012tier 3 --ps p012seed 0 --ps testmode true')
    time.sleep(2.0)
    sh(f'"{HDC}" shell uitest uiInput click 240 620')
    print("settling 35s ...", flush=True)
    time.sleep(35.0)
    print("=== static burst (8) ===", flush=True)
    if not burst("static", 8):
        print("static burst FAILED", flush=True); sys.exit(1)
    print("=== tilt: full-width swipe 100->1200 ===", flush=True)
    sh(f'"{HDC}" shell uitest uiInput swipe 100 1400 1200 1400', timeout=30)
    print("=== tilt burst (16) ===", flush=True)
    if not burst("tilt", 16):
        print("tilt burst FAILED", flush=True); sys.exit(1)
    log = sh(f'"{HDC}" shell "hilog -x -T P012"', timeout=60)
    with open(os.path.join(OUT, "dyn-s96-hilog.txt"), "w", encoding="utf-8") as f:
        f.write(log)
    print("hilog lines:", len(log.splitlines()), flush=True)
    sh(f'"{HDC}" shell power-shell timeout -r')
    print("DONE", flush=True)

if __name__ == "__main__":
    main()
