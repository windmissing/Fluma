# T177 采集器 v2（T181 §4.1 合规）：分 PID/run 归属 + 显式状态
#   修订要点：
#   1. 冷启前先冻结旧 PID/run 并归档旧尾（hilog 全量 dump），再 force-stop；记录命令 stdout/stderr/退出码
#   2. 启动新包后立即读新 PID、model/run 身份；建立新 PID+run 过滤窗
#   3. 只把归属已证明的新进程事件计入冷启失败率；hilog reset 不是归属证据
#   4. 普通 aa start 未产生新 run 报 REUSED_RUN；无 START 不假定长按有效
#   5. 旧 failed 状态直接报 PREEXISTING_FAILED，不反复运行无效触摸宽矩阵
#   6. 同进程 newrun 按 run 分界（不只靠 PID）；保留两段物理 commit 与 diagnostic sequence
param(
    [string]$Postures = 'LUR'   # L=左倾 U=回正 R=右倾，用户按此顺序手持
)
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t177v2-capture-20261027'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
$script:state = [ordered]@{}
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss.fff'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console-v2.log') -Value "[$t] $msg" }
function Run-Cmd($desc, $cmdline) {
    $o = & $cmdline 2>&1
    $code = $LASTEXITCODE
    Add-Content -Path (Join-Path $out 'cmd-audit.log') -Value "[$(Get-Date -Format 'HH:mm:ss.fff')] DESC=$desc EXIT=$code CMD=$cmdline OUT=$($o -join ' | ')"
    return $o
}
function Get-AppPid {
    $o = Run-Cmd 'pidof' { & $hdc shell "pidof $bundle" }
    return (($o -join ' ').Trim() -split '\s+')[0]
}
function Dump-Full($file) {
    $o = Run-Cmd "hilog dump -> $file" { & $hdc shell 'hilog -x' }
    $o | Out-File (Join-Path $out $file) -Encoding utf8
}
function Shot($n) {
    Run-Cmd "snapshot $n" { & $hdc shell "snapshot_display -f /data/local/tmp/t177v2-$n.jpeg" }
    Run-Cmd "recv $n" { & $hdc file recv /data/local/tmp/t177v2-$n.jpeg (Join-Path $out "$n.jpeg") }
}

Log-Step 'T177v2 start (T181 §4.1 compliant)'
& $hdc shell 'power-shell timeout -o 600000' | Out-Null

# ---- 步骤1：冻结旧进程身份与旧尾 ----
$oldPid = Get-AppPid
$script:state['oldPid'] = $oldPid
Log-Step "step1 freeze old identity: oldPid=$oldPid"
if ($oldPid) {
    Dump-Full 'pre-freeze-old-tail-full.txt'
    # 从旧尾提取旧 run 身份
    $runLine = Select-String -Path (Join-Path $out 'pre-freeze-old-tail-full.txt') -Pattern 'p011-run-\d+-\d+' | Select-Object -Last 1
    if ($runLine -match '(p011-run-\d+-\d+)') { $script:state['oldRun'] = $matches[1] }
    Log-Step "step1 old run=$($script:state['oldRun']) archived (pre-freeze-old-tail-full.txt)"
} else {
    $script:state['oldRun'] = 'NONE'
    Log-Step 'step1 no old process (fresh device state)'
}

# ---- 步骤2：force-stop 并立即确认死亡 ----
Log-Step 'step2 force-stop'
Run-Cmd 'force-stop' { & $hdc shell "aa force-stop $bundle" }
Start-Sleep -Seconds 2
$dead = Get-AppPid
if ($dead) { Log-Step "step2 WARN: process still alive after force-stop pid=$dead" } else { Log-Step 'step2 old process confirmed dead' }

# ---- 步骤3：冷启 + 立即读新身份 ----
Log-Step 'step3 cold start'
Run-Cmd 'cold-start' { & $hdc shell "aa start -a EntryAbility -b $bundle --ps emptysanddemo 1" }
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 2
$newPid = Get-AppPid
$script:state['newPid'] = $newPid
Log-Step "step3 newPid=$newPid"
# 启动即全量 dump（含 model/run/config READY 行）
Dump-Full 'post-start-identity-full.txt'
$runLine2 = Select-String -Path (Join-Path $out 'post-start-identity-full.txt') -Pattern "p011-run-\d+-\d+" | Select-Object -Last 1
if ($runLine2 -match '(p011-run-\d+-\d+)') { $script:state['newRun'] = $matches[1] }
Log-Step "step3 newRun=$($script:state['newRun'])"
# 归属校验：newRun 不得等于 oldRun
if ($script:state['newRun'] -eq $script:state['oldRun'] -and $script:state['oldRun'] -ne 'NONE') {
    Log-Step 'step3 REUSED_RUN: newRun==oldRun — aborting posture windows (T181 §4.1)'
    $script:state['runStatus'] = 'REUSED_RUN'
    $script:state | ConvertTo-Json | Out-File (Join-Path $out 'capture-state.json') -Encoding utf8
    Write-Output 'DONE (REUSED_RUN)'
    exit 0
}
$script:state['runStatus'] = 'NEW_RUN'

# ---- 步骤4：PREEXISTING_FAILED 检查（冷启静置 8s，看 step 是否健康）----
Log-Step 'step4 preexisting-failed check (8s quiet)'
Start-Sleep -Seconds 8
Dump-Full 'precheck-full.txt'
$pf = Select-String -Path (Join-Path $out 'precheck-full.txt') -Pattern "page-physics-failed" | Where-Object { $_.Line -match $newPid }
if ($pf) {
    Log-Step "step4 PREEXISTING_FAILED: $($pf.Count) page-physics-failed for new PID — skipping touch windows"
    $script:state['preexisting'] = 'FAILED'
    $script:state | ConvertTo-Json | Out-File (Join-Path $out 'capture-state.json') -Encoding utf8
    Write-Output 'DONE (PREEXISTING_FAILED)'
    exit 0
}
$script:state['preexisting'] = 'HEALTHY'
Log-Step 'step4 healthy: no physics failure for new PID'

# ---- 步骤5：三姿态窗口（用户手持；微位移触摸）----
$seq = 0
foreach ($p in $Postures.ToCharArray()) {
    $seq++
    $pname = @{ 'L' = 'tilt-LEFT'; 'U' = 'upright'; 'R' = 'tilt-RIGHT' }[[string]$p]
    Log-Step "posture $seq/$($Postures.Length): $pname — user holds now"
    # 微位移触摸发沙（2.5s）
    Run-Cmd "touch $seq" { & $hdc shell 'uitest uiInput swipe 700 1200 715 1205 2500' }
    Start-Sleep -Seconds 1
    Shot "p$seq-mid"
    Start-Sleep -Seconds 6
    Shot "p$seq-end"
}
# 静置沉积
Log-Step 'settle 12s'
Start-Sleep -Seconds 12
Shot 'settled'

# ---- 步骤6：终态全量 dump（新 PID+run 过滤窗分析用）----
Dump-Full 'final-full.txt'
# 校验进程仍活 + run 未变
$endPid = Get-AppPid
$script:state['endPid'] = $endPid
$runLine3 = Select-String -Path (Join-Path $out 'final-full.txt') -Pattern 'p011-run-\d+-\d+' | Select-Object -Last 1
if ($runLine3 -match '(p011-run-\d+-\d+)') { $script:state['endRun'] = $matches[1] }
if ($endPid -eq $newPid -and $script:state['endRun'] -eq $script:state['newRun']) {
    Log-Step 'step6 attribution OK: endPid==newPid && endRun==newRun'
} else {
    Log-Step "step6 WARN: attribution drift endPid=$endPid endRun=$($script:state['endRun'])"
}
$script:state | ConvertTo-Json | Out-File (Join-Path $out 'capture-state.json') -Encoding utf8
& $hdc shell 'power-shell -r' | Out-Null
Log-Step "DONE state=$($script:state | ConvertTo-Json -Compress)"
Write-Output 'DONE'
