# P-013 P1-G0 surface lifecycle capture: per-cycle hilog dump (avoid ring-buffer roll-off)
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\p013-p1g0-probe-20261025'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue

function Dump-Surface($file) {
  & $hdc shell 'hilog -x' 2>$null |
    Select-String 'OnSurfaceCreated|OnSurfaceChanged|OnSurfaceDestroyed|EGL init|EGL destroy|hello 帧|surface destroyed|createSurface|destroySurface' |
    ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}

& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps p013g0 1 --ps p013g0lib hello --ps p013g0hit none --ps p013g0diag true" | Out-Null
Start-Sleep -Seconds 6
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 5
Dump-Surface 'P4b-surface-cycles.txt'
Write-Output "cycle 0 dumped"

for ($k = 1; $k -le 6; $k++) {
  & $hdc shell 'uitest uiInput keyEvent Home' | Out-Null
  Start-Sleep -Milliseconds 1200
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps p013g0 1 --ps p013g0lib hello --ps p013g0hit none --ps p013g0diag true" | Out-Null
  Start-Sleep -Milliseconds 2200
  Dump-Surface 'P4b-surface-cycles.txt'
  Write-Output "cycle $k dumped"
}
Start-Sleep -Seconds 2
& $hdc shell "snapshot_display -f /data/local/tmp/g0-P4b.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/g0-P4b.jpeg (Join-Path $out 'P4b-hello-after6bg.jpeg') | Out-Null
Write-Output "DONE"
