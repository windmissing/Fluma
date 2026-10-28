# T156 双场真实负载窄补证：PID锁定+先注入后finish+原码拉取（v2 纪律）
#   基线: 0cc0439 (T123索引+T166+T174列guard), s151 3c8abfae, 树指纹 f7fbec80d9d7eff8
#   臂: R0 冒烟 / R1 T94原卡死序列 / R2 加压 / R3 先注入后finish(原码完整拉取)
#   主判: 三臂不冻结 + 真实准入收据 + 双流 CLOSED + manifest COMPLETE + 原码完整
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t156-real-load-20261028'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss.fff'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console-v2.log') -Value "[$t] $msg" }
function Run-Cmd($desc, $cmdline) {
    $o = & $cmdline 2>&1
    $code = $LASTEXITCODE
    Add-Content -Path (Join-Path $out 'cmd-audit.log') -Value "[$(Get-Date -Format 'HH:mm:ss.fff')] DESC=$desc EXIT=$code CMD=$cmdline OUT=$($o -join ' | ')"
    return $o
}
function Get-AppPid { $o = Run-Cmd 'pidof' { & $hdc shell "pidof $bundle" }; return (($o -join ' ').Trim() -split '\s+')[0] }
function Dump-Full($file) { $o = Run-Cmd "hilog dump -> $file" { & $hdc shell 'hilog -x' }; $o | Out-File (Join-Path $out $file) -Encoding utf8 }
function Shot($n) {
    Run-Cmd "snapshot $n" { & $hdc shell "snapshot_display -f /data/local/tmp/t156-$n.jpeg" }
    Run-Cmd "recv $n" { & $hdc file recv /data/local/tmp/t156-$n.jpeg (Join-Path $out "$n.jpeg") }
}
Log-Step 'T156 start (build 0cc0439, s151 3c8abfae, tree f7fbec80d9d7eff8)'
& $hdc shell 'power-shell timeout -o 600000' | Out-Null

# ---- step1 冻结旧身份 ----
$oldPid = Get-AppPid
Log-Step "step1 oldPid=$oldPid"
if ($oldPid) { Dump-Full 'pre-freeze-old-tail-full.txt' }

# ---- step2 force-stop ----
Run-Cmd 'force-stop' { & $hdc shell "aa force-stop $bundle" }
Start-Sleep -Seconds 2
if (Get-AppPid) { Log-Step 'step2 WARN still alive' } else { Log-Step 'step2 old dead' }

# ---- step3 冷启 + 新身份 ----
Run-Cmd 'cold-start' { & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false --ps shake 0" }
Start-Sleep -Seconds 6
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 2
$newPid = Get-AppPid
Log-Step "step3 newPid=$newPid"
Dump-Full 'post-start-identity-full.txt'
$rl = Select-String -Path (Join-Path $out 'post-start-identity-full.txt') -Pattern '(p011-run-\d+-\d+)' | Select-Object -Last 1
$newRun = $rl.Matches[0].Groups[1].Value
Log-Step "step3 newRun=$newRun"
if ($newRun -match '1793010180197|1793100180197|1793057021611|1793064587384|1793064633964|1793100180197') { Log-Step 'step3 WARN: newRun matches a stale known runId!' }

# ---- step4 健康检查 ----
Start-Sleep -Seconds 8
Dump-Full 'precheck-full.txt'
$pf = Select-String -Path (Join-Path $out 'precheck-full.txt') -Pattern 'page-physics-failed' | Where-Object { $_.Line -match $newPid }
if ($pf) { Log-Step "step4 PREEXISTING_FAILED ($($pf.Count))"; Write-Output 'DONE (PREEXISTING_FAILED)'; exit 0 }
Log-Step 'step4 healthy'
Shot 'R0-healthy'

# ---- R1: T94 原卡死序列（shake=1 + 3swipe）----
Log-Step 'R1 shake=1 + 3x swipe'
# 改 shake 用面板按钮？——任务书允许重开臂。为保持 run 连续，改用第二冷启做 R1
Run-Cmd 'R1 cold start shake=1' { & $hdc shell "aa force-stop $bundle" }
Start-Sleep -Seconds 2
Run-Cmd 'R1 start' { & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false --ps shake 1" }
Start-Sleep -Seconds 6
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 2
$r1pid = Get-AppPid
$r1run = ''
Log-Step "R1 PID=$r1pid"
for ($i=1; $i -le 3; $i++) {
    & $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
    Start-Sleep -Seconds 2
    Shot "R1-swipe-$i"
    Log-Step "R1 swipe $i done"
}
Start-Sleep -Seconds 25
Shot 'R1-settled'
# PID 存活检查
$r1alive = Get-AppPid
if ($r1alive -eq $r1pid) { Log-Step "R1 alive PID=$r1pid (no freeze)" } else { Log-Step "R1 PROCESS DEAD (was $r1pid, now $r1alive)" }
Dump-Full 'R1-full.txt'
Log-Step 'R1 done'

# ---- R2: 加压（shake=0.6 + 6swipe）----
Log-Step 'R2 shake=0.6 + 6x swipe'
Run-Cmd 'R2 cold start' { & $hdc shell "aa force-stop $bundle" }
Start-Sleep -Seconds 2
Run-Cmd 'R2 start' { & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false --ps shake 0.6" }
Start-Sleep -Seconds 6
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 2
$r2pid = Get-AppPid
Log-Step "R2 PID=$r2pid"
for ($i=1; $i -le 6; $i++) {
    & $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
    Start-Sleep -Seconds 2
}
Start-Sleep -Seconds 20
Shot 'R2-settled'
$r2alive = Get-AppPid
if ($r2alive -eq $r2pid) { Log-Step "R2 alive PID=$r2pid" } else { Log-Step "R2 PROCESS DEAD (was $r2pid)" }
Dump-Full 'R2-full.txt'
Log-Step 'R2 done'

# ---- R3: shake=0 + 先注入后 finish（原码完整拉取主臂）----
Log-Step 'R3 shake=0 + 2x swipe -> finish (inject-first order)'
Run-Cmd 'R3 cold start' { & $hdc shell "aa force-stop $bundle" }
Start-Sleep -Seconds 2
Run-Cmd 'R3 start' { & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false --ps shake 0" }
Start-Sleep -Seconds 6
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 2
$r3pid = Get-AppPid
Log-Step "R3 PID=$r3pid"
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
Start-Sleep -Seconds 2
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
Start-Sleep -Seconds 8
Shot 'R3-injected'
# 获取 store 路径（finish 前从 P011_OBS_STORE 行）
Dump-Full 'R3-pre-finish.txt'
$fpLine = Select-String -Path (Join-Path $out 'R3-pre-finish.txt') -Pattern 'P011_OBS_STORE ' | Select-Object -First 1
# finish（前台按钮）
& $hdc shell 'uitest uiInput click 70 140' | Out-Null
Start-Sleep -Seconds 8
Dump-Full 'R3-post-finish.txt'
$closed = Select-String -Path (Join-Path $out 'R3-post-finish.txt') -Pattern 'P011_OBS_STORE_CLOSED' | Select-Object -First 1
if ($closed) { Log-Step "R3 CLOSED: $($closed.Line.Substring([Math]::Max(0,$closed.Line.Length-260)))" } else { Log-Step 'R3 CLOSED NOT FOUND' }
$r3alive = Get-AppPid
Log-Step "R3 after-finish PID=$r3alive (was $r3pid)"
if ($fpLine -match '"filePath":"([^"]+)"') {
    $fp = $matches[1]
    Start-Sleep -Seconds 3
    & $hdc file recv $fp (Join-Path $out 'R3-store.ndjson') 2>&1 | Out-Null
    $sz = 0
    if (Test-Path (Join-Path $out 'R3-store.ndjson')) { $sz = (Get-Item (Join-Path $out 'R3-store.ndjson')).Length }
    Log-Step "R3 ledger pulled ${sz}B"
    $afp = $fp -replace '\.ndjson$', '-admissions.ndjson'
    & $hdc file recv $afp (Join-Path $out 'R3-admissions.ndjson') 2>&1 | Out-Null
    $asz = 0
    if (Test-Path (Join-Path $out 'R3-admissions.ndjson')) { $asz = (Get-Item (Join-Path $out 'R3-admissions.ndjson')).Length }
    Log-Step "R3 admissions pulled ${asz}B"
    if ($sz -gt 0) {
        $py = 'C:\Users\admin\.workbuddy\binaries\python\versions\3.13.12\python.exe'
        & $py 'D:\hmprojects\HelloWorld\tools\p011_capture_manifest.py' (Join-Path $out 'R3-store.ndjson') (Join-Path $out 'R3-admissions.ndjson') (Join-Path $out 'fence.json') (Join-Path $out 'manifest.json') 2>&1 | Select-Object -Last 5 | ForEach-Object { Log-Step "manifest: $_" }
    }
} else { Log-Step 'R3 store path not found' }
Log-Step 'R3 done'

& $hdc shell 'power-shell -r' | Out-Null
Log-Step 'ALL DONE'
Write-Output 'DONE'
