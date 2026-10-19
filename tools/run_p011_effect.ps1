param(
  [Parameter(Mandatory = $true)][ValidateSet("EC-TT-01", "EC-TT-02", "EC-TT-03", "EC-TT-04", "EC-TT-05")][string]$Scenario,
  [Parameter(Mandatory = $true)][string]$Round,
  [string]$Hdc = "C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe",
  [string]$Target = "62T0225B10010020",
  [string]$Bundle = "com.example.helloworld",
  [string]$Ability = "EntryAbility",
  [string]$OutRoot = "demo\effect\P011"
)

$ErrorActionPreference = "Stop"

function Invoke-DeviceShell {
  param([string[]]$Arguments)
  & $Hdc -t $Target shell @Arguments
}

function Capture-Frame {
  param([string]$Label)
  $devicePath = "/data/local/tmp/p011_$Scenario`_$Round`_$Label.jpeg"
  Invoke-DeviceShell @("snapshot_display", "-f", $devicePath) | Out-Null
   $localPath = Join-Path $roundDir "${Label}.jpeg"
  & $Hdc -t $Target file recv $devicePath $localPath | Out-Null
  Invoke-DeviceShell @("rm", $devicePath) | Out-Null
}

function Invoke-Drag {
  param([double]$FromX, [double]$FromY, [double]$ToX, [double]$ToY, [int]$PressMs = 500, [int]$TotalMs = 1000)
  Invoke-DeviceShell @("uinput", "-T", "-g", "$FromX", "$FromY", "$ToX", "$ToY", "$PressMs", "$TotalMs") | Out-Null
}

function Wait-Seconds {
  param([double]$Seconds)
  Start-Sleep -Seconds $Seconds
}

$scenarioDir = Join-Path $OutRoot $Scenario
$roundDir = Join-Path $scenarioDir $Round
$logDir = Join-Path $roundDir "logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

Invoke-DeviceShell @("aa", "force-stop", $Bundle) | Out-Null
Invoke-DeviceShell @("hilog", "-r") | Out-Null
Invoke-DeviceShell @("aa", "start", "-b", $Bundle, "-a", $Ability, "--ps", "rendermode", "4", "--ps", "testmode", "true", "--ps", "wspreset", "thick", "--ps", "wssnapshot", "true") | Out-Null
Wait-Seconds 4

if ($Scenario -eq "EC-TT-01") {
  Wait-Seconds 2
  Capture-Frame "01_start"
  Invoke-Drag -FromX 640 -FromY 1400 -ToX 678 -ToY 1400 -PressMs 500 -TotalMs 3000
  Wait-Seconds 3
  Capture-Frame "02_right_hold"
  Invoke-Drag -FromX 678 -FromY 1400 -ToX 640 -ToY 1400 -PressMs 500 -TotalMs 3000
  Wait-Seconds 2
  Capture-Frame "03_return_settled"
  Invoke-Drag -FromX 640 -FromY 1400 -ToX 602 -ToY 1400 -PressMs 500 -TotalMs 3000
  Wait-Seconds 3
  Capture-Frame "04_left_hold"
  Invoke-Drag -FromX 602 -FromY 1400 -ToX 640 -ToY 1400 -PressMs 500 -TotalMs 3000
  Wait-Seconds 2
  Capture-Frame "05_final_settled"
}
elseif ($Scenario -eq "EC-TT-02") {
  for ($iteration = 1; $iteration -le 2; $iteration++) {
    $prefix = "{0:D2}" -f $iteration
    Capture-Frame "${prefix}_start"
    Invoke-Drag -FromX 640 -FromY 1400 -ToX 690 -ToY 1400 -PressMs 300 -TotalMs 1000
    Wait-Seconds 1
    Capture-Frame "${prefix}_right_hold"
    Invoke-Drag -FromX 690 -FromY 1400 -ToX 640 -ToY 1400 -PressMs 300 -TotalMs 1000
    Wait-Seconds 0.5
    Capture-Frame "${prefix}_returned"
    Invoke-Drag -FromX 640 -FromY 1400 -ToX 590 -ToY 1400 -PressMs 300 -TotalMs 1000
    Wait-Seconds 1
    Capture-Frame "${prefix}_left_hold"
    Invoke-Drag -FromX 590 -FromY 1400 -ToX 640 -ToY 1400 -PressMs 300 -TotalMs 1000
    Wait-Seconds 3
    Capture-Frame "${prefix}_final_settled"
  }
}
elseif ($Scenario -eq "EC-TT-03") {
  Capture-Frame "00_start"
  for ($cycle = 1; $cycle -le 3; $cycle++) {
    $prefix = "{0:D2}" -f ($cycle * 2 - 1)
    Invoke-Drag -FromX 640 -FromY 1400 -ToX 665 -ToY 1400 -PressMs 200 -TotalMs 500
    Capture-Frame "${prefix}_right"
    $prefix = "{0:D2}" -f ($cycle * 2)
    Invoke-Drag -FromX 665 -FromY 1400 -ToX 615 -ToY 1400 -PressMs 200 -TotalMs 500
    Capture-Frame "${prefix}_left"
  }
  Wait-Seconds 5
  Capture-Frame "07_settle_5s"
}
elseif ($Scenario -eq "EC-TT-04") {
  Capture-Frame "01_start"
  Invoke-Drag -FromX 640 -FromY 1400 -ToX 678 -ToY 1400 -PressMs 500 -TotalMs 1500
  Wait-Seconds 3
  Capture-Frame "02_right_hold"
  Invoke-Drag -FromX 678 -FromY 1400 -ToX 640 -ToY 1400 -PressMs 500 -TotalMs 1000
  Capture-Frame "03_returned"
  Invoke-Drag -FromX 640 -FromY 1400 -ToX 602 -ToY 1400 -PressMs 500 -TotalMs 1500
  Wait-Seconds 3
  Capture-Frame "04_left_hold"
  Invoke-Drag -FromX 602 -FromY 1400 -ToX 640 -ToY 1400 -PressMs 500 -TotalMs 1000
  Wait-Seconds 3
  Capture-Frame "05_final_settled"
}
elseif ($Scenario -eq "EC-TT-05") {
  Write-Output "EC-TT-05 is a replay review of saved recordings; no new input is injected by this script."
}

Invoke-DeviceShell @("hilog", "-x") | Out-File -FilePath (Join-Path $logDir "hilog.txt") -Encoding UTF8
Write-Output "Scenario=$Scenario Round=$Round Frames=$roundDir Log=$(Join-Path $logDir 'hilog.txt')"

