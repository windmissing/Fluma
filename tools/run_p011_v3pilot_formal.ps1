param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [string]$Hap = 'D:\hmprojects\hw-v3pilot-c869cda\entry\build\default\outputs\default\entry-default-s106-signed.hap',
  [string]$ExpectedHapSha256 = 'e9f3f928cbe6fe0ec586a781cd71ce86e6cd57728dfa7d580142f0e316deb6a9',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-v3pilot4-20261021',
  [double]$PeakADeg = 18.7,
  [double]$RateDegPerSec = 0.5,
  [int]$DurationSec = 240,
  # §26.6 identity triple (XOR-bound with metadata group; all-or-none)
  [string]$ExecSourceCommit = 'c869cda741f17def77da923e43fcafece05bb892',
  [string]$ExecSourceFileSha256 = '',  # filled from WaterSandField.ets at runtime if empty
  [string]$ExecBuildSha256 = ''        # filled from HAP digest if empty
)

$ErrorActionPreference = 'Stop'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) { throw "HAP SHA256 mismatch: $hapSha" }
if (-not $ExecSourceFileSha256) {
  $ExecSourceFileSha256 = (Get-FileHash -LiteralPath 'D:\hmprojects\hw-v3pilot-c869cda\entry\src\main\ets\physics\WaterSandField.ets' -Algorithm SHA256).Hash.ToLowerInvariant()
}
if (-not $ExecBuildSha256) { $ExecBuildSha256 = $hapSha }

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed: ' + ($Arguments -join ' ') }
  return $output
}

$RunDir = Join-Path $OutputRoot ('run-' + (Get-Date -Format 'HHmmss'))
New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
$logPath = Join-Path $RunDir 'hilog.txt'

Write-Output "=== run start ($RunDir) ==="
$srcPrefix = $ExecSourceFileSha256.Substring(0, 12)
$bldPrefix = $ExecBuildSha256.Substring(0, 12)
Write-Output "identity: commit=$ExecSourceCommit srcFileSha=$srcPrefix... buildSha=$bldPrefix..."
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
  # Full formal want: schedule + metadata group + identity triple (§26.5/§26.6)
  Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
    '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
    '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
    '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
    '--ps','p011schedule',("$PeakADeg" + ':' + "$RateDegPerSec"),
    '--ps','p011runrole','calibration',
    '--ps','thresholdcalibrationid','hysteresis-calib-v1',
    '--ps','thetastartdeg','26',
    '--ps','thetastopdeg','20',
    '--ps','execsourcecommit',$ExecSourceCommit,
    '--ps','execsourcefilesha256',$ExecSourceFileSha256,
    '--ps','execbuildsha256',$ExecBuildSha256) | Out-Null
  Write-Output "formal want sent (schedule=${PeakADeg}:${RateDegPerSec} + metadata + identity), waiting $DurationSec s..."
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

$runId = $null
try {
  $m = Select-String -LiteralPath $logPath -Pattern 'p011-run-\d+-\d+' | Select-Object -First 1
  if ($m) { $runId = $m.Matches[0].Value }
} catch {}
if ($null -eq $runId) { throw 'runId not found in hilog' }
Write-Output "runId: ${runId}"
$devicePath = "/data/app/el2/100/base/com.example.helloworld/haps/entry/cache/p011-v212-relax-hysteresis-${runId}.ndjson"
$ledgerPath = Join-Path $RunDir 'ledger.ndjson'
Invoke-Hdc @('file','recv',$devicePath,$ledgerPath) | Out-Null
$lenBytes = (Get-Item $ledgerPath).Length
Write-Output ("ledger: " + $ledgerPath + " (" + $lenBytes + " bytes)")
Write-Output "runId=$runId" | Set-Content -LiteralPath (Join-Path $RunDir 'runid.txt')
@{
  runId = $runId
  hapSha256 = $hapSha
  execSourceCommit = $ExecSourceCommit
  execSourceFileSha256 = $ExecSourceFileSha256
  execBuildSha256 = $ExecBuildSha256
  peakADeg = $PeakADeg
  rateDegPerSec = $RateDegPerSec
  scheduleSha256 = '7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46'
  protocolId = 'p011-observation3-v3'
  protocolRevision = 'preregistration-v1'
  analysisRole = 'pilot'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $RunDir 'run-identity.json') -Encoding UTF8
exit 0
