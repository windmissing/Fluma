param([string]$Text = "8,8,32,32;565,210,48,48;1100,400,48,48")
$env:HTTP_PROXY=""; $env:HTTPS_PROXY=""; $env:http_proxy=""; $env:https_proxy=""
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue
$hdc = "C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
# 用 Start-Process 传参避免 Git Bash sh 分号拆分
$argList = @('shell','uitest','uiInput','text',$Text)
$out = & $hdc @argList 2>&1
Write-Output $out
