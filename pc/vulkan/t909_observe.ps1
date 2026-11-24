param(
    [Parameter(Mandatory=$true)][string]$Output,
    [Parameter(Mandatory=$true)][int]$X,
    [Parameter(Mandatory=$true)][int]$Y,
    [ValidateRange(1,1000000)][int]$FirstFrame = 1,
    [ValidateRange(1,120)][int]$Frames = 60,
    [switch]$KappaZero,
    [switch]$StageTiming,
    [switch]$InventoryV1
)
$ErrorActionPreference = 'Stop'
if ($X -lt 0 -or $Y -lt 0) { throw 'ROI origin must be nonnegative' }
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$artifacts = Join-Path $workspace '.codex/local-test'
$modules = Join-Path $artifacts 't830-m1-bounded'
$program = Join-Path $artifacts 't909-pc-ninja/hello_world_pc.exe'
$candidate = Join-Path $modules 'quantum_candidate.spv'
if ($InventoryV1) { $candidate = Join-Path $artifacts 't909-inventory/quantum_candidate_inventory_v1.spv' }
$source = Join-Path $modules 'full_source.spv'
$commit = Join-Path $modules 'quantum_commit.spv'
foreach ($path in $program,$candidate,$source,$commit) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing build artifact $path" }
}
$run = New-Item -ItemType Directory -Path $Output
$request = Join-Path $run.FullName 'roi.txt'
@("$FirstFrame $Frames", "$X $Y 16 16", "$($X+16) $Y 16 16", "$($X+32) $Y 16 16") |
    Set-Content -LiteralPath $request -Encoding ascii
$parameters = @('--m1-window', $source, $candidate, $commit, '--m1-empty-preview', '--m1-manual-gravity',
                '--m1-one-quantum-grant', '--m1-dual-source', '--m1-observe-series', $request,
                (Join-Path $run.FullName 'root'))
if ($KappaZero) { $parameters += '--m1-observe-kappa-zero' }
if ($StageTiming) { $parameters += '--m1-stage-timing' }
if ($InventoryV1) { $parameters += '--m1-inventory-v1' }
$builds = foreach ($path in $program,$candidate,$source,$commit) {
    Get-FileHash -LiteralPath $path -Algorithm SHA256 | Select-Object Path,Hash
}
@{ executable = $program; arguments = $parameters; artifacts = @($builds) } |
    ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $run.FullName 'argv.json') -Encoding utf8
$log = Join-Path $run.FullName 'demo.jsonl'
& $program @parameters | Set-Content -LiteralPath $log -Encoding utf8
exit $LASTEXITCODE
