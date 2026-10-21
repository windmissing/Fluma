#!/usr/bin/env python3
"""P-012 PERF 动态复核（密采样）：swipe 后立刻以~0.15s 间隔连拍 24 帧，
覆盖沙粒滑落（安息角到达）之前的运动窗口，再对沙带做互相关平流量测量。

背景：run_p012_eab_dynamic.py 的 0.4s 间隔在本次实测中沙堆于~1s 内即滑到
安息角静止（帧间逐位相同），运动窗口被错过，无法测到纹理平流。本脚本把
间隔压到 0.15s 并加长帧数，专门抓运动段。

输出：test/artifacts/p012-measurements/adv-s99-tilt-{i}.jpeg
     test/artifacts/p012-measurements/adv-s99-hilog.txt
"""
import subprocess, time, os, sys

HDC = r"C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
OUT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
TAG = os.environ.get("P012_TAG", "s99")
N = int(os.environ.get("P012_FRAMES", "24"))
GAP = float(os.environ.get("P012_GAP", "0.15"))
Y0, Y1 = 1400, 1700#沙带（含颗粒纹理）纵向范围


def sh(cmd, timeout=120):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return (r.stdout or "") + (r.stderr or "")


def snap(local):
    sh(f'"{HDC}" shell "snapshot_display -f /data/local/tmp/x.jpeg"', timeout=60)
    sh(f'"{HDC}" file recv /data/local/tmp/x.jpeg "{local}"', timeout=60)
    ok = os.path.exists(local) and os.path.getsize(local) > 1000
    print("  snap", os.path.basename(local), "OK" if ok else "FAIL", flush=True)
    return ok


def main():
    os.makedirs(OUT, exist_ok=True)
    sh(f'"{HDC}" shell power-shell timeout -o 600000')
    sh(f'"{HDC}" shell aa force-stop com.example.helloworld')
    time.sleep(1.5)
    sh(f'"{HDC}" shell aa start -a EntryAbility -b com.example.helloworld '
       f'--ps rendermode 4 --ps p012tier 3 --ps p012seed 0 --ps testmode true')
    time.sleep(2.0)
    sh(f'"{HDC}" shell uitest uiInput click 240 620')
    print("settle 30s ...", flush=True)
    time.sleep(30.0)
    print("=== 交错：snap / swipe / snap ... ===", flush=True)
    # 关键：snapshot_display 单帧耗时约1.4s，若"先 swipe 再连拍"则运动
    # 窗口（实测约2-3s）会在第2~3 帧后就已结束，后续帧全静止。
    # 故把 swipe 插在第 2 与第 3 次快照之间：hdc 调用串行返回，但 swipe
    # 命令在设备侧异步执行，正好落在连拍序列中间。
    for i in range(N):
        if i == 2:
            print("  >>> swipe now (async on device) <<<", flush=True)
            sh(f'"{HDC}" shell uitest uiInput swipe 100 1600 1200 1600 400', timeout=60)
        snap(os.path.join(OUT, f"adv-{TAG}-tilt-{i}.jpeg"))
        if i < N - 1:
            time.sleep(GAP)
    log = sh(f'"{HDC}" shell "hilog -x -T P012"', timeout=60)
    with open(os.path.join(OUT, f"adv-{TAG}-hilog.txt"), "w", encoding="utf-8") as f:
        f.write(log)
    sh(f'"{HDC}" shell power-shell timeout -r')
    print("DONE", flush=True)


if __name__ == "__main__":
    main()
