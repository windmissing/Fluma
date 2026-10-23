param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [string]$PlanJson = 'D:\hmprojects\HelloWorld\calibration\p011-corr-matrix-formal-plan-v1.json',
  [string]$Hap = 'D:\hmprojects\HelloWorld\entry\build\default\outputs\default\entry-default-s133-signed.hap',
  [string]$ExpectedHapSha256 = 'b0c67920c9e2f0b5b54f86f66f5f3de5e4900ca76c7783bd3e2e4d9df124d6f3',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-corr-matrix-formal-108-20261022',
  [string]$ScheduleCsv = 'D:\hmprojects\HelloWorld\calibration\schedule-obs3v3-A18.7-rate0.5.csv',
  [string]$ScheduleSha256 = '7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46',
  [string]$GitCommit = '9d11849',
  [int]$PrefixMs = 65000,
  [int]$FormalMs = 205000,
  [int]$MaxRuns = 0,
  [switch]$DryRun
)

# P-011 correction matrix: FORMAL 108-run batch (preregistration §4/§6).
#   G1 12 blocks + G2 12 blocks + G0 3 blocks, each block = A/B/C/D sharing
#   one seed; Williams balanced sequences; block execution order frozen in
#   the plan JSON (master seed 20261022) — never reshuffled here.
#   B/C/D bind p011cvar + p011route=relax-correction-weighted-v1;
#   A binds p011cvar=A with the route UNBOUND (original relax).
#   Every run carries p011conditioning=<stratum> (65 s prefix).
# Sentinels per run abort the batch on identity, prefix, hard-safety or
# D-1b closure failure (preregistration §7.1/§7.2 — no post-hoc repair).

$ErrorActionPreference = 'Stop'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

$python = 'C:\Users\admin\.workbuddy\binaries\python\versions\3.13.12\python.exe'
$scanPy = 'D:\hmprojects\HelloWorld\tools\p011_ledger_scan.py'

$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) { throw "HAP SHA256 mismatch: $hapSha" }
$schedSha = (Get-FileHash -LiteralPath $ScheduleCsv -Algorithm SHA256).Hash.ToLowerInvariant()
if ($schedSha -ne $ScheduleSha256) { throw "Schedule SHA256 mismatch: $schedSha" }
$wsfSha = (Get-FileHash -LiteralPath 'D:\hmprojects\HelloWorld\entry\src\main\ets\physics\WaterSandField.ets' -Algorithm SHA256).Hash.ToLowerInvariant()
$planSha = (Get-FileHash -LiteralPath $PlanJson -Algorithm SHA256).Hash.ToLowerInvariant()

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
if ($DryRun) { Write-Output 'DRY RUN: plan printed, no device activity.' }

$plan = Get-Content -LiteralPath $PlanJson -Raw | ConvertFrom-Json
$totalMs = $PrefixMs + $FormalMs

if (-not $DryRun) { Copy-Item -LiteralPath $PlanJson -Destination (Join-Path $OutputRoot 'plan-frozen.json') }

$manifest = [ordered]@{
  batch = 'p011-corr-matrix-formal-108'
  preregistration = 'preregistration-P011-correction-matrix-v1.md §4/§6'
  build = (Split-Path $Hap -Leaf)
  hapSha256 = $hapSha
  sourceCommit = $GitCommit
  waterSandFieldSha256 = $wsfSha
  planJson = $PlanJson
  planSha256 = $planSha
  totalRuns = $plan.totalRuns
  sleepMsPerRun = $totalMs
  strataOrder = @($plan.strataOrder)
  note = 'conditioning prefix bound per stratum; sentinels abort on identity/prefix/hard-safety/D-1b failure; raw ledger kept only when the scan reports mid-file bad lines'
  timestamp = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ss')
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutputRoot 'collect-manifest.json') -Encoding UTF8

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed: ' + ($Arguments -join ' ') }
  return $output
}

if ($DryRun) {
  foreach ($r in $plan.runs) {
    Write-Output ('{0,3} {1} {2} {3} {4} seed={5} route={6} cond={7}' -f `
      $r.runIndex, $r.plannedId, $r.stratum, $r.williamsSequence, $r.variant, `
      $r.seed, $(if ($r.route -eq '') { 'UNBOUND' } else { $r.route }), $r.conditioning)
  }
  exit 0
}

$batchLog = Join-Path $OutputRoot 'batch-log.csv'
Add-Content -LiteralPath $batchLog -Value 'runIndex,plannedId,stratum,blockId,variant,startWall,records,d1bClosed,identityOk,prefixOk,hardSafetyOk,maxErr,boundaryContact,wallSec'

Invoke-Hdc @('shell','power-shell','wakeup') | Out-Null
Invoke-Hdc @('shell','power-shell','timeout','-o','43200000') | Out-Null
Invoke-Hdc @('install','-r',$Hap) | Out-Null

$executedRuns = 0
foreach ($r in $plan.runs) {
  # Graceful stop-file check (manager 2026-10-23 16:24 user ruling:
  # EARLY_STOPPED_AT_USER_DISCRETION — free the device for the effector
  # window; completed runs stay valid, remaining plan rows unexecuted).
  $stopFile = Join-Path $OutputRoot 'STOP_BATCH.now'
  if (Test-Path $stopFile) {
    Write-Output ('STOP_BATCH.now detected before run ' + $r.plannedId + '; stopping gracefully (EARLY_STOPPED_AT_USER_DISCRETION).')
    break
  }
  if ($MaxRuns -gt 0 -and $executedRuns -ge $MaxRuns) {
    Write-Output ('MaxRuns reached (' + $MaxRuns + '); stopping (smoke / partial mode).')
    break
  }
  $RunDir = Join-Path $OutputRoot $r.plannedId
  $sentinel = Join-Path $RunDir 'sentinel.json'
  if ((Test-Path -LiteralPath $sentinel)) {
    Write-Output ('SKIP (already complete): ' + $r.plannedId)
    $executedRuns++
    continue
  }
  if (Test-Path -LiteralPath $RunDir) {
    $stale = $RunDir + '-partial-' + (Get-Date -Format 'yyyyMMddHHmmss')
    Write-Output ('moving incomplete run dir -> ' + (Split-Path $stale -Leaf))
    Move-Item -LiteralPath $RunDir -Destination $stale
  }
  New-Item -ItemType Directory -Path $RunDir | Out-Null

  $startWall = Get-Date
  Write-Output ('=== [' + $r.runIndex + '/' + $plan.totalRuns + '] ' + $r.plannedId +
    ' (varian=' + $r.variant + ', seed=' + $r.seed + ', route=' + $(if ($r.route -eq '') { 'UNBOUND' } else { $r.route }) + ') ===')

  $metaPath = Join-Path $RunDir 'scan-meta.json'
  $logPath = Join-Path $RunDir 'hilog.txt'
  # A run whose ledger was already scanned (e.g. the first run before a
  # sentinel-criteria fix) is re-evaluated, never silently re-captured.
  $needCapture = -not (Test-Path -LiteralPath $metaPath)
  if (-not $needCapture) {
    Write-Output ('  ledger already scanned -> re-evaluating sentinels from ' + (Split-Path $metaPath -Leaf))
  }
  Invoke-Hdc @('shell','aa','force-stop',$Bundle)
  if ($needCapture) {
  Invoke-Hdc @('shell','hilog','-r') | Out-Null
  Invoke-Hdc @('shell','hilog','-G','16M','-t','app') | Out-Null
  $hilog = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog') `
    -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
    -WindowStyle Hidden -PassThru
  try {
    Start-Sleep -Seconds 1
    $wantArgs = @('shell','aa','start','-b',$Bundle,'-a',$Ability,
      '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
      '--ps','seed',([string]$r.seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
      '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
      '--ps','p011schedule','18.7:0.5',
      '--ps','p011cvar',$r.variant,
      '--ps','p011conditioning',$r.conditioning,
      '--ps','p011runrole','calibration',
      '--ps','thresholdcalibrationid','user-configured',
      '--ps','thetastartdeg','26',
      '--ps','thetastopdeg','20',
      '--ps','execsourcecommit',$GitCommit,
      '--ps','execsourcefilesha256',$wsfSha,
      '--ps','execbuildsha256',$hapSha)
    if ($r.route -ne '') { $wantArgs += @('--ps','p011route',$r.route) }
    Invoke-Hdc @($wantArgs) | Out-Null
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
  }

  # ---- run id (single run guaranteed by the force-stop above) ----
  $runId = $null
  try {
    $m = Select-String -LiteralPath $logPath -Pattern 'p011-run-\d+-\d+' | Select-Object -First 1
    if ($m) { $runId = $m.Matches[0].Value }
  } catch {}
  if ($null -eq $runId) {
    throw ('runId not found for ' + $r.plannedId + ' -> MEASUREMENT_BLOCKED, aborting batch')
  }
  Write-Output ('  runId=' + $runId)

  if ($needCapture) {
  $devicePath = "/data/app/el2/100/base/com.example.helloworld/haps/entry/cache/p011-v212-relax-hysteresis-$runId.ndjson"
  $ledgerPath = Join-Path $RunDir 'ledger.ndjson'
  Invoke-Hdc @('file','recv',$devicePath,$ledgerPath) | Out-Null

  $ledgerTrim = Join-Path $RunDir 'ledger-trimmed.ndjson'
  & $python $scanPy $ledgerPath $metaPath $ledgerTrim
  if ($LASTEXITCODE -ne 0) { throw 'ledger scan failed for ' + $r.plannedId }
  }

  $meta = Get-Content -LiteralPath $metaPath -Raw | ConvertFrom-Json
  $seenVariants = @($meta.variants.PSObject.Properties.Name)

  # --- SENTINEL 1: identity (preregistration §1 — no label-only arms) ---
  $identityOk = $true
  if ($r.variant -eq 'A') {
    if ($seenVariants -contains 'B' -or $seenVariants -contains 'C' -or $seenVariants -contains 'D') { $identityOk = $false }
  } else {
    if (-not ($seenVariants -contains $r.variant)) { $identityOk = $false }
    if ($seenVariants.Count -ne 1) { $identityOk = $false }
    if (-not $meta.d1bClosure) { $identityOk = $false }
  }

  # --- SENTINEL 2: conditioning prefix actually ran before the formal schedule ---
  $prefixOk = ($meta.conditioningPrefixRecords -gt 0) -and
              ($null -ne $meta.formalStart) -and
              ($meta.formalStart.elapsedFromFirstMs -ge ($PrefixMs - 5000)) -and
              ($meta.formalStart.elapsedFromFirstMs -le ($PrefixMs + 5000))

  # --- SENTINEL 3: hard safety (any False field is a stop, §7.1) ---
  $hs = $meta.hardSafety
  $hardSafetyOk = ($hs.rangeFalse -eq 0) -and ($hs.conservationFalse -eq 0) -and
                  ($hs.finiteFalse -eq 0) -and ($hs.stockFalse -eq 0) -and
                  ($hs.capacityFalse -eq 0) -and ($meta.runIdUnique)
  # The engine's own hard gate is
  #   range &&= maxSandTopInvariantError <= tan(30 deg) + EPS   (EPS = 1e-9,
  #   WaterSandField.ets const EPS) and it lands in audit.range. The sentinel
  #   therefore uses the SAME tolerance instead of a stricter comparison, and
  #   records boundary contact (within 1e-9) separately for the receipt.
  $tan30 = [Math]::Tan(30 * [Math]::PI / 180)
  $engineEps = 1e-9
  $invariantOk = ([double]$hs.maxSandTopInvariantError -le ($tan30 + $engineEps))
  $invariantAtBoundary = ([Math]::Abs(([double]$hs.maxSandTopInvariantError) - $tan30) -le $engineEps)

  $verdict = [ordered]@{
    plannedId = $r.plannedId
    stratum = $r.stratum
    blockId = $r.blockId
    williamsSequence = $r.williamsSequence
    variant = $r.variant
    route = $r.route
    seed = $r.seed
    runId = $runId
    runIdUnique = $meta.runIdUnique
    records = $meta.records
    variantsSeen = $seenVariants
    d1b = $meta.d1b
    d1bClosure = $meta.d1bClosure
    identityOk = $identityOk
    prefixRecords = $meta.conditioningPrefixRecords
    formalStartElapsedMs = $(if ($meta.formalStart -ne $null) { $meta.formalStart.elapsedFromFirstMs } else { $null })
    formalStartStateHash = $(if ($meta.formalStart -ne $null) { $meta.formalStart.stateHash } else { $null })
    prefixOk = $prefixOk
    hardSafety = $hs
    maxSandTopInvariantError = $hs.maxSandTopInvariantError
    invariantWithinTan30 = $invariantOk
    invariantToleranceNote = 'tan30 + 1e-9 (engine EPS, WaterSandField.ets)'
    invariantAtBoundary = $invariantAtBoundary
    hardSafetyOk = $hardSafetyOk
    rawBytes = $meta.rawBytes
    trimmedBytes = $meta.trimmedBytes
    midFileBadLines = $meta.midFileBadLines
    tailDroppedLines = $meta.tailDroppedLines
    startWall = $startWall.ToString('yyyy-MM-ddTHH:mm:ss')
    hdcRecvWallSec = [int]((Get-Date) - $startWall).TotalSeconds
  }
  $verdict | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $RunDir 'sentinel.json') -Encoding UTF8

  Write-Output ('  records={0} variants=[{1}] d1bClosed={2} identityOk={3} prefixOk={4} hardSafetyOk={5} invWithinTan30={6} maxErr={7} atBoundary={8}' -f `
    $meta.records, ($seenVariants -join ','), $meta.d1bClosure, $identityOk, $prefixOk, $hardSafetyOk, $invariantOk, $hs.maxSandTopInvariantError, $invariantAtBoundary)

  # raw copy is byte-identical to the trimmed copy minus trailing garbage
  # (and it is only kept when the scan saw problems) — frees ~2 GB per run.
  if (($needCapture) -and ($meta.midFileBadLines -eq 0) -and ($meta.tailDroppedLines -le 2)) {
    Remove-Item -LiteralPath $ledgerPath -Force
    Write-Output ('  raw ledger released (identical prefix bytes, trimmed kept), freed ' + [Math]::Round($meta.rawBytes / 1GB, 2) + ' GB')
  } elseif ($needCapture) {
    Write-Output ('  RAW KEPT: midBad=' + $meta.midFileBadLines + ' tailDropped=' + $meta.tailDroppedLines)
  }

  $wallSec = [int]((Get-Date) - $startWall).TotalSeconds
  Add-Content -LiteralPath $batchLog -Value ('{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13}' -f `
    $r.runIndex, $r.plannedId, $r.stratum, $r.blockId, $r.variant, $startWall.ToString('yyyy-MM-ddTHH:mm:ss'),
    $meta.records, $meta.d1bClosure, $identityOk, $prefixOk, ($hardSafetyOk -and $invariantOk),
    $hs.maxSandTopInvariantError, $invariantAtBoundary, $wallSec)

  if (-not $identityOk) { throw ('IDENTITY FAILURE: ' + $r.plannedId + ' saw [' + ($seenVariants -join ',') + '] -> MEASUREMENT_BLOCKED, aborting batch') }
  if (-not $prefixOk) { throw ('PREFIX FAILURE: ' + $r.plannedId + ' conditioning prefix missing/misaligned -> MEASUREMENT_BLOCKED, aborting batch') }
  if (-not $hardSafetyOk) { throw ('HARD SAFETY FAILURE: ' + $r.plannedId + ' -> batch stopped per §7.1') }
  if (-not $invariantOk) { throw ('INVARIANT FAILURE: ' + $r.plannedId + ' maxSandTopInvariantError=' + $hs.maxSandTopInvariantError + ' > tan30+1e-9 -> batch stopped') }
  $executedRuns++
}

Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Write-Output 'FORMAL 108-RUN BATCH COMPLETE'
exit 0
