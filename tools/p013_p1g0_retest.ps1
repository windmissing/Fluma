# P-013 P1-G0 补证复测（T35）：T32 帧桥诊断 + T34 TEXTURE 合成落地后的五项闭环
#   ①帧提交 ②sRGB（显式 colorspace + GL_BACK 编码）③三层 z 序 ④透明（二值 alpha）⑤输入透传
#  arms: R0 hello 基线 / R1 renderer+TEXTURE 主臂 / R2 renderer+SURFACE 对照 /
#        R3 hit=transparent / R4 hit=default / R5 diag=off 同帧 ROI / R6 前后台 10 次 / R7 动态错帧
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\p013-p1g0-retest-20261025'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'retest start'

function Start-Probe($arm, $lib, $profile, $hit, $diag) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps p013g0 1 --ps p013g0lib $lib --ps p013g0profile $profile --ps p013g0hit $hit --ps p013g0diag $diag" | Out-Null
  Start-Sleep -Seconds 6
  Log-Step "started arm lib=$lib profile=$profile hit=$hit diag=$diag"
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null   # dismiss USB popup
  Start-Sleep -Seconds 6
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/r-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/r-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump-G0($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'P013G0|P013Hello|OnSurface|EGL init' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}

# ---------- R0: hello + TEXTURE + hit=none（sRGB 基线 + z 序回归；hello 清屏色走纹理路径） ----------
Start-Probe 'R0' 'hello' 'texture' 'none' 'true'
Start-Sleep -Seconds 4
Shot 'R0-hello-texture'
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 2
Dump-G0 'R0-hello-texture-g0.txt'
Shot 'R0-hello-texture-after-swipe'
Log-Step 'R0 shot+dump done'
Write-Output "R0 done"

# ---------- R1: renderer + TEXTURE + hit=none + diag=on（主臂：帧提交/合成收据/读回/z 序/透明） ----------
Start-Probe 'R1' 'renderer' 'texture' 'none' 'true'
Start-Sleep -Seconds 8
Shot 'R1-renderer-texture'
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 3
Shot 'R1-renderer-texture-after-swipe'
Dump-G0 'R1-renderer-texture-g0.txt'
Log-Step 'R1 shot+dump done'
Write-Output "R1 done"

# ---------- R2: renderer + SURFACE + hit=none（SURFACE 对照臂：叠层结构差异取证） ----------
Start-Probe 'R2' 'renderer' 'surface' 'none' 'true'
Start-Sleep -Seconds 8
Shot 'R2-renderer-surface'
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 3
Shot 'R2-renderer-surface-after-swipe'
Dump-G0 'R2-renderer-surface-g0.txt'
Log-Step 'R2 shot+dump done'
Write-Output "R2 done"

# ---------- R3: renderer + TEXTURE + hit=transparent（输入透传：期望 2 次 Down） ----------
Start-Probe 'R3' 'renderer' 'texture' 'transparent' 'true'
Start-Sleep -Seconds 6
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 3
Dump-G0 'R3-renderer-texture-hittransparent-g0.txt'
Shot 'R3-renderer-texture-hittransparent'
Log-Step 'R3 done'
Write-Output "R3 done"

# ---------- R4: renderer + TEXTURE + hit=default（输入负对照：期望 0 次 Down） ----------
Start-Probe 'R4' 'renderer' 'texture' 'default' 'true'
Start-Sleep -Seconds 6
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 3
Dump-G0 'R4-renderer-texture-hitdefault-g0.txt'
Shot 'R4-renderer-texture-hitdefault'
Log-Step 'R4 done'
Write-Output "R4 done"

# ---------- R5: renderer + TEXTURE + diag=off（与 R1 同静态帧比对非沙 ROI） ----------
Start-Probe 'R5' 'renderer' 'texture' 'none' 'false'
Start-Sleep -Seconds 8
Shot 'R5-renderer-texture-diagoff'
Dump-G0 'R5-renderer-texture-diagoff-g0.txt'
Log-Step 'R5 done'
Write-Output "R5 done"

# ---------- R6: 前后台 10 次循环（TEXTURE profile 生命周期回归） ----------
Start-Probe 'R6' 'renderer' 'texture' 'none' 'true'
Start-Sleep -Seconds 4
for ($k = 1; $k -le 10; $k++) {
  & $hdc shell 'uitest uiInput keyEvent Home' | Out-Null
  Start-Sleep -Milliseconds 900
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps p013g0 1 --ps p013g0lib renderer --ps p013g0profile texture --ps p013g0hit none --ps p013g0diag true" | Out-Null
  Start-Sleep -Milliseconds 1500
}
Start-Sleep -Seconds 3
Dump-G0 'R6-renderer-texture-surface10bg-g0.txt'
Shot 'R6-renderer-texture-surface10bg'
Log-Step 'R6 done'
Write-Output "R6 done"

# ---------- R7: 动态错帧（连续 swipe + 密集连拍，帧 token 与交换时序） ----------
Start-Probe 'R7' 'renderer' 'texture' 'none' 'true'
Start-Sleep -Seconds 6
for ($k = 1; $k -le 6; $k++) {
  & $hdc shell 'uitest uiInput swipe 300 1400 900 1400 300' | Out-Null
  Start-Sleep -Milliseconds 400
  Shot ("R7-renderer-texture-dyn-" + $k)
}
Dump-G0 'R7-renderer-texture-dyn-g0.txt'
Log-Step 'R7 done'
Write-Output "R7 done"
Write-Output "DONE"
