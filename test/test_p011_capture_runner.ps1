# P-011 v2.8 host regression: residual cleanup is scoped to target hilog only.
$ErrorActionPreference = 'Stop'
$testOutput = Join-Path $env:TEMP ('p011-runner-test-' + [guid]::NewGuid().ToString('N'))
try {
  . (Join-Path $PSScriptRoot 'p011_tc02_formal_suite.ps1') -Mode Schedule -OutputRoot $testOutput
  $script:mockProcesses = @(
    [pscustomobject]@{ ProcessId = 11; CommandLine = 'hdc.exe -t 62T0225B10010020 shell hilog' },
    [pscustomobject]@{ ProcessId = 12; CommandLine = 'hdc.exe -t other-device shell hilog' },
    [pscustomobject]@{ ProcessId = 13; CommandLine = 'hdc.exe -s server' },
    [pscustomobject]@{ ProcessId = 14; CommandLine = 'hdc.exe -t 62T0225B10010020 shell sh other-script.sh' }
  )
  $script:stopped = @()
  function Get-CimInstance { param($Filter, $ClassName) $script:mockProcesses }
  function Stop-Process {
    param($Id, [switch]$Force, $ErrorAction)
    $script:stopped += $Id
    $script:mockProcesses = @($script:mockProcesses | Where-Object ProcessId -ne $Id)
  }
  function Wait-Process { param($Id, $Timeout, $ErrorAction) }
  Clear-ResidualHilogCollectors
  if ($script:stopped.Count -ne 1 -or $script:stopped[0] -ne 11) {
    throw 'Residual cleanup affected unrelated tasks or failed to clear the target collector'
  }
  if ($script:mockProcesses.Count -ne 3) { throw 'Unrelated processes were removed' }
  Write-Host 'PASS: only target hilog collector stopped; daemon and unrelated tasks retained'
} finally {
  $resolvedTestOutput = [System.IO.Path]::GetFullPath($testOutput)
  $resolvedTemp = [System.IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
  if ($resolvedTestOutput.StartsWith($resolvedTemp, [System.StringComparison]::OrdinalIgnoreCase)) {
    Remove-Item -LiteralPath $resolvedTestOutput -Recurse -Force -ErrorAction SilentlyContinue
  }
}
