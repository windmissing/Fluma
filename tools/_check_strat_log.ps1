$HDC = "C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
& $HDC shell aa force-stop com.example.helloworld
Start-Sleep -Seconds 1
& $HDC shell aa start -a EntryAbility -b com.example.helloworld --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true
Start-Sleep -Seconds 3
& $HDC shell uitest uiInput click 240 620
Start-Sleep -Seconds 5
& $HDC shell "hilog -x | grep strat | tail -12"
