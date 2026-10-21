param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [string]$Hap = 'D:\hmprojects\hw-v3pilot-69345cc\entry\build\default\outputs\default\entry-default-s105-signed.hap',
  [string]$ExpectedHapSha256 = '393f996e692063511fa778e57b659e92c332ab544861cddf415a090d9424de30',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-v3pilot3-20261021',
  [double]$PeakADeg = 18.7,
  [double]$RateDegPerSec = 0.5,
  [int]$DurationSec = 240
)

$ErrorActionPreference = 'Stop'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) { throw "HAP SHA256 mismatch: $hapSha" }

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed: ' + ($Arguments -join ' ') }
  return $output
}

$RunDir = Join-Path $OutputRoot ('smoke2-' + (Get-Date -Format 'HHmmss'))
New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
$logPath = Join-Path $RunDir 'hilog.txt'

Write-Output "=== smoke2 run start ($RunDir) ==="
Invoke-Hdc @('shell','power-shell','wakeup') | Out-Null
Invoke-Hdc @('shell','power-shell','timeout','-o','600000') | Out-Null
Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Invoke-Hdc @('install','-r',$Hap) | Out-Null
Invoke-Hdc @('shell','hilog','-r') | Out-Null
Invoke-Hdc @('shell','hilog','-G','16M','-t','app') | Out-Null
$hilog = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog') `
  -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
  -WindowStyle Hidden -PassThru

try {
  Start-Sleep -Seconds 1
  # Schedule want WITH the metadata arming group (§26.5): p011runrole +
  # thresholdcalibrationid + thetastartdeg + thetastopdeg travel in the SAME
  # want; validated as a group, armed BEFORE player.start().
  Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
    '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
    '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
    '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
    '--ps','p011schedule',("$PeakADeg" + ':' + "$RateDegPerSec"),
    '--ps','p011runrole','calibration',
    '--ps','thresholdcalibrationid','hysteresis-calib-v1',
    '--ps','thetastartdeg','26',
    '--ps','thetastopdeg','20') | Out-Null
  Write-Output "schedule want + metadata sent, waiting $DurationSec s..."
  Start-Sleep -Seconds $DurationSec
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

# Locate the ledger via the newest cache file (hilog OBS_STORE line may be
# flushed by the 16MB buffer; cache file name carries the runId)
$runId = $null
try {
  $m = Select-String -LiteralPath $logPath -Pattern 'p011-run-\d+-\d+' | Select-Object -First 1
  if ($m) { $runId = $m.Matches[0].Value }
} catch {}
if ($null -eq $runId) { throw 'runId not found in hilog' }
Write-Output "runId: $runId"
$devicePath = "/data/app/el2/100/base/com.example.helloworld/haps/entry/cache/p011-v212-relax-hysteresis-$runId.ndjson"
$ledgerPath = Join-Path $RunDir 'ledger.ndjson'
Invoke-Hdc @('file','recv',$devicePath,$ledgerPath) | Out-Null
Write-Output "ledger: $ledgerPath ($((Get-Item $ledgerPath).Length) bytes)"

$dispatchCount = (Select-String -LiteralPath $logPath -Pattern 'SCHEDULE_DISPATCH' | Measure-Object).Count
Write-Output "hilog dispatch lines: $dispatchCount"
Write-Output "runId=$runId" | Set-Content -LiteralPath (Join-Path $RunDir 'runid.txt')
exit 0
