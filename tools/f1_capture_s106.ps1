$HDC = "C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
& $HDC shell hilog -r
& $HDC shell aa force-stop com.example.helloworld
Start-Sleep -Seconds 1
& $HDC shell aa start -a EntryAbility -b com.example.helloworld --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true
Start-Sleep -Seconds 3
& $HDC shell uitest uiInput click 240 620
Start-Sleep -Seconds 32
# 动态段：全宽 swipe 制造倾斜
& $HDC shell uitest uiInput swipe 100 1500 1200 1500 300
Start-Sleep -Seconds 6
& $HDC shell uitest uiInput swipe 1200 1500 100 1500 300
Start-Sleep -Seconds 6
& $HDC shell uitest uiInput swipe 100 1500 1200 1500 300
Start-Sleep -Seconds 6
& $HDC shell "hilog -x -t WaterSand | grep WS_FRAME" | Out-File -Encoding utf8 D:\hmprojects\HelloWorld\test\artifacts\p012-measurements\f1-s106-wsframe-hilog.txt
$n = (Get-Content D:\hmprojects\HelloWorld\test\artifacts\p012-measurements\f1-s106-wsframe-hilog.txt | Measure-Object -Line).Lines
Write-Host "lines=$n"
