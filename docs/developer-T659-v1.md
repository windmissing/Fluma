# T659：单沙准备 owner 纯值身份与票据链修复

2026-11-19，developer。按 T658 已交工程/机器合同完成本卡开发与编译，**结束待验收**。不使用真机、不运行测试、不签名部署；READY/RESERVED/HOLD/严格 PNG 产品结果未测，不作 PASS 声明。

## 实现

- `test/t426/PrepareReadinessV4.ets` 保留 PrepareAttemptOwner 构造名，去掉原型方法依赖。copyPrepareOwner 校验前九项为非负 safe integer、原 native 四项非零及非空 run，再逐字段复制十 primitive；samePrepareOwner 逐字段 strict equal；prepareOwnerKey 按 T658 唯一十项 tuple 序列化，不作授权。内部只持有独立快照，取证访问返回另一个副本，调用方修改原对象不影响 before。
- 首次 bind 受检递增 ownerSerial，等值 fresh owner 保留快照/serial；本 attempt 身份改变撤销，未知/不可精确表示身份具名失败。旧 prepare/page 的 owner、ticket 只 discard。begin/show 复位当前许可；hide 撤销并清 owner。changed 直接调用原父语义，不比较同一对象冒 fresh 读数。frame 未绑定 owner 不进入 READY；check、checkForReserve 与 controller 的真实 guard 共用原 strict predicate 和同一个 owner 核对，原输入/布局/nativeReady 变化仍撤销。
- ticket 绑定 prepare/page/observation/input/**ownerSerial**/ownerKey，gate 私有保存发行值，消费核全部字段且一次。实际生成页仍同 UI 栈 fresh read→scope 核对→check ticket→真实 reserveCapture→仅 RESERVED 后 consume；无 await/改布局插入。消费失配立即使 capture 失效并 return，不再执行注入/继续 HOLD；reserve 失败不消费。
- `test/t574/t574_prepare_v4_overlay.cjs` 在真实产品页链调用新 `t659_owner_glue.cjs`。窗口订阅在每次 branch 按真实引用、window generation、prepare/page/branch 重绑；帧关联实际 window 引用，读回后再核同代。旧 frame/deadline/window 回调只 discard，不再用旧 deadline 撤销新 attempt。异步预约错误记录捕获的原 scope，不把旧 I/O 错误写入新 attempt。
- 窗口/帧/owner/票据相关错误先保存首末 raw code/name/message、before/after owner 与 scope，再使当前 attempt fail/disable；非当前错误 discard。message UTF8 每项≤1024B，转义后的 JSON 长度也受限，显式截断标志。gate 报告≤14336B，完整 prepare 冷文件≤16384B；额外 observation/历史过长明确标 detail/observationTruncated，异步写有 catch/finally，不吞错误继续 READY。

完整字段/预算/票据字典见 `test/reports/t659/owner-contract.json`。未改守恒/物理、ROI/HOLD/PNG 标定、3000ms 仅 PREPARING 规则、READY 无 TTL 及响应式 reserveEnabled；没有新增渲染/Canvas 回退。

## 可复现产品编译

从仓库根目录执行：

```powershell
python tools/singlesand/prepare_owner_build.py .codex/local-test/t659-new-source
```

该工具导出 **T592 所测 T598 的固定产品基线 9ec800f4** 到全新隔离源码目录，叠加工作区真实 owner 模板和两个 overlay，再执行该基线 `test/t598/generate.cjs`；不是只修改 local-test 输出。源码/lock/native/pages 都保该正式产品基线，并编实际 P051DerivedV2Demo 及原 single-sand controller/renderer；不是 compile-anchor 空壳。

选择固定基线的原因：当前工作区 DiagnosticColdFiles 已有后续信用 writer，旧 T372 生成器的文本接缝 `const bytes=...;const f=` 已不存在；直接从当前根生成会报 T372_ANCHOR。此次没有回退/改写当前冷 writer，也不扩大本卡到无关冷 API 迁移。工具固定真实已交产品版本、记录叠加源 SHA 和全部生成源 SHA，后续 T592 应使用下面**本次新包**，不能复用旧 signed 包。

实际构建目录：`.codex/local-test/t659-delivery-source/.codex/local-test/product`。在该目录使用 DevEco hvigor：

```text
hvigorw.bat --mode module -p product=default -p module=entry@default assembleHap --no-daemon
```

完整 native/ArkTS/unsigned HAP 构建 **58.605s**，最后 owner/frame/check 与 JSON 预算修订后的 ArkTS 增量构建 **14.437s**，均零错误；最终 CompileArkTS 实际执行 6.521s。存在 SDK/既有 API 编译警告，不声称零警告。未执行 unit、应用、shader、签名、安装或设备取证。

| 原件 | 结果 |
|---|---|
| unsigned HAP | `.codex/local-test/t659-delivery-source/.codex/local-test/product/entry/build/default/outputs/default/entry-default-unsigned.hap` |
| HAP SHA256 / bytes | `f3a0beb3377b4a00df61170b7e748389fe6d1219b5f54d07703b47c6bfa9b42e` / 7980038 |
| 包内 ets/modules.abc SHA256 / bytes | `28077347107671a7013380238b16adff0ce23ac91700be3b6cfc0d684b2dc6fe` / 2159204 |
| 包内 libp013renderer.so SHA256 | `4a18eb40342e63d229828c190d296bbd5a980154f983758766337b55530920f7` |
| 全部源及编译收据 | `test/reports/t659/generated-sources.json`、`compile-receipt.json` |
| 原始生成/编译日志 | `generation.log`、`full-build.log`、`final-build.log`（同目录，保原字节） |

已逐文件核对实际生成源与 fingerprint，并记录全部包内 native SHA；编译指纹不是运行/产品 PASS。后继由 T592 tester 对同一新版本独立执行十字段变化、同 ID 不同窗口引用/代号、readiness 下降、过期回调、票据重复消费/reserve 失败/consume 失配和生命周期，以及真实固定键盘→prepare→RESERVED→HOLD→严格 PNG。设备验证不作本开发卡关闭前置。
