#!/bin/bash
# P-012 帧时三段拆分采集（P-013 立项前置数据）
# 三场景（静置/普通倾斜/重载动态）× OFF/ON（p012ablate=toplight）各 30s 窗
# 设备须空闲（108-run 批后）；physicsTime 哨兵过滤热态帧（F1 §2.1 教训）
HDC="C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/toolchains/hdc.exe"
OUT="D:/hmprojects/HelloWorld/test/artifacts/p012-measurements"
TAG="${P012_TAG:-s134}"

run_block() {
  local scene=$1 abl=$2 dur=$3
  "$HDC" shell hilog -r > /dev/null 2>&1
  "$HDC" shell aa force-stop com.example.helloworld > /dev/null 2>&1
  sleep 2
  local ABLARG=""
  if [ "$abl" = "toplight" ]; then ABLARG="--ps p012ablate toplight"; fi
  "$HDC" shell aa start -a EntryAbility -b com.example.helloworld --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 $ABLARG --ps testmode true > /dev/null 2>&1
  sleep 3
  "$HDC" shell uitest uiInput click 240 620 > /dev/null 2>&1   # 关 USB 弹窗
  sleep 2
  case $scene in
    settle)
      ;;                                                   # 静置 30s
    tilt)
      "$HDC" shell uitest uiInput swipe 1200 1500 100 1500 8000 > /dev/null 2>&1  # 8s 慢倾
      ;;
    heavy)
      "$HDC" shell uitest uiInput swipe 100 1500 1200 1500 3000 > /dev/null 2>&1  # 快倾
      sleep 4
      "$HDC" shell uitest uiInput swipe 1200 1500 100 1500 3000 > /dev/null 2>&1  # 快回
      ;;
  esac
  sleep "$dur"
  "$HDC" shell "hilog -x | grep WS_FRAME" > "$OUT/seg3-$TAG-$scene-$abl-hilog.txt" 2>/dev/null
  echo "block $scene/$abl: $(grep -c WS_FRAME "$OUT/seg3-$TAG-$scene-$abl-hilog.txt") frames"
  sleep 25                                                  # 块间冷却（热态哨兵）
}

for abl in off toplight; do
  run_block settle  "$abl" 30
  run_block tilt    "$abl" 24
  run_block heavy   "$abl" 22
done
echo DONE
