$Hdc='C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$Target='62T0225B10010020'
$Bundle='com.example.helloworld'
$Out='test\artifacts\p011-tc02-original-device-grid-20261016.log'
$Err="$Out.stderr"
Remove-Item $Out,$Err -Force -ErrorAction SilentlyContinue
& $Hdc -t $Target shell aa force-stop $Bundle | Out-Null
& $Hdc -t $Target shell hilog -r | Out-Null
$Hilog=Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog') -RedirectStandardOutput $Out -RedirectStandardError $Err -WindowStyle Hidden -PassThru
& $Hdc -t $Target file send 'D:\hmprojects\HelloWorld\test\p011_tc02_original_device.sh' /data/local/tmp/p011_tc02_original_device.sh
& $Hdc -t $Target shell chmod +x /data/local/tmp/p011_tc02_original_device.sh
& $Hdc -t $Target shell sh /data/local/tmp/p011_tc02_original_device.sh
if (-not $Hilog.HasExited) { Stop-Process -Id $Hilog.Id -Force }
$Hilog.WaitForExit(3000)
(Select-String -Path $Out -Pattern 'P011_EVENT input').Count
