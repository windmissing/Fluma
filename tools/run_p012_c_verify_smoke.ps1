param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [string]$Hap = 'D:\hmprojects\hw-p012-c-verify\entry\build\default\outputs\default\entry-default-s107-signed.hap',
  [string]$ExpectedHapSha256 = 'c1514721e85c5d9194f49d4ef572c7366ece185ddff9b43560f1287e1e0ea2a3',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p012-c-verify-20261022-smoke'
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

function Capture-Frame {
  param([string]$Name)
  $devicePath = '/data/local/tmp/p012-smoke.jpeg'
  $localPath = Join-Path $RunDir ($Name + '.jpeg')
  Invoke-Hdc @('shell','snapshot_display','-f',$devicePath) | Out-Null
  Invoke-Hdc @('file','recv',$devicePath,$localPath) | Out-Null
  Invoke-Hdc @('shell','rm','-f',$devicePath) | Out-Null
  Write-Output ("  captured " + $Name)
}

foreach ($mode in @('on','off')) {
  $tag = "smoke-$mode"
  $RunDir = Join-Path $OutputRoot $tag
  New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
  Write-Output "=== $tag ==="
  Invoke-Hdc @('shell','aa','force-stop',$Bundle)
  Start-Sleep -Seconds 1
  if ($mode -eq 'off') {
    Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
      '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
      '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','false',
      '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
      '--ps','p012tier','3','--ps','p012ablate','strat') | Out-Null
  } else {
    Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
      '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
      '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','false',
      '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
      '--ps','p012tier','3') | Out-Null
  }
  Start-Sleep -Seconds 25
  Capture-Frame "smoke-$mode-static-0"
  Start-Sleep -Milliseconds 500
  Capture-Frame "smoke-$mode-static-1"
}
Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Write-Output "SMOKE DONE"
exit 0
