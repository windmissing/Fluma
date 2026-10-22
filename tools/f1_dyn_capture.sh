#!/bin/bash
# P-012 F1 消融配对块采集（Bash 版，避开 PowerShell 管道挂起）
HDC="C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/toolchains/hdc.exe"
OUT="D:/hmprojects/HelloWorld/test/artifacts/p012-measurements"

run_block() {
  local name=$1 ablate=$2
  "$HDC" shell hilog -r > /dev/null 2>&1
  "$HDC" shell aa force-stop com.example.helloworld > /dev/null 2>&1
  sleep 1
  "$HDC" shell aa start -a EntryAbility -b com.example.helloworld --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps p012ablate $ablate --ps testmode true > /dev/null 2>&1
  sleep 3
  "$HDC" shell uitest uiInput click 240 620 > /dev/null 2>&1
  sleep 2
  sleep 6              # 静置段
  "$HDC" shell uitest uiInput swipe 100 1500 1200 1500 600 > /dev/null 2>&1
  sleep 6              # 慢倾段
  "$HDC" shell uitest uiInput swipe 1200 1500 100 1500 600 > /dev/null 2>&1
  sleep 4              # 回正段
  "$HDC" shell "hilog -x | grep WS_FRAME" > "$OUT/f1dyn-$name-hilog.txt" 2>/dev/null
  echo "block $name: $(grep -c WS_FRAME "$OUT/f1dyn-$name-hilog.txt") frames"
}

run_block "s132-off" "off"
run_block "s132-dyn" "dyn"
echo DONE
