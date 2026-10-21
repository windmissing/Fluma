param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [string]$Hap = 'D:\hmprojects\hw-v3pilot-d4aa997\entry\build\default\outputs\default\entry-default-s104-signed.hap',
  [string]$ExpectedHapSha256 = '79cbf52602b3f58563da3877e3f2b66209ed3d74f9b6b2555bb199c323b57c51',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-v3pilot2-20261021',
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
if ($hapSha -ne $ExpectedHapSha256) {
  throw "HAP SHA256 mismatch: $hapSha != $ExpectedHapSha256"
}

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed: ' + ($Arguments -join ' ') }
  return $output
}

$RunDir = Join-Path $OutputRoot ('smoke-' + (Get-Date -Format 'HHmmss'))
New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
$logPath = Join-Path $RunDir 'hilog.txt'

Write-Output "=== smoke run start ($RunDir) ==="
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
  # Pure schedule want (no wstilt): player starts; metadata arming is the
  # question under test. This is a DIAGNOSTIC run, not the formal pilot.
  Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
    '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
    '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
    '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
    '--ps','p011schedule',("$PeakADeg" + ':' + "$RateDegPerSec")) | Out-Null
  Write-Output "schedule want sent (peak=$PeakADeg rate=$RateDegPerSec), waiting $DurationSec s..."
  Start-Sleep -Seconds $DurationSec
} finally {
  Invoke-Hdc @('shell','aa','force-stop',$Bundle)
  Start-Sleep -Seconds 1
  if (-not $hilog.HasExited) { Stop-Process -Id $hilog.Id -Force }
  $hilog.WaitForExit(5000) | Out-Null
  $hilogDump = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog','-x','-t','app') `
    -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
    -WindowStyle Hidden -PassThru
  if (-not $hilogDump.WaitForExit(30000)) {
    Stop-Process -Id $hilogDump.Id -Force
  }
}

# Pull the ledger
$storeLine = (Select-String -LiteralPath $logPath -Pattern 'P011_OBS_STORE' | Select-Object -First 1).Line
if ($null -eq $storeLine) { throw 'observation store identity missing' }
$store = $storeLine.Substring($storeLine.IndexOf('{')) | ConvertFrom-Json
$ledgerPath = Join-Path $RunDir 'ledger.ndjson'
$ledgerName = [System.IO.Path]::GetFileName($store.filePath)
$ledgerDevicePaths = @((Invoke-Hdc @('shell','find','/data/app/el2','-name',$ledgerName,'-type','f')) |
  ForEach-Object { $_.Trim() } | Where-Object { $_.StartsWith('/data/app/el2/') })
if ($ledgerDevicePaths.Count -ne 1) { throw 'ledger path resolution failed' }
Invoke-Hdc @('file','recv',$ledgerDevicePaths[0],$ledgerPath) | Out-Null

# Quick verdict from hilog
$dispatchCount = (Select-String -LiteralPath $logPath -Pattern 'SCHEDULE_DISPATCH' | Measure-Object).Count
$driftExceeded = (Select-String -LiteralPath $logPath -Pattern 'SCHEDULE_DRIFT_EXCEEDED' | Measure-Object).Count
$playerStart = (Select-String -LiteralPath $logPath -Pattern 'player START' | Measure-Object).Count
$rejected = (Select-String -LiteralPath $logPath -Pattern 'SCHEDULE_ACTIVE_COMMAND_REJECTED' | Measure-Object).Count
Write-Output "playerStart=$playerStart dispatchCount=$dispatchCount driftExceeded=$driftExceeded rejected=$rejected"
Write-Output "ledger: $ledgerPath"
# Check first ledger record's context identity fields
$first = Get-Content -LiteralPath $ledgerPath -TotalCount 1
if ($first -match '"executedThresholds":null') {
  Write-Output 'IDENTITY: executedThresholds=null (state machine NOT armed - MISMATCH expected)'
} elseif ($first -match '"executedThresholds"') {
  Write-Output 'IDENTITY: executedThresholds present (state machine armed)'
} else {
  Write-Output 'IDENTITY: executedThresholds field not found in first record'
}
if ($first -match '"runRole":"calibration"') { Write-Output 'runRole=calibration OK' }
elseif ($first -match '"runRole":null') { Write-Output 'runRole=null (metadata NOT armed)' }
exit 0
