# P-013 P1-G0 platform probe: five platform probes on a real device
#   1 surface lifecycle   2 sRGB attachment   3 three-layer z-order
#   4 transparency        5 input pass-through
# lib=hello    -> libp013hello.so (constant clear color, P0a proven)
# lib=renderer -> libp013renderer.so (P1 diagnostic pass; frames currently rejected by native)
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\p013-p1g0-probe-20261025'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null

function Start-Probe($lib, $hit, $diag) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps p013g0 1 --ps p013g0lib $lib --ps p013g0hit $hit --ps p013g0diag $diag" | Out-Null
  Start-Sleep -Seconds 6
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null   # dismiss USB popup
  Start-Sleep -Seconds 6
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/g0-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/g0-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump-G0($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'P013G0|P013Hello|OnSurface|EGL init' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}

# ---------- P1: hello + hit=none  (sRGB + z-order + input baseline) ----------
Start-Probe 'hello' 'none' 'true'
Start-Sleep -Seconds 4
Shot 'P1-hello-hitnone'
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 2
Dump-G0 'P1-hello-hitnone-g0.txt'
Shot 'P1-hello-hitnone-after-swipe'
Write-Output "P1 done"

# ---------- P2: hello + hit=default  (does the XComponent swallow touches?) ----------
Start-Probe 'hello' 'default' 'true'
Start-Sleep -Seconds 4
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 2
Dump-G0 'P2-hello-hitdefault-g0.txt'
Shot 'P2-hello-hitdefault-after-swipe'
Write-Output "P2 done"

# ---------- P3: hello + hit=transparent ----------
Start-Probe 'hello' 'transparent' 'true'
Start-Sleep -Seconds 4
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 2
Dump-G0 'P3-hello-hittransparent-g0.txt'
Shot 'P3-hello-hittransparent-after-swipe'
Write-Output "P3 done"

# ---------- P4: surface lifecycle - 10x background/foreground in one instance ----------
Start-Probe 'hello' 'none' 'true'
Start-Sleep -Seconds 3
for ($k = 1; $k -le 10; $k++) {
  & $hdc shell 'uitest uiInput keyEvent Home' | Out-Null
  Start-Sleep -Milliseconds 900
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps p013g0 1 --ps p013g0lib hello --ps p013g0hit none --ps p013g0diag true" | Out-Null
  Start-Sleep -Milliseconds 1500
}
Start-Sleep -Seconds 3
Dump-G0 'P4-hello-surface10bg-g0.txt'
Shot 'P4-hello-surface10bg'
Write-Output "P4 done"

# ---------- P5: renderer path (P1 diagnostic pass) - frame submission evidence ----------
Start-Probe 'renderer' 'none' 'true'
Start-Sleep -Seconds 6
Shot 'P5-renderer-diagon'
& $hdc shell 'uitest uiInput swipe 300 1400 900 1400 600' | Out-Null
Start-Sleep -Seconds 3
Dump-G0 'P5-renderer-diagon-g0.txt'
Shot 'P5-renderer-diagon-after-swipe'
Write-Output "P5 done"
Write-Output "DONE"
