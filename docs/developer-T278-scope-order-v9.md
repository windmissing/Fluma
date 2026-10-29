# T278 诊断 scope 顺序补正

2026-10-29，developer，不使用真机。T250 正常 scope 回归发现共用诊断做法的缺陷：在 `emit(LEAVE)` 写出原scope行前做冷读取，会让冷读取产生的嵌套scope行插到该LEAVE前，导致父栈顺序错位。物理结果不变，但该顺序不能冒完整父子证据。

T278 隔离生成器将afterObserver移至原emit的finally，先写本行再冷读取；即使全局4096行溢出，finally仍产独立AFTER关键证据。没有改生产 TxWorkTrace/物理，NEXT/OBS/NA合同和分母不变。

新隔离 `.codex/local-test/t278-v9`：原AFTER **99/0**；同包scope父子顺序/正常物理开关等价 **12/0**。全局scope4096仍如实INCOMPLETE（截断），没有宣整流完整；独立逐点证据仍dropped=0。证据 test/reports/t278/scope-v9-summary.json、scope-v9-execution.json.gz、scope-order-v9.json。

hvigor PackageHap BUILD SUCCESSFUL，签名验签成功。新包 `.codex/local-test/t257-sign/T278-scope-v9/T278-scope-v9-signed.hap`，SHA256 `ff6188e08c2ae0cba4e8dd825620853dcd1b815696cb79f68010e58e134fac6f`；签名记录 test/reports/t257/T278-scope-v9.json。先前v8档保留追溯，续证改用v9包。未用真机、不改渲染通道或默认路由。
