# T180+T187 联合窄补证（v2 纪律）
#   基线: 设备在装 s151 3c8abfae（entry/src 与 HEAD d9b4afb 零差异，含 T178+T185 诊断）
#   臂: C0 DEMO 静置基线(T187) / C1 SandCanvas 控制臂无注入(T180) / C2 SandCanvas 注入臂(T180)
#       C3 DEMO 注入臂(T187, T103 同款)
#   导出: WS_EVENT scene_material_rejection {part,parts,text} / first_physics_failure_v2 {单行整包}
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t180-t187-joint-20261028'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss.fff'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console-v2.log') -Value "[$t] $msg" }
function Run-Cmd($desc, $cmdline) {
    $o = & $cmdline 2>&1
    Add-Content -Path (Join-Path $out 'cmd-audit.log') -Value "[$(Get-Date -Format 'HH:mm:ss.fff')] DESC=$desc EXIT=$LASTEXITCODE CMD=$cmdline OUT=$($o -join ' | ')"
    return $o
}
function Get-AppPid { $o = Run-Cmd 'pidof' { & $hdc shell "pidof $bundle" }; return (($o -join ' ').Trim() -split '\s+')[0] }
function Dump-Full($file) { $o = Run-Cmd "hilog dump -> $file" { & $hdc shell 'hilog -x' }; $o | Out-File (Join-Path $out $file) -Encoding utf8 }
function Shot($n) {
    Run-Cmd "snapshot $n" { & $hdc shell "snapshot_display -f /data/local/tmp/t180187-$n.jpeg" }
    Run-Cmd "recv $n" { & $hdc file recv /data/local/tmp/t180187-$n.jpeg (Join-Path $out "$n.jpeg") }
}
Log-Step 'T180+T187 joint start (s151 3c8abfae in place; entry/src == d9b4afb)'

function Cold-Start($arm, $pageArgs) {
    $oldPid = Get-AppPid
    Log-Step "$arm freeze oldPid=$oldPid"
    if ($oldPid) { Dump-Full "$arm-pre-freeze.txt" }
    Run-Cmd "$arm force-stop" { & $hdc shell "aa force-stop $bundle" }
    Start-Sleep -Seconds 2
    Run-Cmd "$arm start" { & $hdc shell "aa start -a EntryAbility -b $bundle $pageArgs" }
    Start-Sleep -Seconds 6
    & $hdc shell 'uitest uiInput click 240 620' | Out-Null
    Start-Sleep -Seconds 2
    $newPid = Get-AppPid
    Log-Step "$arm newPid=$newPid"
    Dump-Full "$arm-identity.txt"
    return $newPid
}

# ---------- C0: DEMO 静置基线（T187 健康基线） ----------
Log-Step 'C0 DEMO static baseline'
$c0pid = Cold-Start 'C0' '--ps emptysanddemo 1'
Start-Sleep -Seconds 15
Shot 'C0-static'
Dump-Full 'C0-full.txt'
$c0end = Get-AppPid
Log-Step "C0 end pid=$c0end (was $c0pid) — attribution $(if($c0end -eq $c0pid){'OK'}else{'DRIFT'})"

# ---------- C1: SandCanvas 控制臂（T180 无注入） ----------
Log-Step 'C1 SandCanvas control (no injection)'
$c1pid = Cold-Start 'C1' '--ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false --ps shake 0'
Start-Sleep -Seconds 15
Shot 'C1-static'
Dump-Full 'C1-full.txt'
$c1end = Get-AppPid
Log-Step "C1 end pid=$c1end (was $c1pid) — attribution $(if($c1end -eq $c1pid){'OK'}else{'DRIFT'})"

# ---------- C2: SandCanvas 注入臂（T180 触发） ----------
Log-Step 'C2 SandCanvas injection (shake=0 + 2x swipe)'
$c2pid = Cold-Start 'C2' '--ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false --ps shake 0'
for ($i=1; $i -le 2; $i++) {
    & $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
    Start-Sleep -Seconds 3
    Log-Step "C2 swipe $i done"
}
Start-Sleep -Seconds 15
Shot 'C2-injected'
Dump-Full 'C2-full.txt'
$c2end = Get-AppPid
Log-Step "C2 end pid=$c2end (was $c2pid)"

# ---------- C3: DEMO 注入臂（T187 触发，T103 同款） ----------
Log-Step 'C3 DEMO injection (4x swipe, T103 sequence)'
$c3pid = Cold-Start 'C3' '--ps emptysanddemo 1'
Start-Sleep -Seconds 4
for ($i=1; $i -le 4; $i++) {
    & $hdc shell 'uitest uiInput swipe 400 1200 850 1200 2000' | Out-Null
    Start-Sleep -Seconds 3
    Log-Step "C3 swipe $i done"
}
Start-Sleep -Seconds 15
Shot 'C3-settled'
Dump-Full 'C3-full.txt'
$c3end = Get-AppPid
Log-Step "C3 end pid=$c3end (was $c3pid)"

& $hdc shell 'power-shell -r' | Out-Null
Log-Step 'ALL DONE'
Write-Output 'DONE'
