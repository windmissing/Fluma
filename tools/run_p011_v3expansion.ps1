param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [int]$StartIndex = 1,
  [int]$Count = 6,
  [string]$Hap = 'D:\hmprojects\hw-v3pilot-c869cda\entry\build\default\outputs\default\entry-default-s106-signed.hap',
  [string]$ExpectedHapSha256 = 'e9f3f928cbe6fe0ec586a781cd71ce86e6cd57728dfa7d580142f0e316deb6a9',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-v3expansion-20261021',
  [string]$RandomizationManifest = 'D:\hmprojects\HelloWorld\calibration\randomization-manifest-obs3v3-expansion-v1.json',
  [string]$ScheduleCsv = 'D:\hmprojects\HelloWorld\calibration\schedule-obs3v3-A18.7-rate0.5.csv',
  [string]$ScheduleSha256 = '7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46',
  [string]$GitCommit = 'c869cda741f17def77da923e43fcafece05bb892',
  [string]$WorktreeRoot = 'D:\hmprojects\hw-v3pilot-c869cda',
  [double]$PeakADeg = 18.7,
  [double]$RateDegPerSec = 0.5
)

$ErrorActionPreference = 'Stop'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

# Observation-3 v3 EXPANSION batch runner (34 runs, protocol §6 + pre-expansion ruling).
#采集窗口 = schedule 理论时长 196.3s + 5s 收尾缓冲（ruling 2.1 建议 4）。

$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) { throw "HAP SHA256 mismatch: $hapSha" }
$schedSha = (Get-FileHash -LiteralPath $ScheduleCsv -Algorithm SHA256).Hash.ToLowerInvariant()
if ($schedSha -ne $ScheduleSha256) { throw "Schedule SHA256 mismatch: $schedSha" }
$randSha = (Get-FileHash -LiteralPath $RandomizationManifest -Algorithm SHA256).Hash.ToLowerInvariant()
$rand = Get-Content -LiteralPath $RandomizationManifest -Raw | ConvertFrom-Json
$srcFileSha = (Get-FileHash -LiteralPath (Join-Path $WorktreeRoot 'entry\src\main\ets\physics\WaterSandField.ets') -Algorithm SHA256).Hash.ToLowerInvariant()
$runnerSha = (Get-FileHash -LiteralPath $MyInvocation.MyCommand.Path -Algorithm SHA256).Hash.ToLowerInvariant()

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed: ' + ($Arguments -join ' ') }
  return $output
}

# Group lookup from randomization manifest (plannedRunId -> fit/holdout)
$fitList = @($rand.randomization.fit)
$holdList = @($rand.randomization.holdout)

Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Invoke-Hdc @('install','-r',$Hap) | Out-Null

$totalMs = 196300 + 5000  # schedule ideal duration + 5s tail buffer

for ($n = $StartIndex; $n -lt ($StartIndex + $Count); $n++) {
  $plannedId = 'p011-v3expansion-run-{0:d3}' -f $n
  $group = 'unassigned'
  if ($fitList -contains $plannedId) { $group = 'fit' }
  elseif ($holdList -contains $plannedId) { $group = 'holdout' }
  $RunDir = Join-Path $OutputRoot $plannedId
  if (Test-Path -LiteralPath $RunDir) { throw "Evidence already exists: $RunDir" }
  New-Item -ItemType Directory -Path $RunDir | Out-Null
  Write-Output "=== $plannedId (group=$group) start ==="

  $logPath = Join-Path $RunDir 'hilog.txt'
  Invoke-Hdc @('shell','power-shell','wakeup') | Out-Null
  Invoke-Hdc @('shell','power-shell','timeout','-o','600000') | Out-Null
  Invoke-Hdc @('shell','aa','force-stop',$Bundle)
  Invoke-Hdc @('shell','hilog','-r') | Out-Null
  Invoke-Hdc @('shell','hilog','-G','16M','-t','app') | Out-Null
  $hilog = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog') `
    -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
    -WindowStyle Hidden -PassThru
  try {
    Start-Sleep -Seconds 1
    Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
      '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
      '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
      '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
      '--ps','p011schedule',("$PeakADeg" + ':' + "$RateDegPerSec"),
      '--ps','p011runrole','calibration',
      '--ps','thresholdcalibrationid','hysteresis-calib-v1',
      '--ps','thetastartdeg','26',
      '--ps','thetastopdeg','20',
      '--ps','execsourcecommit',$GitCommit,
      '--ps','execsourcefilesha256',$srcFileSha,
      '--ps','execbuildsha256',$hapSha) | Out-Null
    Write-Output "want sent, waiting $totalMs ms..."
    Start-Sleep -Milliseconds $totalMs
  } finally {
    Invoke-Hdc @('shell','aa','force-stop',$Bundle)
    Start-Sleep -Seconds 1
    if (-not $hilog.HasExited) { Stop-Process -Id $hilog.Id -Force }
    $hilog.WaitForExit(5000) | Out-Null
    $hilogDump = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog','-x','-t','app') `
      -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
      -WindowStyle Hidden -PassThru
    if (-not $hilogDump.WaitForExit(30000)) { Stop-Process -Id $hilogDump.Id -Force }
  }

  $runId = $null
  try {
    $m = Select-String -LiteralPath $logPath -Pattern 'p011-run-\d+-\d+' | Select-Object -First 1
    if ($m) { $runId = $m.Matches[0].Value }
  } catch {}
  if ($null -eq $runId) { throw "runId not found for $plannedId" }

  $devicePath = "/data/app/el2/100/base/com.example.helloworld/haps/entry/cache/p011-v212-relax-hysteresis-$runId.ndjson"
  $ledgerPath = Join-Path $RunDir 'ledger.ndjson'
  Invoke-Hdc @('file','recv',$devicePath,$ledgerPath) | Out-Null

  # Trim a possibly-truncated tail line before the checker
  $trimmed = Join-Path $RunDir 'ledger-trimmed.ndjson'
  & python -c "import json,sys; f=open(sys.argv[1],encoding='utf-8',errors='replace'); lines=f.readlines(); good=lines;`nimport json as j`nwhile good:`n    try:`n        j.loads(good[-1]); break`n    except Exception: good=good[:-1]`nopen(sys.argv[2],'w',encoding='utf-8').writelines(good); print('records:',len(good))" $ledgerPath $trimmed 2>&1 | Select-Object -Last 1

  # Manifest for the checker (checker trap: must pass --manifest)
  $manifestPath = Join-Path $RunDir 'check-manifest.json'
  @{
    calibrationProtocol = @{
      protocolId = 'p011-observation3-v3'
      protocolRevision = 'preregistration-v1'
      scheduleSha256 = $ScheduleSha256
      configuredThresholds = @{ thetaStartDeg = 26; thetaStopDeg = 20 }
    }
    analysisRole = 'expansion'
    plannedRunId = $plannedId
    group = $group
    runId = $runId
    randomizationManifestSha256 = $randSha
    scheduleSha256 = $ScheduleSha256
    scheduleCsv = $ScheduleCsv
    runnerSha256 = $runnerSha
    hapSha256 = $hapSha
    gitCommit = $GitCommit
    peakADeg = $PeakADeg
    rateDegPerSec = $RateDegPerSec
  } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding UTF8

  Write-Output "running v3 checker..."
  Push-Location 'D:\hmprojects\HelloWorld\test'
  $v3Summary = Join-Path $RunDir 'v3-summary.json'
  & python 'p011_obs3_v3_protocol_check.py' --ledger $trimmed --manifest $manifestPath --source pilot --output $v3Summary 2>&1 | Select-Object -Last 2
  $v3Exit = $LASTEXITCODE
  Pop-Location

  $summary = Get-Content -LiteralPath $v3Summary -Raw | ConvertFrom-Json
  Write-Output ("=== $plannedId done: runId=$runId group=$group overall=" + $summary.overall + " v3Exit=$v3Exit ===")
}
exit 0
