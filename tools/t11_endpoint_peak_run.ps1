# T11 P6 设备取证：endpoint 合规 run 与同账本 peak 双报
#   依据: docs/endpoint-protocol-P011-terminal-nondeposited-v1.md（协议 v1，APPROVED 未生效）
#         docs/tester-T40-o2-supplementary-v1.md §4（T11 取证资格判据）
#   臂: T11-A TC-WS-08 摇晃后静置（endpoint 合规主 run）
#       T11-B 外源准入 run（手势注入 + 摇晃，验证 IH/IL 与 count==epoch）
#       T11-C 静置对照（shake=0 沉积基线）
#   采集: WS_METRIC(250ms) + WS_SNAPSHOT + WS_EVENT + P011_OBS_STORE + store 文件 pull
# 注意: 前台执行（后台 PowerShell 长脚本会被杀）；USB 弹窗必须先关；testmode=true 传感器不接管
#       rendermode 4=MODE_WATERSAND（parseLiquidCAWant 默认 1=细沙CA，不带则物理场不步进、无 WS_METRIC）
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t11-endpoint-peak-run-20261025'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T11 device run start'

function Start-Run($arm, $shake) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps wssnapshot true --ps shake $shake --ps testmode true" | Out-Null
  Start-Sleep -Seconds 6
  Log-Step "started $arm shake=$shake"
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null   # dismiss USB popup
  Start-Sleep -Seconds 6
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/t11-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t11-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump-Run($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_METRIC|WS_SNAPSHOT|WS_EVENT|P011_OBS_STORE|P011_MODE|LiquidCA want|P011_SCHEDULE|WaterSand' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}
function Pull-Store($tag) {
  # 从本臂 dump 提取 P011_OBS_STORE 的 filePath 并 pull
  $dump = Join-Path $out "$tag-full.txt"
  $line = Select-String -Path $dump -Pattern 'P011_OBS_STORE' | Select-Object -First 1
  if ($line -match '"filePath":"([^"]+)"') {
    $fp = $matches[1]
    Log-Step "$tag store filePath=$fp"
    & $hdc file recv $fp (Join-Path $out "$tag-store.ndjson") 2>&1 | Out-Null
    if (Test-Path (Join-Path $out "$tag-store.ndjson")) {
      Log-Step "$tag store pulled ($((Get-Item (Join-Path $out "$tag-store.ndjson")).Length) bytes)"
    } else { Log-Step "$tag store pull FAILED" }
  } else { Log-Step "$tag P011_OBS_STORE not found in dump" }
}

# ---------- T11-A: TC-WS-08 摇晃后静置（endpoint 合规主 run） ----------
Log-Step 'T11-A start (shake=1.0, settle >=8s, last-1s window)'
Start-Run 'T11-A' '1.0'
Start-Sleep -Seconds 10          # 摇晃衰减段
Shot 'T11-A-shaking'
Start-Sleep -Seconds 30          # 静置段（>=8s）
Shot 'T11-A-settled'
Start-Sleep -Seconds 15          # 末 1s 窗 + 余量
Shot 'T11-A-final'
Dump-Run 'T11-A-full.txt'
Pull-Store 'T11-A'
Log-Step 'T11-A done'

# ---------- T11-B: 外源准入 run（注入 + 摇晃） ----------
Log-Step 'T11-B start (inject via swipe + shake=0.6)'
Start-Run 'T11-B' '0.6'
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null   # 全宽 swipe 注沙
Start-Sleep -Seconds 3
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
Start-Sleep -Seconds 8
Shot 'T11-B-injected'
Start-Sleep -Seconds 18          # 摇晃后静置
Shot 'T11-B-settled'
Dump-Run 'T11-B-full.txt'
Pull-Store 'T11-B'
Log-Step 'T11-B done'

# ---------- T11-C: 静置对照（shake=0） ----------
Log-Step 'T11-C start (shake=0 baseline)'
Start-Run 'T11-C' '0'
Start-Sleep -Seconds 10
Shot 'T11-C-settled'
Start-Sleep -Seconds 15
Dump-Run 'T11-C-full.txt'
Pull-Store 'T11-C'
Log-Step 'T11-C done'

Write-Output "DONE"
